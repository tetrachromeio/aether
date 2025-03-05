// Aether/Http/RoutePattern.h
#ifndef AETHER_HTTP_ROUTEPATTERN_H
#define AETHER_HTTP_ROUTEPATTERN_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

namespace Aether {
namespace Http {

class RoutePattern {
public:
    struct Segment {
        enum Type { Static, Param, Wildcard };
        Type type;
        std::string value;
        std::regex regex;
    };

    RoutePattern(const std::string& pattern);
    bool match(const std::string& path, std::unordered_map<std::string, std::string>& params) const;

private:
    void parsePattern(const std::string& pattern);
    std::vector<Segment> segments_;
};

} // namespace Http
} // namespace Aether

#endif // AETHER_HTTP_ROUTEPATTERN_H