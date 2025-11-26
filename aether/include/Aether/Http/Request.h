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
    // we need req.getURL();
    std::string getUrl() const {
        return path + (params.empty() ? "" : "?" + paramsToString());
    }
    // we need a way to ger the domain
    std::string getDomain() const {
        // should it ger from headers or be set separately?
        auto it = headers.find("host");
        if (it != headers.end()) {
            return it->second;
        }
        // Fallback for older parsers that kept casing
        it = headers.find("Host");
        if (it != headers.end()) {
            return it->second;
        }
        return ""; // Default case if Host header is not found
    }

    
    private:
        std::string paramsToString() const {
            std::string result;
            for (auto it = params.begin(); it != params.end(); ++it) {
                if (it != params.begin()) {
                    result += "&";
                }
                result += it->first + "=" + it->second;
            }
            return result;
        }

};

} // namespace Http
} // namespace Aether

#endif // Aether_HTTP_REQUEST_H
