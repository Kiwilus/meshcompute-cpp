#pragma once
#include "transport_interface.h"
#include <boost/asio.hpp>
#include <memory>

namespace meshcompute {

class TcpTransport : public ITransport {
public:
    TcpTransport(boost::asio::io_context& ioc);
    bool connect(const std::string& address, uint16_t port) override;
    void disconnect() override;
    bool is_connected() const override;
    void send(const std::vector<uint8_t>& data) override;
    void set_receive_callback(ReceiveCallback cb) override;
    std::string transport_type() const override { return "tcp"; }
private:
    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::socket socket_;
    ReceiveCallback recv_cb_;
    std::vector<uint8_t> recv_buffer_;
    void do_read();
};

} // namespace meshcompute