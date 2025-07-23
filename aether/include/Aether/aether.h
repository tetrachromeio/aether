#ifndef AEATHER_H
#define AEATHER_H

// Core components
#include "Aether/Core/EventLoop.h"
#include "Aether/Core/Print.h"
#include "Aether/Core/Config.h"
#include "Aether/Core/Logger.h"
#include "Aether/Core/json.hpp"


// HTTP components
#include "Aether/Http/Server.h"
#include "Aether/Http/Request.h"
#include "Aether/Http/Response.h"
#include "Aether/Http/Router.h"
#include "Aether/Http/HttpParser.h"
#include "Aether/Http/Connection.h"
#include "Aether/Http/Middleware.h"

// Middleware components
#include "Aether/Middleware/ServeStatic.h"

// NeuralDb protocol
#include "Aether/NeuralDb/NeuralDbServer.h"



#endif // Aether_H