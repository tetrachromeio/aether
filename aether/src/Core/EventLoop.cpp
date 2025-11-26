#include "Aether/Core/EventLoop.h"

#include <chrono>

namespace Aether {

EventLoop::EventLoop()
    : ioContext_(std::make_unique<boost::asio::io_context>()),
      workGuard_(std::make_unique<boost::asio::executor_work_guard<
          boost::asio::io_context::executor_type>>(
              boost::asio::make_work_guard(*ioContext_))) {}

EventLoop::~EventLoop() {
    stop();
}

void EventLoop::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(true, std::memory_order_release);

    std::size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
        numThreads = 1;
    }

    threads_.reserve(numThreads);
    for (std::size_t i = 0; i < numThreads; ++i) {
        threads_.emplace_back([this] {
            // Each thread runs the io_context loop until stopped
            ioContext_->run();
        });
    }
}

void EventLoop::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(false, std::memory_order_release);

    // Allow io_context::run() to return
    workGuard_.reset();
    ioContext_->stop();

    // Join all worker threads
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();

    // Reset io_context so it can be restarted if needed
    ioContext_->restart();
    workGuard_ = std::make_unique<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>(
            boost::asio::make_work_guard(*ioContext_));
}

void EventLoop::post(std::function<void()> task) {
    // Legacy overload – still uses std::function, but many call sites
    // can be migrated to the templated post() for better performance.
    boost::asio::post(*ioContext_, std::move(task));
}

void EventLoop::keepAlive() {
    // Simple spin-sleep loop: keeps the calling thread alive while the
    // worker threads process the io_context.
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

boost::asio::io_context& EventLoop::getIoContext() {
    return *ioContext_;
}

} // namespace Aether
