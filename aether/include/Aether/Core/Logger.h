#ifndef AETHER_CORE_LOGGER_H
#define AETHER_CORE_LOGGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <iomanip>

namespace Aether {
namespace Core {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& getInstance();
    
    void setLevel(LogLevel level);
    void setOutputFile(const std::string& filename);
    void enableConsoleOutput(bool enable);
    void enableTimestamps(bool enable);
    void enableThreadId(bool enable);
    
    void log(LogLevel level, const std::string& message, const std::string& file = "", int line = 0);
    
    // Convenience methods
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warn(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);
    void fatal(const std::string& message, const std::string& file = "", int line = 0);
    
private:
    Logger() = default;
    ~Logger();
    
    std::string formatMessage(LogLevel level, const std::string& message, const std::string& file, int line);
    std::string levelToString(LogLevel level);
    std::string getCurrentTimestamp();
    
    LogLevel currentLevel_ = LogLevel::INFO;
    std::unique_ptr<std::ofstream> logFile_;
    bool consoleOutput_ = true;
    bool timestampsEnabled_ = true;
    bool threadIdEnabled_ = false;
    std::mutex logMutex_;
};

// Convenience macros for easier logging with file/line info
#define AETHER_LOG_DEBUG(msg) Aether::Core::Logger::getInstance().debug(msg, __FILE__, __LINE__)
#define AETHER_LOG_INFO(msg) Aether::Core::Logger::getInstance().info(msg, __FILE__, __LINE__)
#define AETHER_LOG_WARN(msg) Aether::Core::Logger::getInstance().warn(msg, __FILE__, __LINE__)
#define AETHER_LOG_ERROR(msg) Aether::Core::Logger::getInstance().error(msg, __FILE__, __LINE__)
#define AETHER_LOG_FATAL(msg) Aether::Core::Logger::getInstance().fatal(msg, __FILE__, __LINE__)

} // namespace Core
} // namespace Aether

#endif // AETHER_CORE_LOGGER_H
