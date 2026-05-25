#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <nlohmann/json.hpp>

namespace meshcompute::compute {

struct ResourceLimits {
    size_t max_ram_mb = 512;
    int max_cpu_percent = 80;
};

struct Job {
    std::string id;
    std::string command;          // auszuführender Befehl / Task-Typ
    nlohmann::json payload;       // Parameter
    int priority = 0;
    std::string submitter_id;     // Node-ID des Auftraggebers
    std::string signature;        // Ed25519 Signatur des gesamten Jobs
    uint32_t max_retries = 3;

    std::string serialize() const; // für Signatur
    static Job from_json(const nlohmann::json& j);
    nlohmann::json to_json() const;
};

struct JobResult {
    std::string job_id;
    bool success;
    std::string output;
    std::string error;
    int exit_code = 0;
};

} // namespace meshcompute::compute