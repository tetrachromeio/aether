#include "Aether/Http/HttpParser.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace Aether {
namespace Http {

namespace {

// Trim leading/trailing whitespace (space, tab, CR, LF) from a string_view
inline std::string_view trim(std::string_view sv) {
    // leading
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' ||
                           sv.front() == '\r' || sv.front() == '\n')) {
        sv.remove_prefix(1);
    }
    // trailing
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' ||
                           sv.back() == '\r' || sv.back() == '\n')) {
        sv.remove_suffix(1);
    }
    return sv;
}

} // anonymous namespace

// Map status codes to reason phrases
std::string HttpParser::statusText(int statusCode) {
    static const std::unordered_map<int, std::string> texts = {
        {200, "OK"},
        {201, "Created"},
        {204, "No Content"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {413, "Payload Too Large"},
        {431, "Request Header Fields Too Large"},
        {500, "Internal Server Error"}
    };

    auto it = texts.find(statusCode);
    return (it != texts.end()) ? it->second : "Unknown";
}

// Legacy overload: keep compatibility, forward to string_view version
bool HttpParser::parseRequest(const std::string& rawRequest, Request& req) {
    return parseRequest(std::string_view(rawRequest.data(), rawRequest.size()), req);
}

bool HttpParser::parseRequest(std::string_view rawRequest, Request& req) {
    // Locate end of headers: "\r\n\r\n"
    const std::string_view delimiter = "\r\n\r\n";
    std::size_t headerEnd = rawRequest.find(delimiter);
    if (headerEnd == std::string_view::npos) {
        return false; // incomplete request
    }

    // Split header block and body
    std::string_view headerBlock = rawRequest.substr(0, headerEnd);
    std::string_view bodyBlock = rawRequest.substr(headerEnd + delimiter.size());

    // First line: request line up to first "\r\n"
    std::size_t lineEnd = headerBlock.find("\r\n");
    if (lineEnd == std::string_view::npos) {
        return false;
    }

    std::string_view startLine = headerBlock.substr(0, lineEnd);
    std::string_view headerLines;
    if (lineEnd + 2 <= headerBlock.size()) {
        headerLines = headerBlock.substr(lineEnd + 2);
    }

    // Parse request line (method, path, version)
    if (!parseStartLine(startLine, req)) {
        return false;
    }

    // Validate HTTP method (same as your original set)
    static const std::vector<std::string> validMethods = {"GET", "POST", "PUT", "DELETE"};
    bool methodOk = false;
    for (const auto& m : validMethods) {
        if (req.method == m) {
            methodOk = true;
            break;
        }
    }
    if (!methodOk) {
        return false;
    }

    // Validate HTTP version
    if (req.version != "HTTP/1.0" && req.version != "HTTP/1.1") {
        return false;
    }

    // Parse headers into req.headers
    if (!parseHeaders(headerLines, req)) {
        return false;
    }

    // Parse body into req.body
    parseBody(bodyBlock, req);
    return true;
}

bool HttpParser::parseStartLine(std::string_view line, Request& req) {
    // Expected: "<METHOD> <PATH> <VERSION>\r\n"
    line = trim(line);
    if (line.empty()) return false;

    // Find first space (end of method)
    std::size_t sp1 = line.find(' ');
    if (sp1 == std::string_view::npos) return false;

    // Find second space (end of path)
    std::size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == std::string_view::npos) return false;

    std::string_view method = line.substr(0, sp1);
    std::string_view path   = line.substr(sp1 + 1, sp2 - (sp1 + 1));
    std::string_view ver    = line.substr(sp2 + 1);

    method = trim(method);
    path   = trim(path);
    ver    = trim(ver);

    // Store as std::string in Request
    req.method.assign(method.begin(), method.end());
    req.path.assign(path.begin(), path.end());
    req.version.assign(ver.begin(), ver.end());

    return true;
}

bool HttpParser::parseHeaders(std::string_view headerBlock, Request& req) {
    req.headers.clear();

    std::size_t pos = 0;
    const std::size_t len = headerBlock.size();

    while (pos < len) {
        // Find end of line
        std::size_t lineEnd = headerBlock.find("\r\n", pos);
        if (lineEnd == std::string_view::npos) {
            lineEnd = len;
        }

        std::string_view line = headerBlock.substr(pos, lineEnd - pos);
        line = trim(line);

        // Empty line: end of headers
        if (line.empty()) {
            break;
        }

        // Find colon separating key and value
        std::size_t colon = line.find(':');
        if (colon != std::string_view::npos) {
            std::string_view keyView = line.substr(0, colon);
            std::string_view valueView = line.substr(colon + 1);

            keyView = trim(keyView);
            valueView = trim(valueView);

            // Normalize header key to lowercase for lookups (as you had)
            std::string key;
            key.reserve(keyView.size());
            for (char c : keyView) {
                key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }

            std::string value(valueView.begin(), valueView.end());
            req.headers[std::move(key)] = std::move(value);
        }

        // Advance to next line
        if (lineEnd + 2 > len) {
            break;
        }
        pos = lineEnd + 2;
    }

    return true;
}

void HttpParser::parseBody(std::string_view bodyContent, Request& req) {
    // For now just store as-is in Request::body
    req.body.assign(bodyContent.begin(), bodyContent.end());
}

} // namespace Http
} // namespace Aether
