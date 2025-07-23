#ifndef AETHER_PERFORMANCE_METRICS_H
#define AETHER_PERFORMANCE_METRICS_H

#include <chrono>
#include <atomic>
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace Aether {
namespace Performance {

// High-resolution timer for performance measurements
class Timer {
public:
    Timer();
    void start();
    void stop();
    void reset();
    
    double getElapsedSeconds() const;
    double getElapsedMilliseconds() const;
    double getElapsedMicroseconds() const;
    
private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::chrono::high_resolution_clock::time_point endTime_;
    bool running_;
};

// RAII timer for automatic timing
class ScopedTimer {
public:
    ScopedTimer(const std::string& name);
    ~ScopedTimer();
    
private:
    std::string name_;
    Timer timer_;
};

// Performance metrics collection
class MetricsCollector {
public:
    static MetricsCollector& getInstance();
    
    // Request metrics
    void incrementRequestCount();
    void recordRequestDuration(double durationMs);
    void recordResponseSize(size_t bytes);
    void incrementErrorCount();
    void recordConcurrentConnections(int count);
    
    // Memory metrics
    void recordMemoryUsage(size_t bytes);
    void recordPeakMemoryUsage(size_t bytes);
    
    // Custom metrics
    void recordMetric(const std::string& name, double value);
    void incrementCounter(const std::string& name);
    
    // Get metrics
    struct RequestMetrics {
        std::atomic<uint64_t> totalRequests{0};
        std::atomic<uint64_t> totalErrors{0};
        std::atomic<double> averageResponseTime{0.0};
        std::atomic<uint64_t> totalResponseBytes{0};
        std::atomic<int> currentConnections{0};
        std::atomic<int> peakConnections{0};
    };
    
    struct MemoryMetrics {
        std::atomic<size_t> currentMemoryUsage{0};
        std::atomic<size_t> peakMemoryUsage{0};
    };
    
    const RequestMetrics& getRequestMetrics() const { return requestMetrics_; }
    const MemoryMetrics& getMemoryMetrics() const { return memoryMetrics_; }
    
    // Get custom metrics
    double getMetric(const std::string& name) const;
    uint64_t getCounter(const std::string& name) const;
    
    // Report generation
    std::string generateReport() const;
    void resetMetrics();
    
private:
    MetricsCollector() = default;
    
    RequestMetrics requestMetrics_;
    MemoryMetrics memoryMetrics_;
    
    mutable std::mutex customMetricsMutex_;
    std::unordered_map<std::string, std::atomic<double>> customMetrics_;
    std::unordered_map<std::string, std::atomic<uint64_t>> customCounters_;
    
    // For calculating averages
    std::atomic<uint64_t> responseDurationSum_{0};
    std::atomic<uint64_t> responseDurationCount_{0};
};

// Connection pool for improved performance
template<typename T>
class ObjectPool {
public:
    ObjectPool(size_t initialSize = 10, size_t maxSize = 100);
    ~ObjectPool();
    
    std::shared_ptr<T> acquire();
    void release(std::shared_ptr<T> obj);
    
    size_t size() const;
    size_t available() const;
    
private:
    std::vector<std::shared_ptr<T>> pool_;
    std::mutex poolMutex_;
    size_t maxSize_;
    std::atomic<size_t> currentSize_{0};
};

// Performance profiler
class Profiler {
public:
    static Profiler& getInstance();
    
    void startProfiling(const std::string& name);
    void endProfiling(const std::string& name);
    
    // Automatic profiling with RAII
    class ProfileScope {
    public:
        ProfileScope(const std::string& name);
        ~ProfileScope();
    private:
        std::string name_;
    };
    
    void generateProfileReport() const;
    void resetProfiles();
    
private:
    struct ProfileData {
        std::chrono::high_resolution_clock::time_point startTime;
        double totalTime = 0.0;
        uint64_t callCount = 0;
        double minTime = std::numeric_limits<double>::max();
        double maxTime = 0.0;
    };
    
    mutable std::mutex profilesMutex_;
    std::unordered_map<std::string, ProfileData> profiles_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> activeSessions_;
};

// Memory pool for efficient allocation
class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t blockCount);
    ~MemoryPool();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getBlockSize() const { return blockSize_; }
    size_t getTotalBlocks() const { return blockCount_; }
    size_t getAvailableBlocks() const;
    
private:
    size_t blockSize_;
    size_t blockCount_;
    char* memory_;
    std::vector<void*> freeBlocks_;
    std::mutex poolMutex_;
};

// Request/Response buffer pool for zero-copy operations
class BufferPool {
public:
    BufferPool(size_t bufferSize = 8192, size_t poolSize = 100);
    
    std::shared_ptr<std::vector<char>> getBuffer();
    void returnBuffer(std::shared_ptr<std::vector<char>> buffer);
    
    size_t getBufferSize() const { return bufferSize_; }
    size_t getPoolSize() const;
    
private:
    size_t bufferSize_;
    std::vector<std::shared_ptr<std::vector<char>>> buffers_;
    std::mutex bufferMutex_;
};

} // namespace Performance
} // namespace Aether

// Convenience macros for profiling
#define AETHER_PROFILE(name) Aether::Performance::Profiler::ProfileScope _prof(name)
#define AETHER_TIMER(name) Aether::Performance::ScopedTimer _timer(name)

#endif // AETHER_PERFORMANCE_METRICS_H
