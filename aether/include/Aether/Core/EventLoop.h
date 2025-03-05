#ifndef AETHER_EVENTLOOP_H
#define AETHER_EVENTLOOP_H

#include <boost/asio.hpp>
#include <vector>
#include <memory>

namespace Aether {

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void start();
    void stop();
    void post(std::function<void()> task);
    void keepAlive(); 
    

    boost::asio::io_context& getIoContext();

private:
    std::unique_ptr<boost::asio::io_context> ioContext_;
    std::unique_ptr<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>> workGuard_;
    std::vector<std::thread> threads_;
    std::atomic<bool> running_;
};

} // namespace Aether

#endif // AETHER_EVENTLOOP_H