#pragma once
#include <chrono>
#include <string>
#include <nlohmann/json.hpp>

namespace meshcompute {

struct SystemMetrics {
    // CPU
    float cpu_percent;
    uint32_t cpu_cores;
    std::vector<float> cpu_per_core;
    
    // Memory
    uint64_t total_ram_mb;
    uint64_t used_ram_mb;
    uint64_t free_ram_mb;
    
    // Battery (mobile/embedded)
    float battery_percent = -1.0f;
    bool is_charging = false;
    
    // Storage
    uint64_t total_disk_mb;
    uint64_t free_disk_mb;
    
    // Network
    uint64_t bytes_sent;
    uint64_t bytes_received;
    
    // Node status
    uint32_t active_jobs;
    uint32_t queued_jobs;
    uint32_t connected_peers;
    std::chrono::system_clock::time_point uptime_since;
    
    nlohmann::json to_json() const;
};

class MetricsCollector {
public:
    MetricsCollector();
    
    SystemMetrics collect();
    void start_periodic_collection(std::chrono::seconds interval = std::chrono::seconds(10));
    void stop();
    
    SystemMetrics get_latest() const;

private:
    void collection_loop();
    float get_cpu_usage();
    uint64_t get_used_ram();
    
    SystemMetrics latest_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};
    std::thread collection_thread_;
};

} // namespace meshcompute