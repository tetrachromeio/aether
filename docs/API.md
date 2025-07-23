# Aether Framework API Documentation

## Table of Contents

1. [Core Components](#core-components)
2. [HTTP Server](#http-server)
3. [Request & Response](#request--response)
4. [Middleware](#middleware)
5. [Routing](#routing)
6. [Configuration](#configuration)
7. [Logging](#logging)
8. [Security](#security)
9. [Performance](#performance)
10. [Examples](#examples)

---

## Core Components

### EventLoop

The `EventLoop` class manages the asynchronous I/O operations using Boost.Asio.

```cpp
#include "Aether/Core/EventLoop.h"

namespace Aether {

class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    
    void start();
    void stop();
    void post(std::function<void()> task);
    void keepAlive();
    
    boost::asio::io_context& getIoContext();
};

}
```

**Methods:**
- `start()` - Starts the event loop
- `stop()` - Stops the event loop
- `post(task)` - Posts a task to be executed asynchronously
- `keepAlive()` - Keeps the event loop running (blocks)
- `getIoContext()` - Returns reference to the underlying io_context

### Logger

Thread-safe logging system with configurable levels and outputs.

```cpp
#include "Aether/Core/Logger.h"

namespace Aether::Core {

enum class LogLevel {
    DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4
};

class Logger {
public:
    static Logger& getInstance();
    
    void setLevel(LogLevel level);
    void setOutputFile(const std::string& filename);
    void enableConsoleOutput(bool enable);
    void enableTimestamps(bool enable);
    void enableThreadId(bool enable);
    
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warn(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);
    void fatal(const std::string& message, const std::string& file = "", int line = 0);
};

}
```

**Convenience Macros:**
```cpp
AETHER_LOG_DEBUG("Debug message");
AETHER_LOG_INFO("Info message");
AETHER_LOG_WARN("Warning message");
AETHER_LOG_ERROR("Error message");
AETHER_LOG_FATAL("Fatal error");
```

### Configuration

Type-safe configuration management with multiple sources.

```cpp
#include "Aether/Core/Config.h"

namespace Aether::Core {

class Config {
public:
    static Config& getInstance();
    
    bool loadFromFile(const std::string& filename);
    bool loadFromJson(const std::string& jsonContent);
    bool loadFromEnv(const std::string& prefix = "AETHER_");
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const;
    
    void set(const std::string& key, const ConfigValue& value);
    bool has(const std::string& key) const;
    
    class Section {
    public:
        template<typename T>
        T get(const std::string& key, const T& defaultValue = T{}) const;
        void set(const std::string& key, const ConfigValue& value);
        bool has(const std::string& key) const;
    };
    
    Section getSection(const std::string& sectionName);
};

}
```

---

## HTTP Server

### Server

The main HTTP server class that handles incoming connections and routes requests.

```cpp
#include "Aether/Http/Server.h"

namespace Aether::Http {

class Server {
public:
    Server();
    ~Server();
    
    // HTTP methods
    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void put(const std::string& path, RequestHandler handler);
    void del(const std::string& path, RequestHandler handler);
    
    // Middleware
    void use(Middleware middleware);
    
    // Configuration
    void views(const std::string& folder);
    
    // Start server
    void run(int port);
    
    // Access to io_context for advanced usage
    boost::asio::io_context& ioContext();
};

}
```

**Example Usage:**
```cpp
#include "Aether/aether.h"

using namespace Aether::Http;

int main() {
    Server server;
    
    server.get("/", [](Request& req, Response& res) {
        res.send("Hello, World!");
    });
    
    server.get("/users/:id", [](Request& req, Response& res) {
        std::string userId = req.params["id"];
        res.json({{"userId", userId}});
    });
    
    server.run(3000);
    return 0;
}
```

---

## Request & Response

### Request

Represents an HTTP request with headers, parameters, and body.

```cpp
namespace Aether::Http {

struct Request {
    std::string method;          // HTTP method (GET, POST, etc.)
    std::string path;            // Request path
    std::string query;           // Query string
    std::string body;            // Request body
    std::string httpVersion;     // HTTP version
    
    std::unordered_map<std::string, std::string> headers;  // HTTP headers
    std::unordered_map<std::string, std::string> params;   // Route parameters
    std::unordered_map<std::string, std::string> queryParams; // Query parameters
    
    // Helper methods
    std::string getHeader(const std::string& name) const;
    bool hasHeader(const std::string& name) const;
    std::string getQueryParam(const std::string& name) const;
    std::string getParam(const std::string& name) const;
};

}
```

### Response

Represents an HTTP response with fluent API for building responses.

```cpp
namespace Aether::Http {

class Response {
public:
    // Status code
    Response& status(int code);
    
    // Headers
    Response& setHeader(const std::string& name, const std::string& value);
    Response& removeHeader(const std::string& name);
    
    // Content
    void send(const std::string& content);
    void json(const std::unordered_map<std::string, std::string>& data);
    void sendFile(const std::string& filePath);
    
    // Redirects
    void redirect(const std::string& url, int code = 302);
    
    // Cookies
    Response& cookie(const std::string& name, const std::string& value, 
                    const CookieOptions& options = {});
    Response& clearCookie(const std::string& name);
};

}
```

**Example Usage:**
```cpp
server.get("/api/user", [](Request& req, Response& res) {
    res.status(200)
       .setHeader("Content-Type", "application/json")
       .json({{"name", "John"}, {"age", "30"}});
});

server.get("/download", [](Request& req, Response& res) {
    res.setHeader("Content-Disposition", "attachment; filename=data.txt")
       .sendFile("./data.txt");
});
```

---

## Middleware

### Middleware System

Aether uses a flexible middleware system for request processing.

```cpp
namespace Aether::Http {

using Middleware = std::function<void(Request&, Response&, Context&, 
                                    std::function<void(std::exception_ptr)>)>;

using ErrorMiddleware = std::function<void(std::exception_ptr, Request&, Response&, 
                                         Context&, std::function<void()>)>;

class MiddlewareStack {
public:
    void use(Middleware middleware);
    void useError(ErrorMiddleware errorMiddleware);
    void run(Request& req, Response& res, std::function<void()> next);
};

}
```

**Built-in Middleware:**

#### Static File Serving
```cpp
#include "Aether/Middleware/ServeStatic.h"

server.use(Aether::Http::serveStatic("./public"));
```

#### Security Headers
```cpp
#include "Aether/Security/Security.h"

server.use([](Request& req, Response& res, Context& ctx, auto next) {
    Aether::Security::SecurityHeaders::addSecurityHeaders(res.headers);
    next(nullptr);
});
```

#### Custom Middleware Example
```cpp
// Logging middleware
server.use([](Request& req, Response& res, Context& ctx, auto next) {
    AETHER_LOG_INFO("Request: " + req.method + " " + req.path);
    next(nullptr);
});

// Authentication middleware
server.use([](Request& req, Response& res, Context& ctx, auto next) {
    if (!req.hasHeader("Authorization")) {
        res.status(401).send("Unauthorized");
        return;
    }
    next(nullptr);
});
```

---

## Routing

### Route Patterns

Aether supports flexible routing with parameters and wildcards.

```cpp
namespace Aether::Http {

class RoutePattern {
public:
    RoutePattern(const std::string& pattern);
    bool match(const std::string& path, 
               std::unordered_map<std::string, std::string>& params) const;
};

}
```

**Supported Patterns:**
- Static routes: `/users`, `/api/v1/status`
- Parameters: `/users/:id`, `/posts/:category/:slug`
- Regex parameters: `/users/:id(\\d+)`, `/files/:name(.*\\.txt)`
- Wildcards: `/static/*`, `/api/v1/*`

**Examples:**
```cpp
// Static route
server.get("/about", handler);

// Parameter route
server.get("/users/:id", [](Request& req, Response& res) {
    std::string id = req.params["id"];
    res.send("User ID: " + id);
});

// Multiple parameters
server.get("/posts/:category/:slug", [](Request& req, Response& res) {
    std::string category = req.params["category"];
    std::string slug = req.params["slug"];
    res.json({{"category", category}, {"slug", slug}});
});

// Regex parameter (digits only)
server.get("/users/:id(\\d+)", [](Request& req, Response& res) {
    int id = std::stoi(req.params["id"]);
    res.send("User ID: " + std::to_string(id));
});

// Wildcard route
server.get("/files/*", [](Request& req, Response& res) {
    std::string path = req.path.substr(7); // Remove "/files/"
    res.sendFile("./uploads/" + path);
});
```

---

## Security

### Security Features

Comprehensive security features for production applications.

```cpp
#include "Aether/Security/Security.h"

namespace Aether::Security {

// Security headers
class SecurityHeaders {
public:
    static void addSecurityHeaders(std::unordered_map<std::string, std::string>& headers);
    static void setStrictTransportSecurity(const std::string& value);
    static void setContentSecurityPolicy(const std::string& policy);
    static void setFrameOptions(const std::string& options);
};

// Input validation
class InputValidator {
public:
    static bool containsSQLInjection(const std::string& input);
    static bool containsXSS(const std::string& input);
    static bool containsPathTraversal(const std::string& path);
    static std::string sanitizeHTML(const std::string& input);
    static bool isValidEmail(const std::string& email);
};

// Rate limiting
class RateLimiter {
public:
    RateLimiter(int maxRequests, std::chrono::seconds windowSize);
    bool allowRequest(const std::string& clientId);
    void blockClient(const std::string& clientId, std::chrono::seconds duration);
};

// CORS handling
class CORSHandler {
public:
    void addAllowedOrigin(const std::string& origin);
    void addAllowedMethod(const std::string& method);
    std::unordered_map<std::string, std::string> generateCORSHeaders(
        const std::string& origin, const std::string& requestMethod) const;
};

}
```

**Security Example:**
```cpp
// Set up security middleware
server.use([](Request& req, Response& res, Context& ctx, auto next) {
    // Add security headers
    SecurityHeaders::addSecurityHeaders(res.headers);
    
    // Validate input
    if (InputValidator::containsXSS(req.body)) {
        res.status(400).send("Invalid input");
        return;
    }
    
    next(nullptr);
});

// Rate limiting
auto rateLimiter = std::make_shared<RateLimiter>(100, std::chrono::seconds(60));

server.use([rateLimiter](Request& req, Response& res, Context& ctx, auto next) {
    std::string clientId = req.getHeader("X-Forwarded-For");
    if (clientId.empty()) {
        clientId = "unknown";
    }
    
    if (!rateLimiter->allowRequest(clientId)) {
        res.status(429).send("Rate limit exceeded");
        return;
    }
    
    next(nullptr);
});
```

---

## Performance

### Performance Monitoring

Built-in performance monitoring and optimization tools.

```cpp
#include "Aether/Performance/Metrics.h"

namespace Aether::Performance {

class MetricsCollector {
public:
    static MetricsCollector& getInstance();
    
    void incrementRequestCount();
    void recordRequestDuration(double durationMs);
    void recordResponseSize(size_t bytes);
    
    std::string generateReport() const;
};

// Profiling
class Profiler {
public:
    static Profiler& getInstance();
    void startProfiling(const std::string& name);
    void endProfiling(const std::string& name);
};

}
```

**Performance Macros:**
```cpp
// Automatic profiling
AETHER_PROFILE("request_handler");

// Automatic timing
AETHER_TIMER("database_query");
```

**Performance Example:**
```cpp
server.get("/api/data", [](Request& req, Response& res) {
    AETHER_PROFILE("api_data_handler");
    
    auto& metrics = MetricsCollector::getInstance();
    Timer timer;
    timer.start();
    
    // Process request
    std::string data = processData(req);
    
    timer.stop();
    metrics.recordRequestDuration(timer.getElapsedMilliseconds());
    metrics.recordResponseSize(data.size());
    
    res.json({{"data", data}});
});
```

---

## Examples

### Complete Server Example

```cpp
#include "Aether/aether.h"
#include "Aether/Security/Security.h"
#include "Aether/Middleware/ServeStatic.h"

using namespace Aether;
using namespace Aether::Http;

int main() {
    // Configure logging
    auto& logger = Core::Logger::getInstance();
    logger.setLevel(Core::LogLevel::INFO);
    logger.setOutputFile("server.log");
    
    // Configure application
    auto& config = Core::Config::getInstance();
    config.loadFromFile("config.json");
    config.loadFromEnv("MYAPP_");
    
    Server server;
    
    // Security middleware
    server.use([](Request& req, Response& res, Context& ctx, auto next) {
        Security::SecurityHeaders::addSecurityHeaders(res.headers);
        next(nullptr);
    });
    
    // Static file serving
    server.use(serveStatic("./public"));
    
    // API routes
    server.get("/api/health", [](Request& req, Response& res) {
        res.json({{"status", "healthy"}, {"timestamp", "2025-01-01T00:00:00Z"}});
    });
    
    server.get("/api/users/:id", [](Request& req, Response& res) {
        std::string userId = req.params["id"];
        
        // Validate input
        if (!Security::InputValidator::isAlphanumeric(userId)) {
            res.status(400).json({{"error", "Invalid user ID"}});
            return;
        }
        
        res.json({{"userId", userId}, {"name", "John Doe"}});
    });
    
    server.post("/api/users", [](Request& req, Response& res) {
        AETHER_PROFILE("create_user");
        
        // Process user creation
        AETHER_LOG_INFO("Creating new user");
        
        res.status(201).json({{"message", "User created successfully"}});
    });
    
    // Start server
    int port = config.get<int>("PORT", 3000);
    AETHER_LOG_INFO("Starting server on port " + std::to_string(port));
    
    server.run(port);
    
    return 0;
}
```

### WebSocket Integration Example

```cpp
#include "Aether/aether.h"
#include "websocket.h" // Custom WebSocket package

int main() {
    Server httpServer;
    Websocket::SocketServer wsServer(httpServer);
    
    // HTTP routes
    httpServer.get("/", [](Request& req, Response& res) {
        res.sendFile("./public/index.html");
    });
    
    // WebSocket events
    wsServer.on("message", [](auto session, const std::string& message) {
        std::cout << "Received: " << message << std::endl;
        session->send("echo", "Echo: " + message);
    });
    
    wsServer.on("join_room", [&wsServer](auto session, const std::string& room) {
        wsServer.broadcast("user_joined", "User joined room: " + room);
    });
    
    // Start both servers
    std::thread wsThread([&wsServer]() {
        wsServer.listen(8080);
    });
    
    httpServer.run(3000);
    
    wsThread.join();
    return 0;
}
```

---

## Best Practices

### Error Handling
```cpp
server.use([](Request& req, Response& res, Context& ctx, auto next) {
    try {
        next(nullptr);
    } catch (const std::exception& e) {
        AETHER_LOG_ERROR("Request error: " + std::string(e.what()));
        res.status(500).json({{"error", "Internal server error"}});
    }
});
```

### Resource Management
```cpp
// Use RAII for automatic cleanup
class DatabaseConnection {
public:
    DatabaseConnection() { /* acquire connection */ }
    ~DatabaseConnection() { /* release connection */ }
};

server.get("/api/data", [](Request& req, Response& res) {
    DatabaseConnection db; // Automatic cleanup
    auto data = db.query("SELECT * FROM users");
    res.json(data);
});
```

### Performance Optimization
```cpp
// Use connection pooling for database connections
// Use memory pools for frequent allocations
// Profile critical paths
// Monitor metrics in production
```
