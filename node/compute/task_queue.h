#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include "job.h"

namespace meshcompute {

// Priority queue comparator: higher priority = popped first
struct JobPriorityCompare {
    bool operator()(const Job& a, const Job& b) {
        return a.priority < b.priority;
    }
};

class TaskQueue {
public:
    void enqueue(const Job& job);
    std::optional<Job> dequeue();                    // Local work
    std::optional<Job> dequeue_for_peer(const std::string& peer_id);  // Remote work
    bool steal_job(Job& job);                         // Work stealing
    
    void mark_completed(const std::string& job_id, const nlohmann::json& result);
    void mark_failed(const std::string& job_id, const std::string& error);
    void retry(const std::string& job_id);
    
    std::vector<Job> get_active_jobs() const;
    std::vector<Job> get_queued_jobs() const;
    
    size_t size() const;
    bool empty() const;

private:
    std::priority_queue<Job, std::vector<Job>, JobPriorityCompare> queue_;
    std::unordered_map<std::string, Job> active_jobs_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace meshcompute