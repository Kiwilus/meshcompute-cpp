#pragma once
#include <cstdint>

namespace meshcompute::monitoring {

struct NodeMetrics {
    double cpu;
    size_t ram_used;
    size_t active_jobs;
};

class MetricsCollector {
public:
    NodeMetrics collect();
};

} // namespace meshcompute::monitoring