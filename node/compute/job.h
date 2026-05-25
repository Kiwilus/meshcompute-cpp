#pragma once
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace meshcompute {

enum class JobPriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
};

enum class JobState {
    QUEUED,
    ASSIGNED,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct Job {
    std::string id;
    std::string submitter_id;
    std::string assigned_worker_id;
    
    JobPriority priority = JobPriority::NORMAL;
    JobState state = JobState::QUEUED;
    
    std::string worker_type;        // e.g. "shell", "sysinfo", "llm"
    nlohmann::json parameters;      // worker-specific parameters
    
    int max_retries = 3;
    int retry_count = 0;
    std::chrono::seconds timeout{300};
    
    // Resource constraints
    uint32_t min_ram_mb = 0;
    uint32_t min_cpu_cores = 0;
    float max_cpu_percent = 100.0f;
    std::vector<std::string> required_platforms;  // "linux-x64", "android-arm64"
    
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point completed_at;
    
    nlohmann::json result;
    std::string error_message;
    
    nlohmann::json to_json() const;
    static Job from_json(const nlohmann::json& j);
};

} // namespace meshcompute