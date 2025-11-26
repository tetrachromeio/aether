#ifndef AETHER_HTTP_PARSER_H
#define AETHER_HTTP_PARSER_H

#include "Request.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace Aether {
namespace Http {

class HttpParser {
public:
    // Public methods for parsing
    // Convenience overload: keeps old signature
    static bool parseRequest(const std::string& rawRequest, Request& req);

    // New, more efficient overload using string_view
    static bool parseRequest(std::string_view rawRequest, Request& req);

    static std::string statusText(int statusCode);

private:
    // Internal parsing helpers (string_view-based)
    static bool parseStartLine(std::string_view line, Request& req);
    static bool parseHeaders(std::string_view headerBlock, Request& req);
    static void parseBody(std::string_view bodyContent, Request& req);
};

} // namespace Http
} // namespace Aether

#endif // AETHER_HTTP_PARSER_H
