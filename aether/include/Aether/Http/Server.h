// Server.h
#ifndef AETHER_HTTP_SERVER_H
#define AETHER_HTTP_SERVER_H

#include "Aether/Core/EventLoop.h"
#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include "Aether/Http/Middleware.h"
#include "Aether/Http/RoutePattern.h"
#include "Aether/NeuralDb/NeuralDbServer.h"
#include <boost/asio.hpp>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>

namespace Aether {
namespace Http {

class Connection; // Forward declaration
class Router; // Forward declaration

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

    // Start NeuralDB protocol listener
    void neural(int port = 7654);

    // Add views method
    void views(const std::string& folder);

private:
    struct Route {
        RoutePattern pattern;
        RequestHandler handler;
    };

    void startAccept();
    void handleNewConnection(
        const boost::system::error_code& error,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket
    );

    RequestHandler findHandler(const std::string& method, const std::string& path, Request& req);

    EventLoop eventLoop_;
    boost::asio::ip::tcp::acceptor acceptor_;
    
    std::mutex handlersMutex_;
    std::vector<Route> getHandlers_;
    std::vector<Route> postHandlers_;
    std::vector<Route> putHandlers_;
    std::vector<Route> deleteHandlers_;

    MiddlewareStack middlewareStack_;
    std::atomic<int> activeConnections_{0};
    static constexpr int maxConnections_{10000};

    // Add views folder path
    std::string viewsFolder_;

    // NeuralDB server instance
    std::unique_ptr<Aether::NeuralDb::NeuralDbServer> neuraldbServer_;
    std::thread neuraldbThread_;
    std::atomic<bool> neuraldbRunning_{false};
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_SERVER_H