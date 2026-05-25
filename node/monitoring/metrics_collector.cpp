#include "meshcompute/monitoring/metrics_collector.h"
#include <spdlog/spdlog.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <fstream>
#include <sstream>
#endif

namespace meshcompute {

MetricsCollector::MetricsCollector() {
    latest_.uptime_since = std::chrono::system_clock::now();
}

SystemMetrics MetricsCollector::collect() {
    SystemMetrics m;
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        m.total_ram_mb = si.totalram / 1024 / 1024;
        m.free_ram_mb = si.freeram / 1024 / 1024;
        m.used_ram_mb = m.total_ram_mb - m.free_ram_mb;
    }
    // CPU Auslastung (vereinfacht)
    std::ifstream stat("/proc/stat");
    std::string line;
    if (std::getline(stat, line)) {
        std::istringstream iss(line);
        std::string cpu;
        long user, nice, system, idle;
        iss >> cpu >> user >> nice >> system >> idle;
        long total = user + nice + system + idle;
        long used = total - idle;
        static long prev_total = 0, prev_used = 0;
        if (prev_total != 0)
            m.cpu_percent = 100.0f * (used - prev_used) / (total - prev_total);
        prev_total = total; prev_used = used;
    }
#endif
    std::lock_guard lock(mutex_);
    latest_ = m;
    return m;
}

void MetricsCollector::start_periodic_collection(std::chrono::seconds interval) {
    running_ = true;
    collection_thread_ = std::thread([this, interval] {
        while (running_) {
            collect();
            std::this_thread::sleep_for(interval);
        }
    });
}

void MetricsCollector::stop() {
    running_ = false;
    if (collection_thread_.joinable()) collection_thread_.join();
}

SystemMetrics MetricsCollector::get_latest() const {
    std::lock_guard lock(mutex_);
    return latest_;
}

} // namespace meshcompute