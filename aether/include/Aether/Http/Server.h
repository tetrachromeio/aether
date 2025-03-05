// Server.h
#ifndef AETHER_HTTP_SERVER_H
#define AETHER_HTTP_SERVER_H

#include "Aether/Core/EventLoop.h"
#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include "Aether/Http/Middleware.h"
#include <boost/asio.hpp>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <string>

namespace Aether {
namespace Http {

class Connection; // Forward declaration

using RequestHandler = std::function<void(Request&, Response&)>;

class Server {
public:
    Server();
    ~Server();

    // Add io_context accessor
    boost::asio::io_context& ioContext() { return eventLoop_.getIoContext(); }

    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void put(const std::string& path, RequestHandler handler);
    void del(const std::string& path, RequestHandler handler);

    void use(Middleware middleware);
    void run(int port);

    // Add views method
    void views(const std::string& folder);

private:
    void startAccept();
    void handleNewConnection(
        const boost::system::error_code& error,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket
    );

    RequestHandler findHandler(const std::string& method, const std::string& path);

    EventLoop eventLoop_;
    boost::asio::ip::tcp::acceptor acceptor_;
    
    std::mutex handlersMutex_;
    std::unordered_map<std::string, RequestHandler> getHandlers_;
    std::unordered_map<std::string, RequestHandler> postHandlers_;
    std::unordered_map<std::string, RequestHandler> putHandlers_;
    std::unordered_map<std::string, RequestHandler> deleteHandlers_;

    MiddlewareStack middlewareStack_;
    std::atomic<int> activeConnections_{0};
    static constexpr int maxConnections_{10000};

    // Add views folder path
    std::string viewsFolder_;
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_SERVER_H