#include "Aether/Http/Middleware.h"

namespace Aether {
namespace Http {

void MiddlewareStack::use(Middleware middleware) {
    middlewares_.push_back(middleware);
}

void MiddlewareStack::run(Request& req, Response& res, std::function<void()> next) {
    size_t index = 0;

    // Define a recursive function to chain middleware
    std::function<void()> runNext = [&]() {
        if (index < middlewares_.size()) {
            auto& middleware = middlewares_[index++];
            middleware(req, res, runNext);
        } else {
            next(); // Call the final handler (route handler)
        }
    };

    // Start the middleware chain
    runNext();
}

} // namespace Http
} // namespace Aether