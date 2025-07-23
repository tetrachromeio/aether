#include "Aether/Core/Config.h"
#include "Aether/Core/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace Aether {
namespace Core {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        AETHER_LOG_ERROR("Failed to open config file: " + filename);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromJson(buffer.str());
}

bool Config::loadFromJson(const std::string& jsonContent) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    // Simple JSON parser for basic key-value pairs
    // For production, consider using a proper JSON library like nlohmann/json
    std::istringstream stream(jsonContent);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "//") {
            continue;
        }
        
        // Look for key-value pairs in format "key":"value" or "key":value
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // Remove quotes from key and value
        auto removeQuotes = [](std::string& str) {
            if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
                str = str.substr(1, str.size() - 2);
            }
            // Remove trailing comma if present
            if (!str.empty() && str.back() == ',') {
                str.pop_back();
            }
        };
        
        removeQuotes(key);
        removeQuotes(value);
        
        // Try to parse value as different types
        ConfigValue configValue;
        
        // Check for boolean
        if (value == "true") {
            configValue = true;
        } else if (value == "false") {
            configValue = false;
        }
        // Check for integer
        else if (value.find('.') == std::string::npos && 
                 std::all_of(value.begin(), value.end(), [](char c) { 
                     return std::isdigit(c) || c == '-'; 
                 })) {
            try {
                configValue = std::stoi(value);
            } catch (...) {
                configValue = value;
            }
        }
        // Check for double
        else if (std::count(value.begin(), value.end(), '.') == 1) {
            try {
                configValue = std::stod(value);
            } catch (...) {
                configValue = value;
            }
        }
        // Default to string
        else {
            configValue = value;
        }
        
        values_[key] = configValue;
    }
    
    AETHER_LOG_INFO("Loaded " + std::to_string(values_.size()) + " configuration values");
    return true;
}

bool Config::loadFromEnv(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    int count = 0;
    
    // Common environment variables to check
    std::vector<std::string> envVars = {
        "PORT", "HOST", "LOG_LEVEL", "LOG_FILE", "DEBUG", "THREADS",
        "SSL_CERT", "SSL_KEY", "STATIC_DIR", "UPLOAD_DIR", "MAX_REQUEST_SIZE"
    };
    
    for (const auto& var : envVars) {
        std::string fullVar = prefix + var;
        const char* value = std::getenv(fullVar.c_str());
        
        if (value != nullptr) {
            std::string strValue(value);
            
            // Convert to appropriate type
            ConfigValue configValue;
            
            if (strValue == "true" || strValue == "1") {
                configValue = true;
            } else if (strValue == "false" || strValue == "0") {
                configValue = false;
            } else if (std::all_of(strValue.begin(), strValue.end(), [](char c) { 
                         return std::isdigit(c) || c == '-'; 
                     })) {
                try {
                    configValue = std::stoi(strValue);
                } catch (...) {
                    configValue = strValue;
                }
            } else {
                configValue = strValue;
            }
            
            values_[var] = configValue;
            count++;
        }
    }
    
    AETHER_LOG_INFO("Loaded " + std::to_string(count) + " environment variables");
    return count > 0;
}

void Config::set(const std::string& key, const ConfigValue& value) {
    std::lock_guard<std::mutex> lock(configMutex_);
    values_[key] = value;
}

bool Config::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return values_.find(key) != values_.end();
}

std::vector<std::string> Config::getKeysWithPrefix(const std::string& prefix) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    std::vector<std::string> keys;
    for (const auto& pair : values_) {
        if (pair.first.substr(0, prefix.length()) == prefix) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

bool Config::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        AETHER_LOG_ERROR("Failed to open config file for writing: " + filename);
        return false;
    }
    
    file << "{\n";
    bool first = true;
    for (const auto& pair : values_) {
        if (!first) {
            file << ",\n";
        }
        
        file << "  \"" << pair.first << "\": ";
        
        std::visit([&file](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                file << "\"" << value << "\"";
            } else if constexpr (std::is_same_v<T, bool>) {
                file << (value ? "true" : "false");
            } else {
                file << value;
            }
        }, pair.second);
        
        first = false;
    }
    file << "\n}\n";
    
    return true;
}

Config::Section Config::getSection(const std::string& sectionName) {
    return Section(*this, sectionName + ".");
}

std::string Config::makeKey(const std::string& section, const std::string& key) const {
    return section.empty() ? key : section + "." + key;
}

// Section implementation
Config::Section::Section(Config& config, const std::string& prefix)
    : config_(config), prefix_(prefix) {
}

void Config::Section::set(const std::string& key, const ConfigValue& value) {
    config_.set(prefix_ + key, value);
}

bool Config::Section::has(const std::string& key) const {
    return config_.has(prefix_ + key);
}

} // namespace Core
} // namespace Aether
