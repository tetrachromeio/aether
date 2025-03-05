#ifndef AETHER_CORE_PRINT_H
#define AETHER_CORE_PRINT_H

#include <string>
#include <iostream>

namespace Aether {
namespace Core {

class Print {
public:
    static void print(const std::string& message);
};

} // namespace Core

// Define print in the AETHERnamespace
inline void print(const std::string& message) {
    Core::Print::print(message);
}

} // namespace AETHER

#endif // CHROMATE_CORE_PRINT_H