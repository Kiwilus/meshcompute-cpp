#include "tcp_transport.h"
#include <spdlog/spdlog.h>

namespace meshcompute {

TcpTransport::TcpTransport(boost::asio::io_context& ioc)
    : ioc_(ioc), socket_(ioc) {}

bool TcpTransport::connect(const std::string& address, uint16_t port) {
    boost::system::error_code ec;
    boost::asio::ip::tcp::resolver resolver(ioc_);
    auto endpoints = resolver.resolve(address, std::to_string(port), ec);
    if (ec) return false;
    boost::asio::connect(socket_, endpoints, ec);
    if (!ec) {
        do_read();
        return true;
    }
    return false;
}

void TcpTransport::disconnect() {
    boost::system::error_code ec;
    socket_.close(ec);
}

bool TcpTransport::is_connected() const {
    return socket_.is_open();
}

void TcpTransport::send(const std::vector<uint8_t>& data) {
    boost::asio::write(socket_, boost::asio::buffer(data));
}

void TcpTransport::set_receive_callback(ReceiveCallback cb) {
    recv_cb_ = std::move(cb);
}

void TcpTransport::do_read() {
    recv_buffer_.resize(4096);
    socket_.async_read_some(boost::asio::buffer(recv_buffer_),
        [this](boost::system::error_code ec, size_t length) {
            if (!ec && recv_cb_) {
                std::vector<uint8_t> data(recv_buffer_.begin(), recv_buffer_.begin() + length);
                recv_cb_(data);
                do_read();
            }
        });
}

} // namespace meshcompute