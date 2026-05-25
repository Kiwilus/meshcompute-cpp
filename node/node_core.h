#pragma once
#include <memory>
#include "discovery/lan_broadcast.h"
#include "peer_manager.h"
#include "compute/task_queue.h"
#include "compute/scheduler.h"
#include "security/key_manager.h"
#include "api/event_bus.h"
#include "monitoring/metrics_collector.h"

namespace meshcompute {

class NodeCore {
public:
    NodeCore(const std::string& config_path);
    ~NodeCore();
    
    void start();
    void stop();
    void run();  // blocking main loop
    
    // API for external control
    std::string submit_job(const Job& job);
    std::vector<PeerInfo> list_peers() const;
    std::vector<Job> get_queue_status() const;
    SystemMetrics get_metrics() const;

private:
    void setup_discovery();
    void setup_transports();
    void setup_workers();
    
    // Event handlers
    void on_peer_discovered(const Message& msg);
    void on_job_received(const Message& msg);
    void on_heartbeat(const Message& msg);
    
    // Components
    boost::asio::io_context ioc_;
    std::unique_ptr<LanBroadcastDiscovery> discovery_;
    std::unique_ptr<PeerManager> peer_manager_;
    std::unique_ptr<TaskQueue> task_queue_;
    std::unique_ptr<Scheduler> scheduler_;
    std::unique_ptr<KeyManager> key_manager_;
    std::unique_ptr<MetricsCollector> metrics_;
    
    std::atomic<bool> running_{false};
    std::string node_id_;
};

} // namespace meshcompute