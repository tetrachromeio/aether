#include "Aether/Http/RoutePattern.h"
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace Aether {
namespace Http {

RoutePattern::RoutePattern(const std::string& pattern) {
    parsePattern(pattern);
}

void RoutePattern::parsePattern(const std::string& pattern) {
    std::istringstream iss(pattern);
    std::string segment;
    
    while (std::getline(iss, segment, '/')) {
        if (segment.empty()) continue;

        if (segment[0] == ':') {
            size_t regexStart = segment.find('(');
            if (regexStart != std::string::npos) {
                if (segment.back() != ')') {
                    throw std::invalid_argument("Invalid route pattern: " + pattern);
                }
                std::string name = segment.substr(1, regexStart - 1);
                std::string regexStr = segment.substr(regexStart + 1, segment.size() - regexStart - 2);
                segments_.push_back({Segment::Param, name, std::regex(regexStr)});
            } else {
                segments_.push_back({Segment::Param, segment.substr(1), std::regex()});
            }
        } else if (segment[0] == '*' && segment.size() > 1) {
            segments_.push_back({Segment::Wildcard, segment.substr(1), std::regex()});
        } else if (segment == "*") {
            segments_.push_back({Segment::Wildcard, "*", std::regex()});
        } else {
            segments_.push_back({Segment::Static, segment, std::regex()});
        }
    }
}

bool RoutePattern::match(const std::string& path, 
                         std::unordered_map<std::string, std::string>& params) const {
    std::istringstream iss(path);
    std::vector<std::string> pathSegments;
    std::string segment;
    
    while (std::getline(iss, segment, '/')) {
        if (!segment.empty()) pathSegments.push_back(segment);
    }

    size_t pi = 0;
    for (size_t si = 0; si < segments_.size(); ++si) {
        const auto& seg = segments_[si];
        
        if (seg.type == Segment::Wildcard) {
            std::string value;
            for (; pi < pathSegments.size(); ++pi) {
                if (!value.empty()) value += "/";
                value += pathSegments[pi];
            }
            params[seg.value] = value; // Store the wildcard match in params
            return true;
        }

        if (pi >= pathSegments.size()) return false;

        if (seg.type == Segment::Static) {
            if (pathSegments[pi] != seg.value) return false;
        } else if (seg.type == Segment::Param) {
            params[seg.value] = pathSegments[pi];
        }

        ++pi;
    }
    return pi == pathSegments.size();
}

} // namespace Http
} // namespace Aether
