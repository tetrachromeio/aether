// Response.h
#ifndef AETHER_HTTP_RESPONSE_H
#define AETHER_HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace Aether {
namespace Http {

struct Response {
    void send(const std::string& body, int statusCode = 200) {
        this->body = body;
        this->statusCode = statusCode;
    }

    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }

    void render(const std::string& viewName) {
        std::string filePath = viewsFolder_ + "/" + viewName + ".html";
        std::ifstream file(filePath);
        if (file.is_open()) {
            std::ostringstream buffer;
            buffer << file.rdbuf();
            send(buffer.str());
        } else {
            send("404 Not Found", 404);
        }
    }

    std::string body;
    int statusCode = 200; // Default to 200 OK
    std::unordered_map<std::string, std::string> headers;

    // Declare the static member variable
    static std::string viewsFolder_;
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_RESPONSE_H