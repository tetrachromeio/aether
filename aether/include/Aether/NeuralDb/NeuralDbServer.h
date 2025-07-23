#ifndef AETHER_NEURALDB_SERVER_H
#define AETHER_NEURALDB_SERVER_H

#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <iostream>

namespace Aether {
namespace NeuralDb {

// OPCODE definitions (example)
enum class Opcode : uint8_t {
    PING = 0x01,
    QUERY = 0x02,
    RESPONSE = 0x03,
    ERROR = 0xFF
};

using MessageHandler = std::function<void(Opcode, const std::vector<uint8_t>&, std::vector<uint8_t>&)>;

class NeuralDbServer {
public:
    NeuralDbServer(int port, MessageHandler handler)
        : io_context_(),
          acceptor_(io_context_),
          handler_(handler),
          running_(false) {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
    }

    void start() {
        running_ = true;
        do_accept();
    }

    void run() {
        io_context_.run();
    }

    void stop() {
        running_ = false;
        acceptor_.close();
        io_context_.stop();
    }

private:
    void do_accept() {
        if (!running_) return;
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor_.get_executor());
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
            if (!ec) {
                std::thread([this, socket]() { handle_client(socket); }).detach();
            }
            do_accept();
        });
    }

    void handle_client(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
        try {
            while (running_ && socket->is_open()) {
                // add debugging output
                uint8_t opcode;
                uint32_t length;
                boost::system::error_code ec;
                boost::asio::read(*socket, boost::asio::buffer(&opcode, 1), ec);
                if (ec) break;
                boost::asio::read(*socket, boost::asio::buffer(&length, 4), ec);
                if (ec) break;
                length = ntohl(length);
                std::vector<uint8_t> payload(length);
                if (length > 0) {
                    boost::asio::read(*socket, boost::asio::buffer(payload.data(), length), ec);
                    if (ec) break;
                }
                std::vector<uint8_t> response;
                handler_(static_cast<Opcode>(opcode), payload, response);
                if (!response.empty()) {
                    uint8_t resp_opcode = opcode; // Echo the request opcode
                    uint32_t resp_len = htonl(response.size());
                    boost::asio::write(*socket, boost::asio::buffer(&resp_opcode, 1));
                    boost::asio::write(*socket, boost::asio::buffer(&resp_len, 4));
                    boost::asio::write(*socket, boost::asio::buffer(response.data(), response.size()));
                }
            }
        } catch (...) {
            // Ignore errors
            // You might want to log this in a real application
            std::cerr << "Error handling client." << std::endl;
        }
        socket->close();
    }

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    MessageHandler handler_;
    std::atomic<bool> running_;
};

} // namespace NeuralDb
} // namespace Aether

#endif // AETHER_NEURALDB_SERVER_H
