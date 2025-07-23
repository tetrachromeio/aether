#ifndef AETHER_CORE_CONFIG_H
#define AETHER_CORE_CONFIG_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <variant>

namespace Aether {
namespace Core {

// Type-safe configuration value that can hold different types
using ConfigValue = std::variant<std::string, int, double, bool>;

class Config {
public:
    static Config& getInstance();
    
    // Load configuration from file
    bool loadFromFile(const std::string& filename);
    bool loadFromJson(const std::string& jsonContent);
    bool loadFromEnv(const std::string& prefix = "AETHER_");
    
    // Get configuration values with type safety
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const;
    
    // Set configuration values
    void set(const std::string& key, const ConfigValue& value);
    
    // Check if key exists
    bool has(const std::string& key) const;
    
    // Get all keys with a prefix
    std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const;
    
    // Save current configuration to file
    bool saveToFile(const std::string& filename) const;
    
    // Configuration sections for organized access
    class Section {
    public:
        Section(Config& config, const std::string& prefix);
        
        template<typename T>
        T get(const std::string& key, const T& defaultValue = T{}) const;
        
        void set(const std::string& key, const ConfigValue& value);
        bool has(const std::string& key) const;
        
    private:
        Config& config_;
        std::string prefix_;
    };
    
    Section getSection(const std::string& sectionName);
    
private:
    Config() = default;
    
    std::string makeKey(const std::string& section, const std::string& key) const;
    
    mutable std::mutex configMutex_;
    std::unordered_map<std::string, ConfigValue> values_;
};

// Template implementation for type-safe gets
template<typename T>
T Config::get(const std::string& key, const T& defaultValue) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    auto it = values_.find(key);
    if (it == values_.end()) {
        return defaultValue;
    }
    
    try {
        return std::get<T>(it->second);
    } catch (const std::bad_variant_access&) {
        // Type mismatch, return default
        return defaultValue;
    }
}

template<typename T>
T Config::Section::get(const std::string& key, const T& defaultValue) const {
    return config_.get<T>(prefix_ + key, defaultValue);
}

} // namespace Core
} // namespace Aether

#endif // AETHER_CORE_CONFIG_H
