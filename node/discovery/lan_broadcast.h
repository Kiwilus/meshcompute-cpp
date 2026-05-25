#pragma once
#include "meshcompute/network/discovery/discovery_protocol.h"
#include <vector>
#include <thread>
#include <atomic>

namespace meshcompute::network::discovery {

class LanBroadcast : public DiscoveryProtocol {
public:
    void start() override;
    void stop() override;
    void on_peer_found(PeerCallback cb) override;
    
private:
    void broadcast_loop();
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::vector<PeerCallback> callbacks_;
};

} // namespace meshcompute::network::discovery