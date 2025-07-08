// Router.h
#ifndef AETHER_HTTP_ROUTER_H
#define AETHER_HTTP_ROUTER_H

#include "Request.h"
#include "Response.h"
#include <string>
#include <functional>

namespace Aether {
namespace Http {

// Forward declaration
class Server;

class Router {
private:
    Server& app;
    std::string basePath;

public:
    Router(const std::string& path, Server& server) : basePath(path), app(server) {}

    void get(const std::string& route, std::function<void(Request&, Response&)> handler);
    void post(const std::string& route, std::function<void(Request&, Response&)> handler);
    void put(const std::string& route, std::function<void(Request&, Response&)> handler);
    void del(const std::string& route, std::function<void(Request&, Response&)> handler);
};

} // namespace Http
} // namespace Aether

#endif // AETHER_HTTP_ROUTER_H
