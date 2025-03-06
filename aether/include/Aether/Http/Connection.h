// Connection.h
#ifndef AETHER_HTTP_CONNECTION_H
#define AETHER_HTTP_CONNECTION_H

#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include "Aether/Http/Middleware.h"
#include <boost/asio.hpp>
#include <memory>
#include <functional>

namespace Aether {
namespace Http {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using RequestHandler = std::function<void(Request&, Response&)>;
    using HandlerLookup = std::function<RequestHandler(const std::string&, const std::string&, Request&)>;

    Connection(
        boost::asio::ip::tcp::socket socket,
        HandlerLookup handlerLookup,
        MiddlewareStack& middlewareStack,
        std::function<void()> cleanupCallback
    );

    void start();

private:
    void handleReadHeaders(const boost::system::error_code& error, size_t bytes_transferred);
    void handleContentLengthBody(size_t header_bytes);
    void processRequest();
    void buildResponse();
    void sendResponse();
    void handleNetworkError(const boost::system::error_code& error);
    void sendError(int statusCode);
    void resetTimeout();
    void handleChunkedBody();
    void closeConnection(); // Declare closeConnection method
    void send100Continue(); // Declare send100Continue method
    void processChunkedBody(size_t bytes_transferred); // Declare processChunkedBody method
    void handleRequestBody(); // Declare handleRequestBody method

    boost::asio::ip::tcp::socket socket_;
    boost::asio::deadline_timer timeoutTimer_;
    HandlerLookup handlerLookup_;
    MiddlewareStack& middlewareStack_;
    std::function<void()> cleanupCallback_;

    std::string requestBuffer_;
    std::string responseData_;
    Request req_;
    Response res_;

    bool isClosed_; // Declare isClosed_ member variable
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_CONNECTION_H