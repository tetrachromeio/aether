#include "Aether/Http/Connection.h"
#include "Aether/Http/HttpParser.h"
#include <iostream>
#include <cctype>
#include <chrono>

namespace Aether {
namespace Http {

Connection::Connection(
    boost::asio::ip::tcp::socket socket,
    HandlerLookup handlerLookup,
    MiddlewareStack& middlewareStack,
    std::function<void()> cleanupCallback
)
    : socket_(std::move(socket)),
      timeoutTimer_(socket_.get_executor()),
      handlerLookup_(std::move(handlerLookup)),
      middlewareStack_(middlewareStack),
      cleanupCallback_(std::move(cleanupCallback)),
      requestBuffer_(),
      responseData_(),
      req_(),
      res_(),
      isClosed_(false),
      totalBodyBytes_(0) {
    boost::system::error_code ec;
    socket_.set_option(boost::asio::ip::tcp::no_delay(true), ec);
}

void Connection::start() {
    resetTimeout();
    requestBuffer_.clear();
    req_ = Request{};
    res_ = Response{};
    totalBodyBytes_ = 0;

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
    std::size_t bytes_transferred
) {
    if (error) {
        handleNetworkError(error);
        return;
    }

    // bytes_transferred is the number of bytes up to and including "\r\n\r\n".
    // There may already be some body bytes after that in requestBuffer_.
    if (bytes_transferred > requestBuffer_.size()) {
        // Should not happen, but guard anyway.
        sendError(400);
        return;
    }

    // Parse request line + headers only from the header portion.
    std::string_view headerView(requestBuffer_.data(), bytes_transferred);
    if (!HttpParser::parseRequest(headerView, req_)) {
        sendError(400);
        return;
    }

    // Extract any body bytes that arrived in the same read as the headers.
    const std::size_t extraBodyBytes = requestBuffer_.size() - bytes_transferred;
    if (extraBodyBytes > 0) {
        req_.body.assign(
            requestBuffer_.data() + bytes_transferred,
            extraBodyBytes
        );
        totalBodyBytes_ = extraBodyBytes;
    } else {
        req_.body.clear();
        totalBodyBytes_ = 0;
    }

    // We no longer need the header bytes in requestBuffer_.
    requestBuffer_.clear();

    // Validate Host header (required for HTTP/1.1)
    const auto hostIt = req_.headers.find("host");
    if (req_.version == "HTTP/1.1" && hostIt == req_.headers.end()) {
        sendError(400); // Bad Request: Host header is required
        return;
    }

    // Handle Expect: 100-continue
    auto expectIt = req_.headers.find("expect");
    if (expectIt != req_.headers.end() && iequals(expectIt->second, "100-continue")) {
        send100Continue();
        return; // Wait for the client to send the body
    }

    // Handle request body
    const bool hasBody = req_.headers.count("content-length");
    const bool chunked = req_.headers.count("transfer-encoding") &&
                         iequals(req_.headers.at("transfer-encoding"), "chunked");

    if (hasBody) {
        handleContentLengthBody();
    } else if (chunked) {
        handleChunkedBody();
    } else {
        processRequest(); // No body: process immediately
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
    const bool hasBody = req_.headers.count("content-length");
    const bool chunked = req_.headers.count("transfer-encoding") &&
                         iequals(req_.headers.at("transfer-encoding"), "chunked");

    if (hasBody) {
        handleContentLengthBody(); // Start reading the body
    } else if (chunked) {
        handleChunkedBody(); // Start reading the chunked body
    } else {
        processRequest(); // No body, process the request
    }
}

void Connection::handleContentLengthBody() {
    try {
        const auto& headerVal = req_.headers.at("content-length");
        const std::size_t content_length = std::stoull(headerVal);

        // totalBodyBytes_ currently includes any bytes that arrived with headers
        if (totalBodyBytes_ > content_length) {
            sendError(400);
            return;
        }

        const std::size_t remaining = content_length - totalBodyBytes_;

        if (exceedsBodyLimit(totalBodyBytes_, remaining)) {
            sendError(413); // Payload Too Large
            return;
        }

        if (remaining > 0) {
            // Read exactly the remaining bytes into requestBuffer_, then append to req_.body
            boost::asio::async_read(
                socket_,
                boost::asio::dynamic_buffer(requestBuffer_),
                boost::asio::transfer_exactly(remaining),
                [self = shared_from_this(), remaining](auto error, auto) {
                    if (error) {
                        self->handleNetworkError(error);
                        return;
                    }

                    // Append newly read body bytes
                    self->req_.body.append(self->requestBuffer_.data(), remaining);
                    self->totalBodyBytes_ += remaining;
                    self->requestBuffer_.clear();

                    self->processRequest();
                }
            );
        } else {
            // We already have the full body
            processRequest();
        }
    } catch (const std::exception&) {
        sendError(400);
    }
}

void Connection::handleChunkedBody() {
    // Read the next chunk size line
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

void Connection::processChunkedBody(std::size_t bytes_transferred) {
    // bytes_transferred includes the chunk size line + "\r\n"
    const std::string sizeLine = requestBuffer_.substr(0, bytes_transferred);
    std::size_t chunkSize = 0;
    try {
        chunkSize = std::stoul(sizeLine, nullptr, 16);
    } catch (const std::exception&) {
        sendError(400);
        return;
    }

    // Remove the size line and trailing "\r\n"
    requestBuffer_.erase(0, bytes_transferred);

    if (chunkSize == 0) {
        // End of chunks: no more data, process the full request
        processRequest();
        return;
    }

    if (exceedsBodyLimit(totalBodyBytes_, chunkSize)) {
        sendError(413);
        return;
    }

    // Read the chunk data (+2 for the trailing "\r\n")
    boost::asio::async_read(
        socket_,
        boost::asio::dynamic_buffer(requestBuffer_),
        boost::asio::transfer_exactly(chunkSize + 2),
        [self = shared_from_this(), chunkSize](auto error, auto) {
            if (error) {
                self->handleNetworkError(error);
                return;
            }

            // Append chunk data (exclude trailing "\r\n")
            self->req_.body.append(self->requestBuffer_.data(), chunkSize);
            self->requestBuffer_.erase(0, chunkSize + 2);
            self->totalBodyBytes_ += chunkSize;

            // Read the next chunk
            self->handleChunkedBody();
        }
    );
}

void Connection::processRequest() {
    try {
        // At this point, req_.body already contains the full body (if any).
        // No need to re-scan or re-parse the buffer for "\r\n\r\n".

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
    const bool keep_alive = isKeepAliveRequested();
    std::string statusText = HttpParser::statusText(res_.statusCode);

    responseData_.clear();
    // Reserve to minimize reallocations: status line + headers + body
    responseData_.reserve(
        res_.body.size() +
        statusText.size() +
        64 +
        res_.headers.size() * 32
    );

    responseData_.append(req_.version);
    responseData_.push_back(' ');
    responseData_.append(std::to_string(res_.statusCode));
    responseData_.push_back(' ');
    responseData_.append(statusText);
    responseData_.append("\r\n");

    responseData_.append("Content-Length: ");
    responseData_.append(std::to_string(res_.body.size()));
    responseData_.append("\r\n");

    responseData_.append("Connection: ");
    responseData_.append(keep_alive ? "keep-alive" : "close");
    responseData_.append("\r\n");

    for (const auto& [key, value] : res_.headers) {
        responseData_.append(key);
        responseData_.append(": ");
        responseData_.append(value);
        responseData_.append("\r\n");
    }

    responseData_.append("\r\n");
    responseData_.append(res_.body);
}

void Connection::sendResponse() {
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(responseData_),
        [self = shared_from_this()](auto error, auto /*bytes_transferred*/) {
            if (error) {
                self->handleNetworkError(error);
                return;
            }

            // Handle keep-alive
            if (self->isKeepAliveRequested()) {
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
        std::string errorHtml = "<html><body><h1>Error " +
                                std::to_string(statusCode) +
                                "</h1><p>An error occurred while processing your request.</p></body></html>";

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
    timeoutTimer_.expires_after(std::chrono::seconds(30)); // 30-second timeout
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

    if (socket_.is_open()) {
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec && ec != boost::asio::error::not_connected) {
            std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
        }

        socket_.close(ec);
        if (ec) {
            std::cerr << "Error closing socket: " << ec.message() << std::endl;
        }
    }

    cleanupCallback_();
}

bool Connection::isKeepAliveRequested() const {
    return wantsKeepAlive(req_);
}

bool Connection::wantsKeepAlive(const Request& req) {
    bool keepAlive = req.version == "HTTP/1.1";
    auto it = req.headers.find("connection");
    if (it != req.headers.end()) {
        const std::string& value = it->second;
        if (iequals(value, "close")) {
            keepAlive = false;
        } else if (iequals(value, "keep-alive")) {
            keepAlive = true;
        }
    }
    return keepAlive;
}

bool Connection::exceedsBodyLimit(std::size_t currentBytes, std::size_t incomingBytes) {
    return incomingBytes > kMaxBodySizeBytes ||
           currentBytes > kMaxBodySizeBytes ||
           (incomingBytes > 0 && currentBytes > kMaxBodySizeBytes - incomingBytes);
}

bool Connection::iequals(const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char lhs, char rhs) {
                          return std::tolower(static_cast<unsigned char>(lhs)) ==
                                 std::tolower(static_cast<unsigned char>(rhs));
                      });
}

} // namespace Http
} // namespace Aether
