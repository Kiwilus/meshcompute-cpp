#include "node/node_core.h"
#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <chrono>

namespace meshcompute {

NodeCore::NodeCore(const std::string& config_path) {
    // Konfiguration laden
    // In der Praxis: ConfigManager nutzen
    node_id_ = "node-001"; // wird durch KeyManager überschrieben
    ioc_ = std::make_shared<boost::asio::io_context>();
    discovery_ = std::make_unique<LanBroadcastDiscovery>(*ioc_);
    peer_manager_ = std::make_unique<PeerManager>(EventBus::instance());
    task_queue_ = std::make_shared<TaskQueue>();
    auto registry = std::make_shared<WorkerRegistry>();
    scheduler_ = std::make_unique<Scheduler>(task_queue_, registry);
    key_manager_ = std::make_unique<KeyManager>();
    metrics_ = std::make_unique<MetricsCollector>();

    // Registriere Worker
    registry->register_worker(std::make_unique<ShellWorker>());
    registry->register_worker(std::make_unique<LLMWorker>());
    registry->register_worker(std::make_unique<CryptoMinerWorker>());
}

NodeCore::~NodeCore() { stop(); }

void NodeCore::start() {
    running_ = true;
    key_manager_->generate_keys();
    node_id_ = key_manager_->get_node_id();
    spdlog::info("Node {} starting", node_id_);

    discovery_->start();
    peer_manager_->start_heartbeat(std::chrono::seconds(5));
    scheduler_->start();
    metrics_->start_periodic_collection(std::chrono::seconds(10));

    // Event-Handler registrieren
    EventBus::instance().subscribe(MessageType::JOB_SUBMIT,
        [this](const Message& msg) { on_job_received(msg); });
    EventBus::instance().subscribe(MessageType::HEARTBEAT,
        [this](const Message& msg) { on_heartbeat(msg); });
}

void NodeCore::stop() {
    running_ = false;
    scheduler_->stop();
    peer_manager_->stop_heartbeat();
    discovery_->stop();
    metrics_->stop();
}

void NodeCore::run() {
    start();
    // Boost io_context loop in eigenem Thread
    std::thread io_thread([this] { ioc_->run(); });
    io_thread.join();
}

std::string NodeCore::submit_job(const Job& job) {
    scheduler_->submit_job(job);
    return job.id;
}

std::vector<PeerInfo> NodeCore::list_peers() const {
    return peer_manager_->get_online_peers();
}

std::vector<Job> NodeCore::get_queue_status() const {
    return task_queue_->get_queued_jobs();
}

SystemMetrics NodeCore::get_metrics() const {
    return metrics_->get_latest();
}

// --- Event-Handler ---
void NodeCore::on_peer_discovered(const Message& msg) {
    auto peer = DiscoveredPeer::from_json(msg.payload); // müsste man definieren
    peer_manager_->add_or_update_peer(peer);
}

void NodeCore::on_job_received(const Message& msg) {
    Job job = Job::from_json(msg.payload);
    submit_job(job);
}

void NodeCore::on_heartbeat(const Message& msg) {
    // Aktualisiere last_heartbeat des Peers
    PeerInfo* peer = peer_manager_->get_peer(msg.sender_id);
    if (peer) peer->last_heartbeat = std::chrono::system_clock::now();
}

} // namespace meshcompute