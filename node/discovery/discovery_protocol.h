#pragma once
#include <string>
#include <chrono>
#include <vector>

namespace meshcompute {

struct DiscoveredPeer {
    std::string node_id;
    std::string ip_address;
    uint16_t port;
    std::string hostname;
    std::string platform;       // "linux-x64", "android-arm64", "windows-x64"
    uint32_t cpu_cores;
    uint64_t total_ram_mb;
    std::chrono::system_clock::time_point last_seen;
};

class DiscoveryProtocol {
public:
    virtual ~DiscoveryProtocol() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::vector<DiscoveredPeer> discover() = 0;
    virtual void announce(const std::string& node_id, uint16_t port) = 0;
};

} // namespace meshcompute