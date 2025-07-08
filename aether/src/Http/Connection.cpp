#include "Aether/Http/Connection.h"
#include "Aether/Http/HttpParser.h"
#include <iostream>

namespace Aether {
namespace Http {

Connection::Connection(
    boost::asio::ip::tcp::socket socket,
    HandlerLookup handlerLookup,
    MiddlewareStack& middlewareStack,
    std::function<void()> cleanupCallback
) : socket_(std::move(socket)),
    timeoutTimer_(socket_.get_executor()),
    handlerLookup_(handlerLookup),
    middlewareStack_(middlewareStack),
    cleanupCallback_(cleanupCallback),
    isClosed_(false) {} // Initialize isClosed_ to false

void Connection::start() {
    resetTimeout();
    requestBuffer_.clear();
    req_ = Request{};
    res_ = Response{};
    
    boost::asio::async_read_until(
        socket_,
        boost::asio::dynamic_buffer(requestBuffer_),
        "\r\n\r\n",
        [self = shared_from_this()](auto error, auto bytes) {
            self->handleReadHeaders(error, bytes);
        }
    );
}

void Connection::handleReadHeaders(
    const boost::system::error_code& error, 
    size_t bytes_transferred
) {
    if (error) {
        handleNetworkError(error);
        return;
    }

    // Parse request line and headers
    if (!HttpParser::parseRequest(requestBuffer_, req_)) {
        sendError(400);
        return;
    }

    // Validate Host header (required for HTTP/1.1)
    if (!req_.headers.count("Host")) {
        sendError(400); // Bad Request: Host header is required
        return;
    }

    // Handle Expect: 100-continue
    if (req_.headers.count("Expect") && req_.headers["Expect"] == "100-continue") {
        send100Continue();
        return; // Wait for the client to send the body
    }

    // Handle request body
    const bool hasBody = req_.headers.count("Content-Length");
    const bool chunked = req_.headers.count("Transfer-Encoding") && 
                         req_.headers["Transfer-Encoding"] == "chunked";

    if (hasBody) {
        handleContentLengthBody(bytes_transferred);
    } else if (chunked) {
        handleChunkedBody();
    } else {
        processRequest();
    }
}

void Connection::send100Continue() {
    static const std::string continueResponse = "HTTP/1.1 100 Continue\r\n\r\n";
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(continueResponse),
        [self = shared_from_this()](auto error, auto) {
            if (error) {
                self->handleNetworkError(error);
                return;
            }

            // Wait for the client to send the body
            self->handleRequestBody();
        }
    );
}



void Connection::handleRequestBody() {
    const bool hasBody = req_.headers.count("Content-Length");
    const bool chunked = req_.headers.count("Transfer-Encoding") && 
                         req_.headers["Transfer-Encoding"] == "chunked";

    if (hasBody) {
        handleContentLengthBody(0); // Start reading the body
    } else if (chunked) {
        handleChunkedBody(); // Start reading the chunked body
    } else {
        processRequest(); // No body, process the request
    }
}

void Connection::handleContentLengthBody(size_t header_bytes) {
    try {
        size_t content_length = std::stoul(req_.headers["Content-Length"]);
        size_t remaining = content_length - (requestBuffer_.size() - header_bytes - 4);

        if (remaining > 0) {
            boost::asio::async_read(
                socket_,
                boost::asio::dynamic_buffer(requestBuffer_),
                boost::asio::transfer_exactly(remaining),
                [self = shared_from_this()](auto error, auto) {
                    if (error) {
                        self->handleNetworkError(error);
                        return;
                    }
                    self->processRequest();
                }
            );
        } else {
            processRequest();
        }
    } catch (const std::exception&) {
        sendError(400);
    }
}

void Connection::handleChunkedBody() {
    // Read the first chunk
    boost::asio::async_read_until(
        socket_,
        boost::asio::dynamic_buffer(requestBuffer_),
        "\r\n",
        [self = shared_from_this()](auto error, auto bytes) {
            if (error) {
                self->handleNetworkError(error);
                return;
            }
            self->processChunkedBody(bytes);
        }
    );
}

void Connection::processChunkedBody(size_t bytes_transferred) {
    size_t chunkSize;
    std::istringstream iss(requestBuffer_.substr(0, bytes_transferred));
    iss >> std::hex >> chunkSize;

    if (chunkSize == 0) {
        // End of chunks
        processRequest();
        return;
    }

    // Read the chunk data
    requestBuffer_.erase(0, bytes_transferred + 2); // Remove chunk size line
    boost::asio::async_read(
        socket_,
        boost::asio::dynamic_buffer(requestBuffer_),
        boost::asio::transfer_exactly(chunkSize + 2), // +2 for \r\n
        [self = shared_from_this(), chunkSize](auto error, auto) {
            if (error) {
                self->handleNetworkError(error);
                return;
            }

            // Append chunk data to body
            self->req_.body.append(self->requestBuffer_.substr(0, chunkSize));
            self->requestBuffer_.erase(0, chunkSize + 2); // Remove chunk data and \r\n

            // Read the next chunk
            self->handleChunkedBody();
        }
    );
}

void Connection::processRequest() {
    try {
        // Extract body
        size_t body_pos = requestBuffer_.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            req_.body = requestBuffer_.substr(body_pos + 4);
        }

        // Execute middleware stack
        middlewareStack_.run(req_, res_, [this]() {
            RequestHandler handler = handlerLookup_(req_.method, req_.path, req_);
            if (handler) {
                handler(req_, res_);
            } else {
                // Send 404 for unmatched routes
                sendError(404);
            }
        });


        buildResponse();
        sendResponse();
    } catch (const std::exception& e) {
        std::cerr << "Processing error: " << e.what() << std::endl;
        sendError(500);
    }
}

void Connection::buildResponse() {
    std::ostringstream oss;
    
    // Status line
    oss << req_.version << " " 
        << res_.statusCode << " "
        << HttpParser::statusText(res_.statusCode) << "\r\n";

    // Headers
    oss << "Content-Length: " << res_.body.size() << "\r\n";
    
    // Connection management
    bool keep_alive = (req_.version == "HTTP/1.1") || 
                     (req_.headers["Connection"] == "keep-alive");
    oss << "Connection: " << (keep_alive ? "keep-alive" : "close") << "\r\n";

    // Custom headers
    for (const auto& [key, value] : res_.headers) {
        oss << key << ": " << value << "\r\n";
    }

    oss << "\r\n" << res_.body;
    responseData_ = oss.str();
}

void Connection::sendResponse() {
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(responseData_),
        [self = shared_from_this()](auto error, auto bytes_transferred) {
            if (error) {
                self->handleNetworkError(error);
                return;
            }
            
            // Handle keep-alive
            bool keep_alive = self->responseData_.find("Connection: keep-alive") != std::string::npos;
            if (keep_alive) {
                self->start(); // Reset for next request
            } else {
                self->closeConnection();
            }
        }
    );
}

void Connection::handleNetworkError(const boost::system::error_code& error) {
    if (error.category() == boost::asio::error::get_system_category()) {
        switch (error.value()) {
            case boost::asio::error::eof:
            case boost::asio::error::connection_reset:
            case boost::asio::error::operation_aborted:
                // Benign errors, no logging needed
                break;
            default:
                std::cerr << "Network error: " << error.message() << std::endl;
        }
    }
    closeConnection();
}

void Connection::sendError(int statusCode) {
    if (statusCode == 404) {
        responseData_ =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 45\r\n"
            "Connection: close\r\n\r\n"
            "<html><body><h1>404 Not Found</h1></body></html>";
    } else {
        // Generate a simple error page without external files
        std::string errorHtml = "<html><body><h1>Error " + std::to_string(statusCode) + "</h1><p>An error occurred while processing your request.</p></body></html>";

        std::ostringstream oss;
        oss << "HTTP/1.1 " << statusCode << " " << HttpParser::statusText(statusCode) << "\r\n";
        oss << "Content-Type: text/html\r\n";
        oss << "Content-Length: " << errorHtml.size() << "\r\n";
        oss << "Connection: close\r\n\r\n";
        oss << errorHtml;

        responseData_ = oss.str();
    }

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(responseData_),
        [self = shared_from_this()](auto, auto) {
            self->closeConnection();
        }
    );
}


void Connection::resetTimeout() {
    timeoutTimer_.expires_from_now(boost::posix_time::seconds(30)); // 30-second timeout
    timeoutTimer_.async_wait([self = shared_from_this()](auto error) {
        if (!error) {
            self->closeConnection();
        }
    });
}

void Connection::closeConnection() {
    if (isClosed_) return; // Ensure socket is only closed once
    isClosed_ = true;

    boost::system::error_code ec;

    // Check if the socket is still open before shutting it down
    if (socket_.is_open()) {
        // Shutdown the socket gracefully
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) {
            // Log the error only if it's not "not connected"
            if (ec != boost::asio::error::not_connected) {
                std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
            }
        }

        // Close the socket
        socket_.close(ec);
        if (ec) {
            std::cerr << "Error closing socket: " << ec.message() << std::endl;
        }
    }

    cleanupCallback_();
}

} // namespace Http
} // namespace Aether