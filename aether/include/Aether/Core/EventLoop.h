#ifndef AETHER_EVENTLOOP_H
#define AETHER_EVENTLOOP_H

#ifndef BOOST_ERROR_CODE_HEADER_ONLY
#define BOOST_ERROR_CODE_HEADER_ONLY
#endif

#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED
#endif

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <utility>

namespace Aether {

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    // Start N worker threads (N = hardware_concurrency, min 1)
    void start();
    // Stop the loop and join all threads
    void stop();

    // High-performance templated post: avoids std::function allocation
    template <typename Handler>
    void post(Handler&& handler) {
        boost::asio::post(*ioContext_, std::forward<Handler>(handler));
    }

    // Optional: dispatch runs inline if already on this io_context's thread
    template <typename Handler>
    void dispatch(Handler&& handler) {
        boost::asio::dispatch(*ioContext_, std::forward<Handler>(handler));
    }

    // Legacy overload if you still have call sites using std::function
    void post(std::function<void()> task);

    // Block the calling thread while the event loop is running
    void keepAlive(); 

    // Access to the underlying io_context
    boost::asio::io_context& getIoContext();

private:
    std::unique_ptr<boost::asio::io_context> ioContext_;
    std::unique_ptr<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>> workGuard_;
    std::vector<std::thread> threads_;
    std::atomic<bool> running_{false};
};

} // namespace Aether

#endif // AETHER_EVENTLOOP_H
