#ifndef AETHER_MIDDLEWARE_SERVESTATIC_H
#define AETHER_MIDDLEWARE_SERVESTATIC_H

#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include "Aether/Http/Middleware.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Aether {
namespace Http {

/**
 * Middleware to serve static files from a specified directory.
 * @param basePath The base directory from which to serve static files.
 * @return A middleware function that serves static files.
 */
Middleware serveStatic(const std::string& basePath);

} // namespace Http
} // namespace Aether

#endif // CHROMATE_MIDDLEWARE_SERVESTATIC_H