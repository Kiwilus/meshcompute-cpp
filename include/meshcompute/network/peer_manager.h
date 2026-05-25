#pragma once
#include <string>
#include <vector>
#include <shared_mutex>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace meshcompute::network {

struct PeerInfo {
    std::string peer_id;
    std::string public_key_hex;
    std::string address;
    uint16_t port;
    std::chrono::steady_clock::time_point last_seen;
    bool connected = false;
};

class PeerManager {
public:
    void add_or_update(const PeerInfo& peer);
    void remove(const std::string& peer_id);
    PeerInfo get(const std::string& peer_id) const;
    std::vector<PeerInfo> all() const;
    void heartbeat(const std::string& peer_id);

    using Callback = std::function<void(const PeerInfo&)>;
    void on_connected(Callback cb);
    void on_disconnected(Callback cb);

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, PeerInfo> peers_;
    std::vector<Callback> connect_cb_, disconnect_cb_;
};

} // namespace meshcompute::network