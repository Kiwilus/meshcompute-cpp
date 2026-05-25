#pragma once
#include "task_queue.h"
#include "worker.h"
#include "job.h"
#include <memory>
#include <thread>
#include <atomic>

namespace meshcompute {

class Scheduler {
public:
    Scheduler(std::shared_ptr<TaskQueue> queue, std::shared_ptr<WorkerRegistry> registry);
    ~Scheduler();

    void start();
    void stop();

    // Assign a job to a specific worker type
    bool submit_job(const Job& job);

    // Work stealing: pull job from queue
    void worker_loop();

private:
    std::shared_ptr<TaskQueue> queue_;
    std::shared_ptr<WorkerRegistry> registry_;
    std::atomic<bool> running_{false};
    std::thread scheduling_thread_;
};

} // namespace meshcompute