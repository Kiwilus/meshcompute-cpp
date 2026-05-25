#include "node/peer_manager.h"
#include <spdlog/spdlog.h>

namespace meshcompute {

PeerManager::PeerManager(EventBus& bus) : event_bus_(bus) {}

void PeerManager::add_or_update_peer(const DiscoveredPeer& peer) {
    std::unique_lock lock(mutex_);
    auto it = peers_.find(peer.node_id);
    if (it == peers_.end()) {
        PeerInfo info;
        info.discovery = peer;
        info.online = true;
        info.last_heartbeat = std::chrono::system_clock::now();
        peers_[peer.node_id] = info;
        spdlog::info("New peer added: {}", peer.node_id);
    } else {
        it->second.discovery = peer;
        it->second.last_heartbeat = std::chrono::system_clock::now();
        it->second.online = true;
    }
}

void PeerManager::remove_peer(const std::string& node_id) {
    std::unique_lock lock(mutex_);
    peers_.erase(node_id);
}

std::vector<PeerInfo> PeerManager::get_online_peers() const {
    std::shared_lock lock(mutex_);
    std::vector<PeerInfo> result;
    for (const auto& [id, info] : peers_)
        if (info.online) result.push_back(info);
    return result;
}

std::vector<PeerInfo> PeerManager::get_all_peers() const {
    std::shared_lock lock(mutex_);
    std::vector<PeerInfo> result;
    for (const auto& [id, info] : peers_) result.push_back(info);
    return result;
}

PeerInfo* PeerManager::get_peer(const std::string& node_id) {
    std::shared_lock lock(mutex_);
    auto it = peers_.find(node_id);
    return (it != peers_.end()) ? &it->second : nullptr;
}

void PeerManager::start_heartbeat(std::chrono::seconds interval) {
    heartbeat_running_ = true;
    heartbeat_thread_ = std::thread([this, interval] {
        while (heartbeat_running_) {
            std::this_thread::sleep_for(interval);
            std::shared_lock lock(mutex_);
            for (auto& [id, info] : peers_) {
                if (info.online) {
                    send_heartbeat(id);
                }
            }
        }
    });
}

void PeerManager::stop_heartbeat() {
    heartbeat_running_ = false;
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
}

void PeerManager::send_heartbeat(const std::string& peer_id) {
    Message msg;
    msg.type = MessageType::HEARTBEAT;
    msg.sender_id = "self"; // wird später gesetzt
    msg.recipient_id = peer_id;
    event_bus_.publish(msg);
}

std::vector<std::string> PeerManager::check_timeout(std::chrono::seconds timeout) {
    auto now = std::chrono::system_clock::now();
    std::unique_lock lock(mutex_);
    std::vector<std::string> offline;
    for (auto& [id, info] : peers_) {
        if (info.online && (now - info.last_heartbeat) > timeout) {
            info.online = false;
            offline.push_back(id);
            spdlog::warn("Peer {} timed out", id);
        }
    }
    return offline;
}

} // namespace meshcompute