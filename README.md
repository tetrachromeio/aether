

# <img src="/figures/Aether_logo.png" height="64"> <img src="/figures/Aether_txt.png" height="64">

# **Aether — High-Performance, Developer-Friendly HTTP Framework for Modern C++**

### **v0.2.0 • Rebuilt Core • ~126k Requests/Second**

> **Aether** merges the familiar ergonomics of frameworks like Express.js
> with the raw, uncompromising performance of modern C++.

Built on **Boost.Asio**. Designed for real servers. Tuned for clarity, stability, and speed.

---

# 🚀 Introduction

Aether is a modern C++ HTTP framework aiming to be:

* **Fast like C++**
* **Simple like Node.js**
* **Modern like Rust**
* **Practical like Go**

It provides a clean developer experience without hiding what makes C++ powerful.

```cpp
server.get("/hello", [](Request& req, Response& res) {
    res.send("Hello from Aether!");
});
```

Aether focuses on correctness, performance, and code that is easy to read, reason about, and contribute to.

---

# ⚡ Performance (v0.2.0)

Benchmarks were run using:

**wrk -t12 -c400 -d30s [http://localhost:3000/](http://localhost:3000/)**

### **Hardware**

**MacBook Air (15-inch, 2025)**

* **Chip:** Apple M4
* **Cores:** 10-core hybrid CPU
* **Memory:** 16 GB unified
* **OS:** macOS Tahoe **26.1**
* **Compiler:** Apple Clang (LLVM)
* **Build Flags:**

  ```
  -O3 -flto -march=armv8.5-a
  ```

### **Aether v0.2.0**

```
Requests/sec: 126,295.16
Avg latency:   3.15ms
Max latency:   110ms
```

---

## 📊 Framework Comparison (Same Machine)

| Framework        | Requests/sec | Notes                         |
| ---------------- | ------------ | ----------------------------- |
| **Aether (C++)** | **126k**     | Your framework 🚀             |
| **Drogon (C++)** | 143k         | Very mature, highly optimized |
| **Go (gnet)**    | 77k          | Event-driven Go               |
| **Node.js HTTP** | 72k          | v18 built-in server           |

### Summary

* **1.63× faster than Go gnet**
* **1.75× faster than Node.js**
* **Within 10–15% of Drogon**, while offering a simpler development model

### Text Visualization

```
Requests/sec
Node.js     ████████▏  (72k)
Go gnet     ████████████▍  (77k)
Aether      ████████████████████████████▎  (126k)
Drogon      ████████████████████████████████ (143k)
```

Aether is now legitimately in the **elite C++ framework tier**.

---

# 🌱 Philosophy

Aether is built on three simple principles:

### **1. Developer experience matters.**

C++ networking libraries shouldn’t require enormous boilerplate or deep ASIO knowledge.
Aether offers:

* Express-style routing
* Clean request/response lifecycle
* Middleware
* Minimal friction

### **2. Performance should be inherent, not something you chase.**

Aether internally uses:

* Zero-copy buffer handling
* Efficient string_view parsing
* Multi-threaded io_context
* No unnecessary heap allocations
* Stable keep-alive lifecycle

Performance comes from design, not hacks.

### **3. Correctness > Benchmark Tricks**

Aether includes:

* Full header parsing
* Chunked transfer decoding
* Content-Length enforcement
* Proper connection reuse
* Clean error handling

All real-world requirements — not disabled for benchmarks.

---

# 🧠 Architecture Overview

### **EventLoop**

Thread pool wrapping a shared `boost::asio::io_context`.
Automatically scales with hardware concurrency.

### **Connection**

Full HTTP/1.1 pipeline:

* async read until `\r\n\r\n`
* request line + header parsing
* body decoding (Content-Length + chunked)
* middleware chain
* route dispatch
* async write
* keep-alive support

### **HttpParser**

Modern, fast, low-allocation parser using:

* `std::string_view`
* lowercased header keys
* minimal copying
* predictable behavior

### **MiddlewareStack**

Chain-based middleware identical in spirit to Express:

```cpp
app.use([](Request& req, Response& res, Next next) {
    // inspect or modify req/res
    next();
});
```

---

# ✨ Features (v0.2.0)

### Implemented

* HTTP/1.1
* GET / POST / PUT / DELETE
* Middleware system
* Static file serving
* Parameter parsing
* Content-Length + chunked bodies
* Routing table
* Thread-scaled event loop
* Keep-alive connection reuse
* Zero-copy buffer strategy

### Roadmap

* [ ] HTTP/2
* [ ] WebSockets
* [ ] TLS/SSL
* [ ] Streaming bodies
* [ ] JSON helpers
* [ ] Better router with patterns
* [ ] Observability (logging / tracing)
* [ ] Aeon package ecosystem

---

# 🛠 Getting Started

### Basic Server

```cpp
#include <Aether/aether.h>

using namespace Aether;
using namespace Aether::Http;

int main() {
    Server app;

    app.get("/", [](auto& req, auto& res) {
        res.send("Hello from Aether!");
    });

    app.use(serveStatic("./public"));

    app.run(3000);
}
```

---

# 📦 Aeon Package Manager (Experimental)

Aether ships with an emerging dependency manager:

```
aeon install websocket@1.0.0
```

Goals:

* cross-platform binary packages
* semantic versioning
* optimized C++ ecosystem
* header-only support

---

# ⚠️ Limitations

* Early-stage project
* API may refine as v1.0 approaches
* No TLS or HTTP/2 yet
* Linux not fully profiled
* Limited documentation

---

# 🤝 Contributing

We welcome all contributions:

* Parser improvements
* HTTP/2 implementation
* Middleware helpers
* Static file optimizations
* Test suites
* Documentation

**Steps:**

1. Fork
2. Create feature branch
3. Open PR

First-time contributors are welcome.

---

# 🌟 Why Aether Exists

C++ has fast frameworks — but very few that are:

* elegant
* expressive
* easy to learn
* async-first
* modern in architecture
* portable
* fun

Aether aims to become that missing framework:
**a clean, modern API with industrial-strength performance.**

---

# ❤️ Join the Journey

```cpp
app.post("/contribute", [](auto& req, auto& res) {
    res.send("Welcome to the Aether community!");
});
```

Aether v0.2.0 already pushes **~126k requests/sec** on an M4 MacBook Air.
And this is only the beginning.


