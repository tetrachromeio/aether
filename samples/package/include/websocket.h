#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "Aether/Core/EventLoop.h"
#include "Aether/Core/Print.h" // Assuming you have a print function
#include "Aether/aether.h"

namespace Websocket {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketSession;

using Session = std::shared_ptr<WebSocketSession>;
using Message = std::string;

class SocketServer {
public:
    using EventHandler = std::function<void(Session, const Message&)>;

    // Accept a Server object
    SocketServer(Aether::Http::Server& server);
    ~SocketServer();

    void listen(int port);
    void on(const std::string& event, EventHandler handler);
    void emit(const std::string& event, const Message& message);
    void broadcast(const std::string& event, const Message& message);

private:
    void do_accept();

    net::io_context& io_context_; // Reference to the shared io_context
    tcp::acceptor acceptor_;
    std::unordered_map<std::string, EventHandler> eventHandlers_;
    std::vector<Session> sessions_;
};

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(tcp::socket socket);
    void start();
    void send(const std::string& event, const std::string& message);
    void onMessage(std::function<void(const Message&)> handler);

private:
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::function<void(const Message&)> messageHandler_;
};

} // namespace Websocket

#endif // WEBSOCKET_H