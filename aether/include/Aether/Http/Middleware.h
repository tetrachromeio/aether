#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <any>
#include "Request.h"
#include "Response.h"

namespace Aether {
namespace Http {

// Context class to store middleware-specific data
class Context {
public:
    template <typename T>
    void set(const std::string& key, const T& value) {
        data_[key] = value;
    }

    template <typename T>
    T get(const std::string& key) const {
        return std::any_cast<T>(data_.at(key));
    }

    bool has(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

private:
    std::unordered_map<std::string, std::any> data_;
};

using Middleware = std::function<void(Request&, Response&, Context&, std::function<void(std::exception_ptr)>)>;
using ErrorMiddleware = std::function<void(std::exception_ptr, Request&, Response&, Context&, std::function<void()>)>;

class MiddlewareStack {
public:
    void use(Middleware middleware);
    void useError(ErrorMiddleware errorMiddleware);
    void run(Request& req, Response& res, std::function<void()> next);

private:
    std::vector<Middleware> middlewares_;
    std::vector<ErrorMiddleware> errorMiddlewares_;
};

} // namespace Http
} // namespace Aether
