
#include "Aether/aether.h"

using namespace Aether; // Bring aeon into scope
using namespace Aether::Http;

int main() {
    print("Starting aeon server..."); // Use print directly

    auto app = Server();

    // GET request handler
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

    app.run(3000); // Start the server on port 3000
    
    return 0;
}
