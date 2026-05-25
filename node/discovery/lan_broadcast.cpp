#include "node/discovery/lan_broadcast.h"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace meshcompute {

LanBroadcastDiscovery::LanBroadcastDiscovery(boost::asio::io_context& ioc)
    : ioc_(ioc), socket_(ioc) {}

LanBroadcastDiscovery::~LanBroadcastDiscovery() { stop(); }

void LanBroadcastDiscovery::start() {
    if (running_) return;
    running_ = true;

    boost::system::error_code ec;
    socket_.open(boost::asio::ip::udp::v4(), ec);
    socket_.set_option(boost::asio::socket_base::broadcast(true));
    socket_.bind(boost::asio::ip::udp::endpoint(
        boost::asio::ip::address_v4::any(), DISCOVERY_PORT));
    broadcast_endpoint_ = boost::asio::ip::udp::endpoint(
        boost::asio::ip::address_v4::broadcast(), DISCOVERY_PORT);

    do_receive();
    broadcast_thread_ = std::thread([this] { do_broadcast(); });
    spdlog::info("LAN Discovery started on port {}", DISCOVERY_PORT);
}

void LanBroadcastDiscovery::stop() {
    running_ = false;
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.close(ec);
    }
    if (broadcast_thread_.joinable()) broadcast_thread_.join();
}

void LanBroadcastDiscovery::do_receive() {
    auto buffer = std::make_shared<std::array<char, 1024>>();
    auto remote = std::make_shared<boost::asio::ip::udp::endpoint>();
    socket_.async_receive_from(boost::asio::buffer(*buffer), *remote,
        [this, buffer, remote](boost::system::error_code ec, size_t length) {
            if (!ec && running_) {
                try {
                    auto j = nlohmann::json::parse(buffer->data(), buffer->data() + length);
                    if (j.contains("announce")) {
                        DiscoveredPeer peer;
                        peer.node_id = j["node_id"];
                        peer.ip_address = remote->address().to_string();
                        peer.port = j["port"];
                        peer.hostname = j.value("hostname", "");
                        peer.platform = j.value("platform", "");
                        peer.cpu_cores = j.value("cpu_cores", 0);
                        peer.total_ram_mb = j.value("ram_mb", 0);
                        peer.last_seen = std::chrono::system_clock::now();
                        {
                            std::lock_guard lock(peers_mutex_);
                            auto it = std::find_if(peers_.begin(), peers_.end(),
                                [&](const DiscoveredPeer& p) { return p.node_id == peer.node_id; });
                            if (it != peers_.end()) *it = peer;
                            else peers_.push_back(peer);
                        }
                        spdlog::debug("Discovered peer {} at {}:{}", peer.node_id, peer.ip_address, peer.port);
                    }
                } catch (...) {}
            }
            if (running_) do_receive();
        });
}

void LanBroadcastDiscovery::do_broadcast() {
    while (running_) {
        nlohmann::json announce;
        announce["announce"] = true;
        // Diese Werte werden später vom NodeCore gesetzt
        announce["node_id"] = "unknown";
        announce["port"] = 0;
        auto data = announce.dump();
        socket_.send_to(boost::asio::buffer(data), broadcast_endpoint_);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

std::vector<DiscoveredPeer> LanBroadcastDiscovery::discover() {
    std::lock_guard lock(peers_mutex_);
    return peers_;
}

void LanBroadcastDiscovery::announce(const std::string& node_id, uint16_t port) {
    // Wird über do_broadcast aktualisiert – hier nur speichern
}

} // namespace meshcompute