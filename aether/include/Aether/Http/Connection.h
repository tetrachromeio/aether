#ifndef AETHER_HTTP_CONNECTION_H
#define AETHER_HTTP_CONNECTION_H

#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include "Aether/Http/Middleware.h"
#include "Aether/Http/HttpParser.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>

namespace Aether {
namespace Http {

using RequestHandler = std::function<void(Request&, Response&)>;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(
        boost::asio::ip::tcp::socket socket,
        std::function<RequestHandler(const std::string&, const std::string&)> handlerLookup,
        MiddlewareStack& middlewareStack,
        std::function<void()> cleanupCallback
    );

    void start();

private:
    // Declare all private methods used in Connection.cpp
    void handleReadHeaders(const boost::system::error_code& error, size_t bytes_transferred);
    void handleContentLengthBody(size_t header_bytes);
    void handleChunkedBody(); // Stub for future implementation
    void handleNetworkError(const boost::system::error_code& error);
    void processRequest();
    void buildResponse();
    void sendResponse();
    void sendError(int statusCode);
    void resetTimeout();

    boost::asio::ip::tcp::socket socket_;
    boost::asio::deadline_timer timeoutTimer_;
    
    std::string requestBuffer_;
    std::string responseData_;
    Request req_;
    Response res_;

    std::function<RequestHandler(const std::string&, const std::string&)> handlerLookup_;
    MiddlewareStack& middlewareStack_;
    std::function<void()> cleanupCallback_;
};

} // namespace Http
} // namespace Chromate

#endif // CHROMATE_HTTP_CONNECTION_H