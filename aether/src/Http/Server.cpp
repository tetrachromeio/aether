// Server.cpp
#include "Aether/Http/Server.h"
#include "Aether/Http/Connection.h"

namespace Aether {
namespace Http {

Server::Server()
    : eventLoop_(),
      acceptor_(eventLoop_.getIoContext()) {
    eventLoop_.start();
}

Server::~Server() {
    if (acceptor_.is_open()) acceptor_.close();
}

void Server::use(Middleware middleware) {
    middlewareStack_.use(middleware);
}

void Server::get(const std::string& path, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    getHandlers_.push_back({RoutePattern(path), handler});
}

void Server::post(const std::string& path, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    postHandlers_.push_back({RoutePattern(path), handler});
}

void Server::put(const std::string& path, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    putHandlers_.push_back({RoutePattern(path), handler});
}

void Server::del(const std::string& path, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    deleteHandlers_.push_back({RoutePattern(path), handler});
}

void Server::views(const std::string& folder) {
    viewsFolder_ = folder;
    Response::viewsFolder_ = folder;
}

void Server::run(int port) {
    try {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
        acceptor_.open(endpoint.protocol());
        acceptor_.bind(endpoint);
        acceptor_.listen();
        startAccept();
        eventLoop_.keepAlive(); // Block here
    } catch (const std::exception& e) {
        // Handle error
    }
}

RequestHandler Server::findHandler(const std::string& method, const std::string& path, Request& req) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    
    const auto& routes = [&]() -> const std::vector<Route>& {
        if (method == "GET") return getHandlers_;
        if (method == "POST") return postHandlers_;
        if (method == "PUT") return putHandlers_;
        return deleteHandlers_;
    }();

    for (const auto& route : routes) {
        req.params.clear();
        if (route.pattern.match(path, req.params)) {
            return route.handler;
        }
    }
    return nullptr;
}

void Server::startAccept() {
    if (activeConnections_ >= maxConnections_) return;

    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(eventLoop_.getIoContext());
    
    acceptor_.async_accept(*socket,
        [this, socket](const boost::system::error_code& error) {
            handleNewConnection(error, socket);
            startAccept();
        });
}

void Server::handleNewConnection(
    const boost::system::error_code& error,
    std::shared_ptr<boost::asio::ip::tcp::socket> socket
) {
    if (!error && activeConnections_ < maxConnections_) {
        ++activeConnections_;
        std::make_shared<Connection>(
            std::move(*socket),
            [this](auto&& method, auto&& path, auto&& req) { 
                return findHandler(method, path, req); 
            },
            middlewareStack_,
            [this] { --activeConnections_; }
        )->start();
    } else if (socket->is_open()) {
        socket->close();
    }
}

} // namespace Http
} // namespace Aether