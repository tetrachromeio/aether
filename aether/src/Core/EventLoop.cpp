#include "Aether/Core/EventLoop.h"

namespace Aether{

EventLoop::EventLoop() 
    : ioContext_(std::make_unique<boost::asio::io_context>()),
      workGuard_(std::make_unique<boost::asio::executor_work_guard<
          boost::asio::io_context::executor_type>>(
              boost::asio::make_work_guard(*ioContext_))),
      running_(false) {}

EventLoop::~EventLoop() { stop(); }

void EventLoop::start() {
    if (running_) return;
    running_ = true;
    
    const size_t numThreads = std::thread::hardware_concurrency();
    threads_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        threads_.emplace_back([this] { ioContext_->run(); });
    }
}

void EventLoop::stop() {
    if (!running_) return;
    
    workGuard_.reset();
    ioContext_->stop();
    
    for (auto& thread : threads_) {
        if (thread.joinable()) thread.join();
    }
    threads_.clear();
    running_ = false;
}

void EventLoop::post(std::function<void()> task) {
    boost::asio::post(*ioContext_, std::move(task));
}

void EventLoop::keepAlive() {
    // Block the main thread while the event loop is running
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

boost::asio::io_context& EventLoop::getIoContext() { 
    return *ioContext_; 
}


} // namespace Aether