#pragma once
#include "discovery_protocol.h"
#include <boost/asio.hpp>
#include <thread>
#include <atomic>

namespace meshcompute {

class LanBroadcastDiscovery : public DiscoveryProtocol {
public:
    static constexpr uint16_t DISCOVERY_PORT = 9999;
    
    LanBroadcastDiscovery(boost::asio::io_context& ioc);
    ~LanBroadcastDiscovery();
    
    void start() override;
    void stop() override;
    std::vector<DiscoveredPeer> discover() override;
    void announce(const std::string& node_id, uint16_t port) override;

private:
    void do_receive();
    void do_broadcast();
    
    boost::asio::io_context& ioc_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint broadcast_endpoint_;
    std::vector<DiscoveredPeer> peers_;
    std::mutex peers_mutex_;
    std::atomic<bool> running_{false};
    std::thread broadcast_thread_;
};

} // namespace meshcompute