#include "Aether/aether.h"


// Built-in modules
using namespace Aether; // Bring aether into scope
using namespace Aether::Http; // Bring aether's built-in HTTP module into scope



int main() {
    print("Starting Aether server..."); // Use print directly

    // Create the HTTP server
    auto app = Server();

    // HTTP request handlers
    app.get("/", [](Request& req, Response& res) {
        print("Handling GET request for /"); // Use print directly
        res.send("Hello from aeon!");
    });

    app.get("/users/:id", [](auto& req, auto& res) {
        res.send("User ID: " + req.params["id"]);
    });

    app.get("/files/*path", [](auto& req, auto& res) {
        res.sendFile("/Volumes/external/Package/aether/sample/public/" + req.params["path"]);
    });

    // Start the HTTP server on port 3000
    app.run(3000);

    return 0;
}