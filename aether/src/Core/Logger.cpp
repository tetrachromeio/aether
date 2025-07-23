#include "Aether/Core/Logger.h"
#include <thread>

namespace Aether {
namespace Core {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (logFile_ && logFile_->is_open()) {
        logFile_->close();
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    currentLevel_ = level;
}

void Logger::setOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (logFile_ && logFile_->is_open()) {
        logFile_->close();
    }
    logFile_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    if (!logFile_->is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
        logFile_.reset();
    }
}

void Logger::enableConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex_);
    consoleOutput_ = enable;
}

void Logger::enableTimestamps(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex_);
    timestampsEnabled_ = enable;
}

void Logger::enableThreadId(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex_);
    threadIdEnabled_ = enable;
}

void Logger::log(LogLevel level, const std::string& message, const std::string& file, int line) {
    if (level < currentLevel_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex_);
    std::string formattedMessage = formatMessage(level, message, file, line);
    
    // Output to console if enabled
    if (consoleOutput_) {
        std::ostream& output = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
        output << formattedMessage << std::endl;
    }
    
    // Output to file if configured
    if (logFile_ && logFile_->is_open()) {
        *logFile_ << formattedMessage << std::endl;
        logFile_->flush();
    }
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warn(const std::string& message, const std::string& file, int line) {
    log(LogLevel::WARN, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::fatal(const std::string& message, const std::string& file, int line) {
    log(LogLevel::FATAL, message, file, line);
}

std::string Logger::formatMessage(LogLevel level, const std::string& message, const std::string& file, int line) {
    std::ostringstream oss;
    
    // Add timestamp if enabled
    if (timestampsEnabled_) {
        oss << "[" << getCurrentTimestamp() << "] ";
    }
    
    // Add log level
    oss << "[" << levelToString(level) << "] ";
    
    // Add thread ID if enabled
    if (threadIdEnabled_) {
        oss << "[Thread:" << std::this_thread::get_id() << "] ";
    }
    
    // Add file and line if provided
    if (!file.empty() && line > 0) {
        // Extract just the filename from full path
        std::string filename = file;
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }
        oss << "[" << filename << ":" << line << "] ";
    }
    
    // Add the actual message
    oss << message;
    
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace Core
} // namespace Aether
