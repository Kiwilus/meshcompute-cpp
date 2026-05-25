#pragma once
#include <queue>
#include <mutex>
#include <optional>
#include "job.h"

namespace meshcompute::compute {

class TaskQueue {
public:
    void push(const Job& job);
    std::optional<Job> pop();
    size_t size() const;
private:
    std::queue<Job> queue_;
    mutable std::mutex mutex_;
};

} // namespace meshcompute::compute