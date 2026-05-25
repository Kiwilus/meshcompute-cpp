#include "meshcompute/compute/scheduler.h"
#include <spdlog/spdlog.h>

namespace meshcompute {

Scheduler::Scheduler(std::shared_ptr<TaskQueue> queue,
                     std::shared_ptr<WorkerRegistry> registry)
    : queue_(queue), registry_(registry) {}

Scheduler::~Scheduler() { stop(); }

void Scheduler::start() {
    if (running_) return;
    running_ = true;
    scheduling_thread_ = std::thread([this] { worker_loop(); });
}

void Scheduler::stop() {
    running_ = false;
    if (scheduling_thread_.joinable()) scheduling_thread_.join();
}

bool Scheduler::submit_job(const Job& job) {
    queue_->enqueue(job);
    return true;
}

void Scheduler::worker_loop() {
    while (running_) {
        auto job_opt = queue_->dequeue();
        if (!job_opt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        Job job = *job_opt;
        IWorker* worker = registry_->get_worker(job.worker_type);
        if (!worker) {
            spdlog::error("No worker for type '{}'", job.worker_type);
            queue_->mark_failed(job.id, "Unknown worker type");
            continue;
        }
        try {
            nlohmann::json result = worker->execute(job);
            queue_->mark_completed(job.id, result);
        } catch (const std::exception& e) {
            queue_->mark_failed(job.id, e.what());
        }
    }
}

} // namespace meshcompute