#include "Aether/Http/Router.h"
#include "Aether/Http/Server.h"

namespace Aether {
namespace Http {

void Router::get(const std::string& route, std::function<void(Request&, Response&)> handler) {
    app.get(basePath + route, handler);
}

void Router::post(const std::string& route, std::function<void(Request&, Response&)> handler) {
    app.post(basePath + route, handler);
}

void Router::put(const std::string& route, std::function<void(Request&, Response&)> handler) {
    app.put(basePath + route, handler);
}

void Router::del(const std::string& route, std::function<void(Request&, Response&)> handler) {
    app.del(basePath + route, handler);
}

} // namespace Http
} // namespace Aether
