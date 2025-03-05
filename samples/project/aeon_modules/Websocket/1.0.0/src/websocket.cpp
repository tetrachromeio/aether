#include "websocket.h"
#include "Aether/aether.h"
#include <iostream>

using namespace Aether; // Bring aether into scope

namespace Websocket {

SocketServer::SocketServer(Aether::Http::Server& server)
    : io_context_(server.ioContext()), // Use the Server's io_context
      acceptor_(io_context_) {}

SocketServer::~SocketServer() {
    // No need to stop eventLoop_ since it's managed by the Server
}

void SocketServer::listen(int port) {
    tcp::endpoint endpoint(tcp::v4(), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(net::socket_base::max_listen_connections);

    do_accept();
}

void SocketServer::on(const std::string& event, EventHandler handler) {
    eventHandlers_[event] = handler;
}

void SocketServer::emit(const std::string& event, const Message& message) {
    for (auto& session : sessions_) {
        session->send(event, message);
    }
}

void SocketServer::broadcast(const std::string& event, const Message& message) {
    for (auto& session : sessions_) {
        session->send(event, message);
    }
}

void SocketServer::do_accept() {
    acceptor_.async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<WebSocketSession>(std::move(socket));
                sessions_.push_back(session);
                session->start();
                session->onMessage([this, session](const Message& message) {
                    // Parse the event and message
                    size_t colonPos = message.find(':');
                    if (colonPos != std::string::npos) {
                        std::string event = message.substr(0, colonPos);
                        std::string msg = message.substr(colonPos + 1);
                        auto it = eventHandlers_.find(event);
                        if (it != eventHandlers_.end()) {
                            it->second(session, msg);
                        }
                    }
                });
            }
            do_accept();
        });
}

WebSocketSession::WebSocketSession(tcp::socket socket)
    : ws_(std::move(socket)) {}

void WebSocketSession::start() {
    ws_.async_accept(
        [self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                self->do_read();
            }
        });
}

void WebSocketSession::send(const std::string& event, const std::string& message) {
    std::string formattedMessage = event + ":" + message; // Combine event and message
    ws_.async_write(
        net::buffer(formattedMessage),
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                print("Error sending message: " + ec.message());
            }
        });
}

void WebSocketSession::onMessage(std::function<void(const Message&)> handler) {
    messageHandler_ = handler;
}

void WebSocketSession::do_read() {
    ws_.async_read(
        buffer_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                Message message(beast::buffers_to_string(self->buffer_.data()));
                self->buffer_.consume(bytes_transferred);
                if (self->messageHandler_) {
                    self->messageHandler_(message);
                }
                self->do_read();
            }
        });
}

} // namespace Websocket