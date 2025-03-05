#include "Aether/aether.h"
#include <websocket.h>

// Built-in modules
using namespace Aether; // Bring aether into scope
using namespace Aether::Http; // Bring aether's built-in HTTP module into scope

// External modules
using namespace Websocket; // External module

int main() {
    print("Starting Aether server..."); // Use print directly

    // Create the HTTP server
    auto app = Server();

    // Create the WebSocket server, passing the HTTP server
    auto ws = SocketServer(app);

    // WebSocket event handler
    ws.on("message", [](Session session, const Message& message) {
        print("Received message: " + message);
        session->send("echo", message); // Send event "echo" and the message
    });

    // Start the WebSocket server on port 8080
    ws.listen(8080);

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