#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include "discovery/discovery_protocol.h"
#include "common/message.h"

namespace meshcompute {

struct PeerInfo {
    DiscoveredPeer discovery;
    std::string public_key;
    bool authenticated = false;
    std::chrono::system_clock::time_point last_heartbeat;
    bool online = false;
    
    // Connection info
    std::string transport_type;  // "websocket", "tcp", "bluetooth"
    void* connection_handle;     // opaque pointer to transport
};

class PeerManager {
public:
    PeerManager(EventBus& bus);
    
    void add_or_update_peer(const DiscoveredPeer& peer);
    void remove_peer(const std::string& node_id);
    std::vector<PeerInfo> get_online_peers() const;
    std::vector<PeerInfo> get_all_peers() const;
    PeerInfo* get_peer(const std::string& node_id);
    
    void start_heartbeat(std::chrono::seconds interval = std::chrono::seconds(5));
    void stop_heartbeat();
    
    // Returns peers that are considered offline
    std::vector<std::string> check_timeout(std::chrono::seconds timeout = std::chrono::seconds(15));

private:
    void heartbeat_loop();
    void send_heartbeat(const std::string& peer_id);
    
    std::unordered_map<std::string, PeerInfo> peers_;
    mutable std::shared_mutex mutex_;
    EventBus& event_bus_;
    std::atomic<bool> heartbeat_running_{false};
    std::thread heartbeat_thread_;
};

} // namespace meshcompute