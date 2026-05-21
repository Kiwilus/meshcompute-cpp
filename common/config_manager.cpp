#include "config_manager.h"
#include <fstream>
#include <spdlog/spdlog.h>

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

void ConfigManager::load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) {
        spdlog::error("Cannot open config file: {}", path);
        throw std::runtime_error("Config file missing");
    }
    ifs >> config_;
}

nlohmann::json ConfigManager::get_root() const {
    return config_;
}

nlohmann::json ConfigManager::get(const std::string& key) const {
    return config_.value(key, nlohmann::json{});
}