#include "Aether/Http/Connection.h"
#include "Aether/Http/HttpParser.h"
#include <iostream>

namespace Aether {
namespace Http {

Connection::Connection(
    boost::asio::ip::tcp::socket socket,
    std::function<RequestHandler(const std::string&, const std::string&)> handlerLookup,
    MiddlewareStack& middlewareStack,
    std::function<void()> cleanupCallback
) : socket_(std::move(socket)),
    timeoutTimer_(socket_.get_executor()),
    handlerLookup_(handlerLookup),
    middlewareStack_(middlewareStack),
    cleanupCallback_(cleanupCallback) {}

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

    // Handle request body
    const bool hasBody = req_.headers.count("Content-Length");
    const bool chunked = req_.headers.count("Transfer-Encoding");

    if (hasBody) {
        handleContentLengthBody(bytes_transferred);
    } else if (chunked) {
        handleChunkedBody();
    } else {
        processRequest();
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

void Connection::processRequest() {
    try {
        // Extract body
        size_t body_pos = requestBuffer_.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            req_.body = requestBuffer_.substr(body_pos + 4);
        }

        // Execute middleware stack
        middlewareStack_.run(req_, res_, [this]() {
            RequestHandler handler = handlerLookup_(req_.method, req_.path);
            if (handler) {
                handler(req_, res_);
            } else {
                res_.send("404 Not Found", 404);
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
        [self = shared_from_this()](auto error, auto) {
            if (error) {
                self->handleNetworkError(error);
                return;
            }
            
            // Handle keep-alive
            bool keep_alive = self->responseData_.find("Connection: keep-alive") != std::string::npos;
            if (keep_alive) {
                self->start(); // Reset for next request
            } else {
                self->cleanupCallback_();
                self->socket_.close();
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
    cleanupCallback_();
    socket_.close();
}

void Connection::sendError(int statusCode) {
    responseData_ = 
        "HTTP/1.1 " + std::to_string(statusCode) + " " + 
        HttpParser::statusText(statusCode) + "\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n";
        
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(responseData_),
        [self = shared_from_this()](auto, auto) {
            self->cleanupCallback_();
            self->socket_.close();
        }
    );
}

void Connection::resetTimeout() {
    timeoutTimer_.expires_from_now(boost::posix_time::seconds(30));
    timeoutTimer_.async_wait([self = shared_from_this()](auto error) {
        if (!error) {
            self->socket_.close();
            self->cleanupCallback_();
        }
    });
}

void Connection::handleChunkedBody() {
    sendError(501); // 501 Not Implemented
}


} // namespace Http
} // namespace Aether