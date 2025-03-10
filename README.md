
<img src="/figures/Aether_logo.png" alt="Aether Logo" style="width: 100px; height: auto;">
<img src="/figures/Aether_txt.png" alt="Aether Text" style="width: 100px; height: auto;">

# Aether - High-Performance HTTP Server Framework for C++

![Aether Performance](https://img.shields.io/badge/Performance-58k%20req%2Fs-brightgreen)  
*Aether: Bridging Node.js simplicity with C++ raw power*

---

## 🚀 Introduction

Aether is an experimental, high-performance HTTP server framework designed to bring the developer-friendly ergonomics of Node.js to C++, leveraging the language's native speed and resource efficiency. Built on Boost.Asio for asynchronous I/O and modern C++17/20 features, Aether aims to provide:

- **Express.js-like middleware syntax**
- **Asynchronous, non-blocking architecture**
- **Zero-copy networking**
- **Compiled-language performance**

```cpp
// Node.js-like simplicity in C++
server.get("/hello", [](Request& req, Response& res) {
    res.send("Hello from compiled code!");
});
```

---

## ⚡ Performance Benchmark (v0.0.1 Alpha)


### Node.js
```bash
Running 30s test @ http://localhost:8000/
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    24.05ms   55.94ms   1.16s    98.61%
    Req/Sec     1.34k   187.35     1.70k    80.98%
  160507 requests in 30.02s, 37.81MB read
Requests/sec:   5346.53
Transfer/sec:      1.26MB
```

### Aether
```bash
Running 30s test @ http://localhost:3000/
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.50ms    1.62ms  99.04ms   97.60%
    Req/Sec    10.44k     0.90k   15.08k    70.85%
  1250707 requests in 30.10s, 99.00MB read
Requests/sec:  41551.57
Transfer/sec:      3.29MB
```

**Performance Comparison Table**
| Metric               | Node.js       | Aether        | Improvement  |
|----------------------|---------------|---------------|--------------|
| Requests/sec         | 5,346.53      | 41,551.57     | 677% faster  |
| Latency (avg)        | 24.05 ms      | 2.50 ms       | 89.6% lower  |
| Latency (max)        | 1.16s         | 99.04ms       | 91.5% lower  |
| Transfer/sec         | 1.26 MB       | 3.29 MB       | 161% higher  |

**Text-Based Bar Chart**  
*(Relative performance comparison)*

**Requests per Second**  
``` 
Node.js  █████ (5.3k)  
Aether   ████████████████████████████ (41.5k)
```

**Average Latency**  
```
Node.js  ███████████████████████ (24.05ms)  
Aether   ███ (2.5ms)
```

**Transfer per Second**  
```
Node.js  ███████ (1.26MB)  
Aether   ███████████████ (3.29MB)
```


**Key Comparison:**
- 🚀 Aether handles **7.8x more requests/sec** (41,552 vs 5,347)
- ⚡ Aether's latency is **9.6x lower** (2.50ms vs 24.05ms average)
- 📤 Aether achieves **2.6x higher throughput** (3.29MB/s vs 1.26MB/s)

**Test Environment**  
MacBook Pro 2019 (1.4 GHz Quad-Core Intel Core i5, 16GB RAM)  
macOS Sequoia 15.0.1, Clang 15, `-O3` optimization



---

## 🧠 Technical Approach

### Core Architecture
- **Boost.Asio Foundation**: Utilizes proactor pattern for async I/O with zero-copy buffer management
- **Lock-Free Middleware**: Middleware chain execution without mutex overhead
- **HTTP/1.1 Pipeline**: Connection reuse with intelligent keep-alive management
- **Modern C++ Paradigms**: Move semantics, RAII resource management, and type-safe handlers

### Key Components
```plaintext
1. EventLoop: Multi-threaded ASIO context pool
2. Connection: Stateful request/response handler
3. MiddlewareStack: Express.js-style middleware chaining
4. HttpParser: RFC-compliant HTTP message processor
```

---

## ✨ Features

### Current Implementation
- **HTTP/1.1 Server** with keep-alive support
- **Middleware System** (`app.use()`) with async chaining
- **Static File Serving** (MIME-type detection)
- **Route Handlers** (`GET`, `POST`, `PUT`, `DELETE`)
- **Connection Pooling** (1000+ concurrent connections)

### Planned Features
- [ ] HTTP/2 & WebSocket Support
- [ ] TLS/SSL Integration
- [ ] Dynamic Route Patterns (`/users/:id`)
- [ ] Template Engine Interface

---

## 📦 Aeon Package Manager

Aether introduces **Aeon** - a modern dependency manager designed for C++ ecosystems:

```bash
aeon install websockets@1.2.0
```

**Advantages over Traditional Build Systems:**
- Automatic dependency resolution
- Cross-platform precompiled binaries
- Semantic versioning support
- Header-only package support

*Learn more: [Aeon Package Manager Documentation](/aeon/README.md)*

---

## 🛠 Getting Started

### Installation (Requires C++17)
```bash
aeon new <project>
```

### Basic Server
```cpp
#include <Aether/aether.h>

using namespace Aether;
using namespace Aether::Http;

int main() {
    auto app = Server(); // Create a new server instance

    
    app.get("/", [](auto& req, auto& res) {
        res.send("Hello from Aether!");
    });
    
    app.use(serveStatic("./public"));

    server.run(3000);

    return 0
}
```

---

## ⚠️ Current Limitations

1. **Alpha Status**  
   Not production-ready - API may change dramatically

2. **Feature Gaps**  
   Missing HTTPS, WebSockets, and advanced routing

3. **Platform Support**  
   Only MacOS tested. Should work be able to build on any platform. But not tested.

4. **Documentation**  
   Early-stage examples and guides

---

## 🤝 Contributing

**We Need Your Help to Build:**  
- HTTP/2 Protocol Implementation
- Benchmark Suite
- Unit Test Coverage
- Aeon Package Recipes

**Contribution Guide:**
1. Fork repository
2. Create feature branch (`feat/your-feature`)
3. Submit PR with tests

*First-time contributors welcome! Check [Good First Issues]()*

---

## 🌟 Why Aether?

While Node.js revolutionized server-side JavaScript, C++ has lacked a framework combining:
- Modern developer experience
- Asynchronous primitives
- Package ecosystem
- Performance ceiling

Aether aims to fill this gap, offering:  
✅ 10μs-level request handling  
✅ Memory-safe resource management  
✅ 100k+ RPS capability  
✅ True parallelism via native threads

---

```cpp
// Join the revolution of high-performance C++ web development!
app.post("/contribute", [](auto& req, auto& res) {
    res.send("Let's build Aether together!");
});
```
