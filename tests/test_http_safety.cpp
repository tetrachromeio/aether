#include "Aether/Http/Response.h"
#include "Aether/Http/Request.h"
#include "Aether/Middleware/ServeStatic.h"
#include "Aether/Http/HttpParser.h"
#include <filesystem>
#include <fstream>
#include <cassert>

using namespace Aether::Http;

int main() {
    // Prepare a temporary directory for static files and templates
    const auto tempDir = std::filesystem::temp_directory_path() / "aether_http_safety";
    std::filesystem::create_directories(tempDir);

    // Static file setup
    {
        std::ofstream(tempDir / "index.html") << "hello";
        std::ofstream(tempDir / "secret.txt") << "secret";
    }

    // ServeStatic should serve normal files and reject traversal
    {
        Request req;
        Response res;
        Context ctx;
        auto middleware = serveStatic(tempDir.string());

        bool nextCalled = false;
        req.path = "/index.html";
        middleware(req, res, ctx, [&](std::exception_ptr) { nextCalled = true; });
        assert(res.body == "hello");
        assert(nextCalled == false);

        // Attempt traversal
        nextCalled = false;
        res = Response{};
        req.path = "/../secret.txt";
        middleware(req, res, ctx, [&](std::exception_ptr) { nextCalled = true; });
        assert(nextCalled == true);
        assert(res.body.empty());
    }

    // Response::render should escape HTML and block traversal
    {
        std::ofstream(tempDir / "template.html") << "<div>{{user}}</div>";
        Response::viewsFolder_ = tempDir.string();

        Response res;
        res.render("template", nlohmann::json{{"user", "<script>alert(1)</script>"}});
        assert(res.body.find("&lt;script&gt;") != std::string::npos);

        res = Response{};
        res.render("../secret", {});
        assert(res.statusCode == 404);
    }

    // Keep-alive negotiation respects Connection header semantics
    {
        Request req;
        req.version = "HTTP/1.1";
        assert(Connection::wantsKeepAlive(req) == true);

        req.headers["connection"] = "close";
        assert(Connection::wantsKeepAlive(req) == false);

        req.version = "HTTP/1.0";
        req.headers["connection"] = "keep-alive";
        assert(Connection::wantsKeepAlive(req) == true);
    }

    // Header parsing normalizes casing
    {
        std::string raw =
            "GET / HTTP/1.1\r\n"
            "Host: Example.com\r\n"
            "Connection: CLOSE\r\n"
            "\r\n";
        Request req;
        assert(HttpParser::parseRequest(raw, req));
        assert(req.headers.count("host") == 1);
        assert(req.headers.at("host") == "Example.com");
        assert(Connection::wantsKeepAlive(req) == false);
    }

    // Body limit guard rails
    {
        assert(Connection::exceedsBodyLimit(0, Connection::kMaxBodySizeBytes - 1) == false);
        assert(Connection::exceedsBodyLimit(Connection::kMaxBodySizeBytes, 1) == true);
        assert(Connection::exceedsBodyLimit(1024, Connection::kMaxBodySizeBytes) == true);
    }

    std::filesystem::remove_all(tempDir);
    return 0;
}
