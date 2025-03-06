#include "Aether/Http/HttpParser.h"
#include <sstream>
#include <algorithm>
#include <vector>

namespace Aether {
namespace Http {

// Map status codes to reason phrases
std::string HttpParser::statusText(int statusCode) {
    static const std::unordered_map<int, std::string> texts = {
        {200, "OK"},
        {400, "Bad Request"},
        {404, "Not Found"},
        {500, "Internal Server Error"}
    };
    return texts.count(statusCode) ? texts.at(statusCode) : "Unknown";
}

bool HttpParser::parseRequest(const std::string& rawRequest, Request& req) {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) return false;

    // Parse headers and body from raw string
    if (!parseHeaders(rawRequest.substr(0, headerEnd), req)) {
        return false;
    }
    parseBody(rawRequest.substr(headerEnd + 4), req);
    return true;
}

bool HttpParser::parseHeaders(const std::string& headerBlock, Request& req) {
    std::istringstream stream(headerBlock);
    std::string line;
    
    // First line is request line
    if (std::getline(stream, line)) {
        if (!parseStartLine(line, req)) {
            return false;
        }
    }

    // Validate HTTP method
    static const std::vector<std::string> validMethods = {"GET", "POST", "PUT", "DELETE"};
    if (std::find(validMethods.begin(), validMethods.end(), req.method) == validMethods.end()) {
        return false;
    }

    // Validate HTTP version
    if (req.version != "HTTP/1.0" && req.version != "HTTP/1.1") {
        return false;
    }

    // Process headers
    while (std::getline(stream, line)) {
        if (line.empty() || line == "\r") break;
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            value.erase(0, value.find_first_not_of(' '));
            req.headers[key] = value;
        }
    }

    return true;
}

void HttpParser::parseBody(const std::string& bodyContent, Request& req) {
    req.body = bodyContent;
}

bool HttpParser::parseStartLine(const std::string& line, Request& req) {
    std::istringstream iss(line);
    return static_cast<bool>(iss >> req.method >> req.path >> req.version);
}

} // namespace Http
} // namespace Aether