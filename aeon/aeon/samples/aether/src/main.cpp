#include "Aether/aether.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

// Built-in modules
using namespace Aether; // Bring aether into scope
using namespace Aether::Http; // Bring aether's built-in HTTP module into scope

// Helper function to compute SHA256 hash using OpenSSL
std::string sha256_hash(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int main() {
    print("Starting Aether server with OpenSSL and NeuralDB integration...");

    // Create the HTTP/NeuralDB server
    auto app = Server();

    // HTTP request handlers
    app.get("/", [](Request& req, Response& res) {
        print("Handling GET request for /");
        res.send("Hello from aeon with OpenSSL support!");
    });

    app.get("/users/:id", [](auto& req, auto& res) {
        res.send("User ID: " + req.params["id"]);
    });

    app.get("/hash/:text", [](auto& req, auto& res) {
        std::string text = req.params["text"];
        std::string hash = sha256_hash(text);
        res.send("SHA256 hash of '" + text + "': " + hash);
    });

    app.get("/files/*path", [](auto& req, auto& res) {
        res.sendFile("/Volumes/external/Package/aether/sample/public/" + req.params["path"]);
    });

    // Start the NeuralDB protocol listener on port 7654 (default)
    app.neural(7654);

    // Start the HTTP server on port 3000
    app.run(3000);

    return 0;
}