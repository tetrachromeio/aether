#include "Aether/Security/Security.h"
#include "Aether/Core/Logger.h"
#include <regex>
#include <algorithm>
#include <fstream>

namespace Aether {
namespace Security {

// SecurityHeaders implementation
std::unordered_map<std::string, std::string> SecurityHeaders::defaultHeaders_;
std::mutex SecurityHeaders::headersMutex_;

void SecurityHeaders::addSecurityHeaders(std::unordered_map<std::string, std::string>& headers) {
    std::lock_guard<std::mutex> lock(headersMutex_);
    
    // Default security headers
    if (defaultHeaders_.empty()) {
        defaultHeaders_["X-Content-Type-Options"] = "nosniff";
        defaultHeaders_["X-Frame-Options"] = "DENY";
        defaultHeaders_["X-XSS-Protection"] = "1; mode=block";
        defaultHeaders_["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains";
        defaultHeaders_["Referrer-Policy"] = "strict-origin-when-cross-origin";
        defaultHeaders_["Content-Security-Policy"] = "default-src 'self'";
    }
    
    for (const auto& header : defaultHeaders_) {
        headers[header.first] = header.second;
    }
}

void SecurityHeaders::setStrictTransportSecurity(const std::string& value) {
    std::lock_guard<std::mutex> lock(headersMutex_);
    defaultHeaders_["Strict-Transport-Security"] = value;
}

void SecurityHeaders::setContentSecurityPolicy(const std::string& policy) {
    std::lock_guard<std::mutex> lock(headersMutex_);
    defaultHeaders_["Content-Security-Policy"] = policy;
}

void SecurityHeaders::setFrameOptions(const std::string& options) {
    std::lock_guard<std::mutex> lock(headersMutex_);
    defaultHeaders_["X-Frame-Options"] = options;
}

void SecurityHeaders::enableXSSProtection(bool enable) {
    std::lock_guard<std::mutex> lock(headersMutex_);
    if (enable) {
        defaultHeaders_["X-XSS-Protection"] = "1; mode=block";
    } else {
        defaultHeaders_.erase("X-XSS-Protection");
    }
}

void SecurityHeaders::enableContentTypeNoSniff(bool enable) {
    std::lock_guard<std::mutex> lock(headersMutex_);
    if (enable) {
        defaultHeaders_["X-Content-Type-Options"] = "nosniff";
    } else {
        defaultHeaders_.erase("X-Content-Type-Options");
    }
}

void SecurityHeaders::setReferrerPolicy(const std::string& policy) {
    std::lock_guard<std::mutex> lock(headersMutex_);
    defaultHeaders_["Referrer-Policy"] = policy;
}

// InputValidator implementation
bool InputValidator::containsSQLInjection(const std::string& input) {
    // Common SQL injection patterns
    std::vector<std::regex> sqlPatterns = {
        std::regex(R"(\b(union|select|insert|update|delete|drop|create|alter|exec|execute)\b)", std::regex::icase),
        std::regex(R"((\-\-|#|\/\*|\*\/))", std::regex::icase),
        std::regex(R"((\b(or|and)\s+\d+=\d+))", std::regex::icase),
        std::regex(R"((\'\s*(or|and)\s*\'\s*=\s*\'))", std::regex::icase)
    };
    
    for (const auto& pattern : sqlPatterns) {
        if (std::regex_search(input, pattern)) {
            return true;
        }
    }
    return false;
}

std::string InputValidator::sanitizeSQL(const std::string& input) {
    std::string sanitized = input;
    
    // Escape single quotes
    size_t pos = 0;
    while ((pos = sanitized.find("'", pos)) != std::string::npos) {
        sanitized.replace(pos, 1, "''");
        pos += 2;
    }
    
    return sanitized;
}

bool InputValidator::containsXSS(const std::string& input) {
    std::vector<std::regex> xssPatterns = {
        std::regex(R"(<\s*script)", std::regex::icase),
        std::regex(R"(javascript\s*:)", std::regex::icase),
        std::regex(R"(on\w+\s*=)", std::regex::icase),
        std::regex(R"(<\s*iframe)", std::regex::icase),
        std::regex(R"(<\s*object)", std::regex::icase),
        std::regex(R"(<\s*embed)", std::regex::icase)
    };
    
    for (const auto& pattern : xssPatterns) {
        if (std::regex_search(input, pattern)) {
            return true;
        }
    }
    return false;
}

std::string InputValidator::sanitizeHTML(const std::string& input) {
    std::string sanitized = input;
    
    // HTML entity encoding for dangerous characters
    std::unordered_map<char, std::string> entities = {
        {'<', "&lt;"}, {'>', "&gt;"}, {'&', "&amp;"}, 
        {'"', "&quot;"}, {'\'', "&#x27;"}, {'/', "&#x2F;"}
    };
    
    for (const auto& entity : entities) {
        size_t pos = 0;
        while ((pos = sanitized.find(entity.first, pos)) != std::string::npos) {
            sanitized.replace(pos, 1, entity.second);
            pos += entity.second.length();
        }
    }
    
    return sanitized;
}

bool InputValidator::containsPathTraversal(const std::string& path) {
    return path.find("..") != std::string::npos || 
           path.find("./") != std::string::npos ||
           path.find("\\") != std::string::npos;
}

std::string InputValidator::sanitizePath(const std::string& path) {
    std::string sanitized = path;
    
    // Remove dangerous path components
    std::vector<std::string> dangerous = {"../", "./", "..", "\\"};
    for (const auto& danger : dangerous) {
        size_t pos = 0;
        while ((pos = sanitized.find(danger, pos)) != std::string::npos) {
            sanitized.erase(pos, danger.length());
        }
    }
    
    // Ensure path starts with /
    if (!sanitized.empty() && sanitized[0] != '/') {
        sanitized = "/" + sanitized;
    }
    
    return sanitized;
}

bool InputValidator::isValidEmail(const std::string& email) {
    std::regex emailPattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, emailPattern);
}

bool InputValidator::isValidURL(const std::string& url) {
    std::regex urlPattern(R"(^https?://[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}(/.*)?$)");
    return std::regex_match(url, urlPattern);
}

bool InputValidator::isAlphanumeric(const std::string& input) {
    return std::all_of(input.begin(), input.end(), [](char c) {
        return std::isalnum(c);
    });
}

bool InputValidator::matchesPattern(const std::string& input, const std::string& pattern) {
    try {
        std::regex regex(pattern);
        return std::regex_match(input, regex);
    } catch (const std::regex_error&) {
        return false;
    }
}

bool InputValidator::validateLength(const std::string& input, size_t minLen, size_t maxLen) {
    return input.length() >= minLen && input.length() <= maxLen;
}

bool InputValidator::validateRequestSize(size_t contentLength, size_t maxSize) {
    return contentLength <= maxSize;
}

// RateLimiter implementation
RateLimiter::RateLimiter(int maxRequests, std::chrono::seconds windowSize)
    : maxRequests_(maxRequests), windowSize_(windowSize) {
}

bool RateLimiter::allowRequest(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto& client = clients_[clientId];
    
    // Check if client is blocked
    if (client.blocked) {
        return false;
    }
    
    // Reset if window has passed
    if (now - client.lastRequest > windowSize_) {
        client.requestCount = 0;
    }
    
    client.requestCount++;
    client.lastRequest = now;
    
    if (client.requestCount > maxRequests_) {
        SecurityAudit::logSecurityEvent(SecurityAudit::EventType::RATE_LIMIT_EXCEEDED, 
                                       clientId, "Request rate exceeded");
        return false;
    }
    
    return true;
}

void RateLimiter::blockClient(const std::string& clientId, std::chrono::seconds duration) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    clients_[clientId].blocked = true;
    
    // Note: In a production system, you'd want to implement automatic unblocking after duration
    AETHER_LOG_WARN("Blocked client: " + clientId);
}

void RateLimiter::unblockClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
        it->second.blocked = false;
        AETHER_LOG_INFO("Unblocked client: " + clientId);
    }
}

bool RateLimiter::isBlocked(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    return it != clients_.end() && it->second.blocked;
}

void RateLimiter::cleanup() {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto now = std::chrono::steady_clock::now();
    
    auto it = clients_.begin();
    while (it != clients_.end()) {
        if (now - it->second.lastRequest > windowSize_ * 2) {
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

// SecurityAudit implementation
void SecurityAudit::logSecurityEvent(EventType type, const std::string& clientId, 
                                   const std::string& details) {
    std::string message = "SECURITY_EVENT [" + eventTypeToString(type) + "] " +
                         "Client: " + clientId + " Details: " + details;
    AETHER_LOG_WARN(message);
}

void SecurityAudit::logSuspiciousActivity(const std::string& clientId, 
                                        const std::string& activity) {
    std::string message = "SUSPICIOUS_ACTIVITY Client: " + clientId + " Activity: " + activity;
    AETHER_LOG_WARN(message);
}

std::string SecurityAudit::eventTypeToString(EventType type) {
    switch (type) {
        case EventType::SUSPICIOUS_REQUEST: return "SUSPICIOUS_REQUEST";
        case EventType::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
        case EventType::SQL_INJECTION_ATTEMPT: return "SQL_INJECTION_ATTEMPT";
        case EventType::XSS_ATTEMPT: return "XSS_ATTEMPT";
        case EventType::PATH_TRAVERSAL_ATTEMPT: return "PATH_TRAVERSAL_ATTEMPT";
        case EventType::INVALID_INPUT: return "INVALID_INPUT";
        case EventType::AUTHENTICATION_FAILURE: return "AUTHENTICATION_FAILURE";
        case EventType::AUTHORIZATION_FAILURE: return "AUTHORIZATION_FAILURE";
        default: return "UNKNOWN";
    }
}

// CORSHandler implementation
void CORSHandler::addAllowedOrigin(const std::string& origin) {
    std::lock_guard<std::mutex> lock(corsMutex_);
    allowedOrigins_.insert(origin);
}

void CORSHandler::addAllowedMethod(const std::string& method) {
    std::lock_guard<std::mutex> lock(corsMutex_);
    allowedMethods_.insert(method);
}

void CORSHandler::addAllowedHeader(const std::string& header) {
    std::lock_guard<std::mutex> lock(corsMutex_);
    allowedHeaders_.insert(header);
}

void CORSHandler::setMaxAge(int seconds) {
    maxAge_ = seconds;
}

void CORSHandler::setAllowCredentials(bool allow) {
    allowCredentials_ = allow;
}

std::unordered_map<std::string, std::string> CORSHandler::generateCORSHeaders(
    const std::string& origin, const std::string& requestMethod) const {
    
    std::lock_guard<std::mutex> lock(corsMutex_);
    std::unordered_map<std::string, std::string> headers;
    
    if (isOriginAllowed(origin)) {
        headers["Access-Control-Allow-Origin"] = origin;
        
        if (allowCredentials_) {
            headers["Access-Control-Allow-Credentials"] = "true";
        }
        
        if (!allowedMethods_.empty()) {
            std::string methods;
            for (const auto& method : allowedMethods_) {
                if (!methods.empty()) methods += ", ";
                methods += method;
            }
            headers["Access-Control-Allow-Methods"] = methods;
        }
        
        if (!allowedHeaders_.empty()) {
            std::string headersStr;
            for (const auto& header : allowedHeaders_) {
                if (!headersStr.empty()) headersStr += ", ";
                headersStr += header;
            }
            headers["Access-Control-Allow-Headers"] = headersStr;
        }
        
        headers["Access-Control-Max-Age"] = std::to_string(maxAge_);
    }
    
    return headers;
}

bool CORSHandler::isOriginAllowed(const std::string& origin) const {
    return allowedOrigins_.count("*") > 0 || allowedOrigins_.count(origin) > 0;
}

bool CORSHandler::isMethodAllowed(const std::string& method) const {
    return allowedMethods_.empty() || allowedMethods_.count(method) > 0;
}

// SSLConfig implementation
bool SSLConfig::validateSSLFiles(const SSLOptions& options) {
    // Check if certificate file exists and is readable
    std::ifstream certFile(options.certFile);
    if (!certFile.good()) {
        AETHER_LOG_ERROR("SSL certificate file not found or not readable: " + options.certFile);
        return false;
    }
    
    // Check if key file exists and is readable
    std::ifstream keyFile(options.keyFile);
    if (!keyFile.good()) {
        AETHER_LOG_ERROR("SSL key file not found or not readable: " + options.keyFile);
        return false;
    }
    
    // Check CA file if specified
    if (!options.caFile.empty()) {
        std::ifstream caFile(options.caFile);
        if (!caFile.good()) {
            AETHER_LOG_ERROR("SSL CA file not found or not readable: " + options.caFile);
            return false;
        }
    }
    
    return true;
}

std::string SSLConfig::getRecommendedCipherSuite() {
    return "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:"
           "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
           "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
           "DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384";
}

bool SSLConfig::isSecureTLSVersion(int version) {
    return version >= 12; // TLS 1.2 or higher
}

} // namespace Security
} // namespace Aether
