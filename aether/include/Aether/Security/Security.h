#ifndef AETHER_SECURITY_SECURITY_H
#define AETHER_SECURITY_SECURITY_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>

namespace Aether {
namespace Security {

// Security headers manager
class SecurityHeaders {
public:
    static void addSecurityHeaders(std::unordered_map<std::string, std::string>& headers);
    static void setStrictTransportSecurity(const std::string& value);
    static void setContentSecurityPolicy(const std::string& policy);
    static void setFrameOptions(const std::string& options);
    static void enableXSSProtection(bool enable);
    static void enableContentTypeNoSniff(bool enable);
    static void setReferrerPolicy(const std::string& policy);
    
private:
    static std::unordered_map<std::string, std::string> defaultHeaders_;
    static std::mutex headersMutex_;
};

// Input validation and sanitization
class InputValidator {
public:
    // SQL injection prevention
    static bool containsSQLInjection(const std::string& input);
    static std::string sanitizeSQL(const std::string& input);
    
    // XSS prevention
    static bool containsXSS(const std::string& input);
    static std::string sanitizeHTML(const std::string& input);
    
    // Path traversal prevention
    static bool containsPathTraversal(const std::string& path);
    static std::string sanitizePath(const std::string& path);
    
    // General input validation
    static bool isValidEmail(const std::string& email);
    static bool isValidURL(const std::string& url);
    static bool isAlphanumeric(const std::string& input);
    static bool matchesPattern(const std::string& input, const std::string& pattern);
    
    // Length and size validation
    static bool validateLength(const std::string& input, size_t minLen, size_t maxLen);
    static bool validateRequestSize(size_t contentLength, size_t maxSize);
};

// Rate limiting
class RateLimiter {
public:
    struct ClientInfo {
        std::chrono::steady_clock::time_point lastRequest;
        int requestCount;
        bool blocked;
    };
    
    RateLimiter(int maxRequests, std::chrono::seconds windowSize);
    
    bool allowRequest(const std::string& clientId);
    void blockClient(const std::string& clientId, std::chrono::seconds duration);
    void unblockClient(const std::string& clientId);
    bool isBlocked(const std::string& clientId);
    
    void cleanup(); // Remove old entries
    
private:
    int maxRequests_;
    std::chrono::seconds windowSize_;
    std::unordered_map<std::string, ClientInfo> clients_;
    std::mutex clientsMutex_;
};

// Security audit logger
class SecurityAudit {
public:
    enum class EventType {
        SUSPICIOUS_REQUEST,
        RATE_LIMIT_EXCEEDED,
        SQL_INJECTION_ATTEMPT,
        XSS_ATTEMPT,
        PATH_TRAVERSAL_ATTEMPT,
        INVALID_INPUT,
        AUTHENTICATION_FAILURE,
        AUTHORIZATION_FAILURE
    };
    
    static void logSecurityEvent(EventType type, const std::string& clientId, 
                                const std::string& details);
    static void logSuspiciousActivity(const std::string& clientId, 
                                    const std::string& activity);
    
private:
    static std::string eventTypeToString(EventType type);
};

// CORS (Cross-Origin Resource Sharing) handler
class CORSHandler {
public:
    void addAllowedOrigin(const std::string& origin);
    void addAllowedMethod(const std::string& method);
    void addAllowedHeader(const std::string& header);
    void setMaxAge(int seconds);
    void setAllowCredentials(bool allow);
    
    std::unordered_map<std::string, std::string> generateCORSHeaders(
        const std::string& origin, const std::string& requestMethod) const;
    
    bool isOriginAllowed(const std::string& origin) const;
    bool isMethodAllowed(const std::string& method) const;
    
private:
    std::unordered_set<std::string> allowedOrigins_;
    std::unordered_set<std::string> allowedMethods_;
    std::unordered_set<std::string> allowedHeaders_;
    int maxAge_ = 86400; // 24 hours
    bool allowCredentials_ = false;
    mutable std::mutex corsMutex_;
};

// SSL/TLS configuration
class SSLConfig {
public:
    struct SSLOptions {
        std::string certFile;
        std::string keyFile;
        std::string caFile;
        std::string cipherSuite;
        bool requireClientCert = false;
        bool verifyPeer = true;
        int minTLSVersion = 12; // TLS 1.2
    };
    
    static bool validateSSLFiles(const SSLOptions& options);
    static std::string getRecommendedCipherSuite();
    static bool isSecureTLSVersion(int version);
};

} // namespace Security
} // namespace Aether

#endif // AETHER_SECURITY_SECURITY_H
