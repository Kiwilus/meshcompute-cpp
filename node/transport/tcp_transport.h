#pragma once
#include "meshcompute/network/transport/transport_interface.h"
#include <unordered_map>
#include <thread>
#include <atomic>

namespace meshcompute::network::transport {

class TcpTransport : public TransportInterface {
public:
    void send(const std::string& peer_id, const std::vector<uint8_t>& data) override;
    void on_receive(RecvCallback cb) override;
    void start();
    void stop();
    
private:
    void accept_loop();
    std::atomic<bool> running_{false};
    std::unordered_map<std::string, RecvCallback> recv_callbacks_;
};

} // namespace meshcompute::network::transport