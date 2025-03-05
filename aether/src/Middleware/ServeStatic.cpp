#include "Aether/Middleware/ServeStatic.h"
#include <iostream> // Ensure this is included

namespace Aether {
namespace Http {

Middleware serveStatic(const std::string& basePath) {
    return [basePath](Request& req, Response& res, std::function<void()> next) {


        // Remove leading '/' from req.path
        std::string requestedPath = req.path;
        if (!requestedPath.empty() && requestedPath[0] == '/') {
            requestedPath.erase(0, 1);
        }

        // Construct the full file path
        std::filesystem::path filePath = std::filesystem::path(basePath) / requestedPath;

       

        // Check if the file exists and is a regular file
        if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
           

            // Read the file content
            std::ifstream file(filePath, std::ios::binary);
            if (file) {
                std::ostringstream buffer;
                buffer << file.rdbuf();
                std::string fileContent = buffer.str();

                // Set the appropriate content type
                std::string contentType = "text/plain"; // Default content type
                std::string extension = filePath.extension().string();
                if (extension == ".html") {
                    contentType = "text/html";
                } else if (extension == ".css") {
                    contentType = "text/css";
                } else if (extension == ".js") {
                    contentType = "application/javascript";
                } else if (extension == ".json") {
                    contentType = "application/json";
                } else if (extension == ".png") {
                    contentType = "image/png";
                } else if (extension == ".jpg" || extension == ".jpeg") {
                    contentType = "image/jpeg";
                }

                // Send the file content as the response
                res.setHeader("Content-Type", contentType);
                res.send(fileContent);
                return; // Stop further middleware processing
            }
        }
        next();
    };
}

} // namespace Http
} // namespace Aether