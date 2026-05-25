#pragma once
#include "meshcompute/compute/job.h"
#include <thread>
#include <atomic>
#include <vector>

namespace meshcompute::compute {
class WorkerEngine {
public:
    void start(size_t threads);
    void stop();
    void enqueue(Job job);
private:
    void loop(size_t id);
    std::atomic<bool> running_{false};
    std::vector<std::thread> pool_;
    // thread-safe queue...
};
}