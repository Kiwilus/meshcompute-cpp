#pragma once
#include "../worker.h"
#include <string>

namespace meshcompute {

class LLMWorker : public IWorker {
public:
    std::string worker_type() const override { return "llm"; }
    nlohmann::json execute(const Job& job) override;
    uint32_t estimated_ram_mb(const Job& job) const override { return 4096; } // Annahme
    uint32_t estimated_cpu_cores(const Job& job) const override { return 4; }
};

} // namespace meshcompute