#ifndef AETHER_HTTP_MIDDLEWARE_H
#define AETHER_HTTP_MIDDLEWARE_H

#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include <functional>
#include <vector>

namespace Aether {
namespace Http {

using Middleware = std::function<void(Request&, Response&, std::function<void()>)>;

class MiddlewareStack {
public:
    void use(Middleware middleware);
    void run(Request& req, Response& res, std::function<void()> next);

private:
    std::vector<Middleware> middlewares_;
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_MIDDLEWARE_H