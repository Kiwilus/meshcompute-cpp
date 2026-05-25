#pragma once
#include "../worker.h"

namespace meshcompute {

class ShellWorker : public IWorker {
public:
    std::string worker_type() const override { return "shell"; }
    nlohmann::json execute(const Job& job) override;
    uint32_t estimated_ram_mb(const Job& job) const override { return 64; }
};

} // namespace meshcompute