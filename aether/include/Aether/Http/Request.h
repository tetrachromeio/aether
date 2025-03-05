#ifndef AETHER_HTTP_REQUEST_H
#define AETHER_HTTP_REQUEST_H

#include <string>
#include <unordered_map>

namespace Aether {
namespace Http {

struct Request {
    std::string method;  // GET/POST/PUT/DELETE
    std::string path;    // Requested URL path
    std::string version; // HTTP version (e.g., "HTTP/1.1")
    std::unordered_map<std::string, std::string> headers; // Request headers
    std::unordered_map<std::string, std::string> params; // URL parameters
    std::string body; // Request body
};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_REQUEST_H