#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "job.h"

namespace meshcompute {

class IWorker {
public:
    virtual ~IWorker() = default;
    
    virtual std::string worker_type() const = 0;
    virtual nlohmann::json execute(const Job& job) = 0;
    
    // Resource estimation
    virtual uint32_t estimated_ram_mb(const Job& job) const { return 128; }
    virtual uint32_t estimated_cpu_cores(const Job& job) const { return 1; }
    
    // Health check
    virtual bool is_available() const { return true; }
};

class WorkerRegistry {
public:
    static WorkerRegistry& instance();
    
    void register_worker(std::unique_ptr<IWorker> worker);
    IWorker* get_worker(const std::string& type);
    std::vector<std::string> available_workers() const;

private:
    std::unordered_map<std::string, std::unique_ptr<IWorker>> workers_;
};

} // namespace meshcompute