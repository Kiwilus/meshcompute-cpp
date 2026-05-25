#pragma once
#include "../worker.h"

namespace meshcompute {

class CryptoMinerWorker : public IWorker {
public:
    std::string worker_type() const override { return "crypto_miner"; }
    nlohmann::json execute(const Job& job) override;
    uint32_t estimated_ram_mb(const Job& job) const override { return 256; }
    uint32_t estimated_cpu_cores(const Job& job) const override { return 1; }
};

} // namespace meshcompute