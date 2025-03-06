#ifndef AETHER_HTTP_PARSER_H
#define AETHER_HTTP_PARSER_H

#include "Request.h"
#include <string>
#include <unordered_map>

namespace Aether {
namespace Http {

class HttpParser {
public:
    // Public methods for parsing
    static bool parseRequest(const std::string& rawRequest, Request& req);
    static std::string statusText(int statusCode);

private:
    // Internal parsing helpers
    static bool parseStartLine(const std::string& line, Request& req);
    static bool parseHeaders(const std::string& headerBlock, Request& req); // Updated to return bool
    static void parseBody(const std::string& bodyContent, Request& req);
};

} // namespace Http
} // namespace Aether

#endif // AETHER_HTTP_PARSER_H