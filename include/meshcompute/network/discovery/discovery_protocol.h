#pragma once
#include <string>
#include <functional>
#include <vector>

namespace meshcompute::network::discovery {

struct DiscoveredPeer {
    std::string id;
    std::string address;
    uint16_t port;
};

class DiscoveryProtocol {
public:
    virtual ~DiscoveryProtocol() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    using PeerCallback = std::function<void(const DiscoveredPeer&)>;
    virtual void on_peer_found(PeerCallback) = 0;
};

} // namespace meshcompute::network::discovery