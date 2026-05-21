#pragma once
#include <string>
#include <nlohmann/json.hpp>

class ConfigManager {
public:
    static ConfigManager& instance();
    void load(const std::string& path);
    nlohmann::json get_root() const;
    nlohmann::json get(const std::string& key) const;

private:
    ConfigManager() = default;
    nlohmann::json config_;
};