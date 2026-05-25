#include "common/message.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace meshcompute {

Message Message::from_json(const nlohmann::json& j) {
    Message msg;
    msg.id = j.at("id").get<std::string>();
    std::string type_str = j.at("type").get<std::string>();
    auto it = message_type_map.find(type_str);
    if (it != message_type_map.end()) {
        msg.type = it->second;
    } else {
        msg.type = MessageType::CUSTOM;
    }
    msg.sender_id = j.value("sender", "");
    msg.recipient_id = j.value("recipient", "");
    msg.timestamp = std::chrono::system_clock::from_time_t(j.value("timestamp", 0));
    msg.signature = j.value("signature", "");
    msg.payload = j.value("payload", nlohmann::json::object());
    return msg;
}

nlohmann::json Message::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    // Reverse lookup type
    for (const auto& [key, val] : message_type_map) {
        if (val == type) {
            j["type"] = key;
            break;
        }
    }
    j["sender"] = sender_id;
    j["recipient"] = recipient_id;
    j["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
    if (!signature.empty()) j["signature"] = signature;
    if (!payload.empty()) j["payload"] = payload;
    return j;
}

std::string Message::serialize() const {
    return to_json().dump();
}

Message Message::deserialize(const std::string& data) {
    return from_json(nlohmann::json::parse(data));
}

} // namespace meshcompute