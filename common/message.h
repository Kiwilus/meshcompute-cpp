#pragma once
#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace meshcompute {

enum class MessageType {
    // Discovery
    DISCOVERY_ANNOUNCE,
    DISCOVERY_RESPONSE,
    // Connection
    HEARTBEAT,
    HEARTBEAT_ACK,
    PEER_CONNECT,
    PEER_DISCONNECT,
    // Authentication
    AUTH_CHALLENGE,
    AUTH_RESPONSE,
    // Jobs
    JOB_SUBMIT,
    JOB_ASSIGN,
    JOB_RESULT,
    JOB_CANCEL,
    JOB_PROGRESS,
    // Monitoring
    METRICS_REPORT,
    HEALTH_CHECK,
    // Generic
    ERROR_MSG,
    CUSTOM
};

struct Message {
    std::string id;
    MessageType type;
    std::string sender_id;
    std::string recipient_id;       // empty = broadcast
    std::chrono::system_clock::time_point timestamp;
    std::string signature;
    nlohmann::json payload;

    static Message from_json(const nlohmann::json& j);
    nlohmann::json to_json() const;
    std::string serialize() const;
    static Message deserialize(const std::string& data);
};

// Message type string mapping
const std::unordered_map<std::string, MessageType> message_type_map = {
    {"discovery_announce", MessageType::DISCOVERY_ANNOUNCE},
    {"discovery_response", MessageType::DISCOVERY_RESPONSE},
    {"heartbeat", MessageType::HEARTBEAT},
    // ...
};

} // namespace meshcompute