#include "node/compute/worker.h"

namespace meshcompute {

WorkerRegistry& WorkerRegistry::instance() {
    static WorkerRegistry reg;
    return reg;
}

void WorkerRegistry::register_worker(std::unique_ptr<IWorker> worker) {
    workers_[worker->worker_type()] = std::move(worker);
}

IWorker* WorkerRegistry::get_worker(const std::string& type) {
    auto it = workers_.find(type);
    return (it != workers_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> WorkerRegistry::available_workers() const {
    std::vector<std::string> types;
    for (const auto& [type, _] : workers_) types.push_back(type);
    return types;
}

} // namespace meshcompute