#include "Aether/Http/Middleware.h"

namespace Aether {
namespace Http {

void MiddlewareStack::use(Middleware middleware) {
    middlewares_.push_back(middleware);
}

void MiddlewareStack::useError(ErrorMiddleware errorMiddleware) {
    errorMiddlewares_.push_back(errorMiddleware);
}

void MiddlewareStack::run(Request& req, Response& res, std::function<void()> next) {
    size_t index = 0;
    Context context; // Shared context for all middleware
    std::exception_ptr error = nullptr;

    // Function to run error middleware
    std::function<void()> runErrorHandlers = [&]() {
        for (auto& errorMiddleware : errorMiddlewares_) {
            errorMiddleware(error, req, res, context, next);
        }
    };

    // Function to run middleware sequentially
    std::function<void(std::exception_ptr)> runNext = [&](std::exception_ptr err) {
        if (err) {
            error = err; // Store error and trigger error middleware
            runErrorHandlers();
            return;
        }

        if (index < middlewares_.size()) {
            auto& middleware = middlewares_[index++];
            try {
                middleware(req, res, context, runNext);
            } catch (...) {
                runNext(std::current_exception());
            }
        } else {
            next(); // Call final handler
        }
    };

    // Start middleware chain
    runNext(nullptr);
}

} // namespace Http
} // namespace Aether
