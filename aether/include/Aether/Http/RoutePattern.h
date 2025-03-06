// File: Aether/Http/RoutePattern.h
#ifndef AETHER_HTTP_ROUTEPATTERN_H
#define AETHER_HTTP_ROUTEPATTERN_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <memory>

namespace Aether {
namespace Http {

class RoutePattern {
public:
    // Represents a segment of a route pattern
    struct Segment {
        enum Type { Static, Param, Wildcard }; // Types of segments
        Type type;                             // Type of the segment
        std::string value;                     // Value of the segment (e.g., "users", ":id", "*")
        std::regex regex;                      // Regex for parameter validation (if applicable)
    };

    // Constructor: Parses the route pattern
    RoutePattern(const std::string& pattern);

    // Matches a path against the route pattern and extracts parameters
    bool match(const std::string& path, std::unordered_map<std::string, std::string>& params) const;

private:
    // Parses the route pattern into segments
    void parsePattern(const std::string& pattern);

    // Stores the segments of the route pattern
    std::vector<Segment> segments_;
};

// Represents a group of routes with a common prefix
class RouteGroup {
public:
    // Constructor: Initializes the group with a prefix
    RouteGroup(const std::string& prefix);

    // Adds a route pattern to the group
    void addRoute(const std::string& pattern, std::shared_ptr<RoutePattern> route);

    // Matches a path against the group's routes and extracts parameters
    bool match(const std::string& path, std::unordered_map<std::string, std::string>& params) const;

private:
    // The common prefix for the group (e.g., "/api/v1")
    std::string prefix_;

    // Stores the route patterns in the group
    std::vector<std::pair<std::string, std::shared_ptr<RoutePattern>>> routes_;
};

} // namespace Http
} // namespace Aether

#endif // AETHER_HTTP_ROUTEPATTERN_H