#include "meshcompute/compute/task_queue.h"
#include <algorithm>

namespace meshcompute {

void TaskQueue::enqueue(const Job& job) {
    std::lock_guard lock(mutex_);
    queue_.push(job);
    cv_.notify_one();
}

std::optional<Job> TaskQueue::dequeue() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty(); });
    Job job = queue_.top();
    queue_.pop();
    active_jobs_[job.id] = job;
    return job;
}

std::optional<Job> TaskQueue::dequeue_for_peer(const std::string& peer_id) {
    // Implementierung für Work-Stealing kann später folgen
    return dequeue();
}

bool TaskQueue::steal_job(Job& job) {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) return false;
    job = queue_.top();
    queue_.pop();
    return true;
}

void TaskQueue::mark_completed(const std::string& job_id, const nlohmann::json& result) {
    std::lock_guard lock(mutex_);
    auto it = active_jobs_.find(job_id);
    if (it != active_jobs_.end()) {
        it->second.state = JobState::COMPLETED;
        it->second.result = result;
        active_jobs_.erase(it);
    }
}

void TaskQueue::mark_failed(const std::string& job_id, const std::string& error) {
    std::lock_guard lock(mutex_);
    auto it = active_jobs_.find(job_id);
    if (it != active_jobs_.end()) {
        it->second.state = JobState::FAILED;
        it->second.error_message = error;
        active_jobs_.erase(it);
    }
}

std::vector<Job> TaskQueue::get_active_jobs() const {
    std::lock_guard lock(mutex_);
    std::vector<Job> result;
    for (const auto& [id, job] : active_jobs_) result.push_back(job);
    return result;
}

std::vector<Job> TaskQueue::get_queued_jobs() const {
    std::lock_guard lock(mutex_);
    std::vector<Job> result;
    auto copy = queue_;
    while (!copy.empty()) {
        result.push_back(copy.top());
        copy.pop();
    }
    return result;
}

size_t TaskQueue::size() const {
    std::lock_guard lock(mutex_);
    return queue_.size() + active_jobs_.size();
}

bool TaskQueue::empty() const {
    std::lock_guard lock(mutex_);
    return queue_.empty() && active_jobs_.empty();
}

} // namespace meshcompute