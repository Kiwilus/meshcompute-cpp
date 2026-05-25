#pragma once
#include <queue>
#include <functional>
#include "meshcompute/compute/job.h"

namespace meshcompute::compute {
class Scheduler {
public:
    void submit(Job job);
    std::optional<Job> fetch_work(const std::string& worker_id);
    void complete(const std::string& job_id, JobResult result);
private:
    std::priority_queue<Job> queue_;
    std::unordered_map<std::string, Job> running_;
    std::shared_mutex mtx_;
};
}