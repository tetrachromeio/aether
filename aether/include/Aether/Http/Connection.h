// Connection.h
#ifndef AETHER_HTTP_CONNECTION_H
#define AETHER_HTTP_CONNECTION_H

#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include "Aether/Http/Middleware.h"
#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#endif
#include <boost/asio.hpp>
#include <memory>
#include <functional>

namespace Aether {
namespace Http {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using RequestHandler = std::function<void(Request&, Response&)>;
    using HandlerLookup = std::function<RequestHandler(const std::string&, const std::string&, Request&)>;

    static constexpr std::size_t kMaxBodySizeBytes = 10 * 1024 * 1024; // 10 MB safeguard

    Connection(
        boost::asio::ip::tcp::socket socket,
        HandlerLookup handlerLookup,
        MiddlewareStack& middlewareStack,
        std::function<void()> cleanupCallback
    );

    void start();
    bool isKeepAliveRequested() const;
    static bool iequals(const std::string& a, const std::string& b);
    static bool wantsKeepAlive(const Request& req);
    static bool exceedsBodyLimit(std::size_t currentBytes, std::size_t incomingBytes);

private:
    void handleReadHeaders(const boost::system::error_code& error, size_t bytes_transferred);
    void handleContentLengthBody();
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
    boost::asio::steady_timer timeoutTimer_;
    HandlerLookup handlerLookup_;
    MiddlewareStack& middlewareStack_;
    std::function<void()> cleanupCallback_;

    std::string requestBuffer_;
    std::string responseData_;
    Request req_;
    Response res_;

    bool isClosed_; // Declare isClosed_ member variable
    std::size_t totalBodyBytes_{0};
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_CONNECTION_H
