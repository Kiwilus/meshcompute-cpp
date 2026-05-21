#include "hub.h"
#include <spdlog/spdlog.h>

void Hub::register_bot(const std::string& bot_id, std::shared_ptr<ClientHandler> handler) {
    std::unique_lock lock(mutex_);
    bots_[bot_id] = handler;
    spdlog::info("Bot registered: {}", bot_id);
}

void Hub::unregister_bot(const std::string& bot_id) {
    std::unique_lock lock(mutex_);
    bots_.erase(bot_id);
    spdlog::info("Bot unregistered: {}", bot_id);
}

std::shared_ptr<ClientHandler> Hub::get_bot(const std::string& bot_id) {
    std::shared_lock lock(mutex_);
    auto it = bots_.find(bot_id);
    return it != bots_.end() ? it->second : nullptr;
}

std::vector<std::string> Hub::list_bots() {
    std::shared_lock lock(mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, _] : bots_) ids.push_back(id);
    return ids;
}

void Hub::set_controller(std::shared_ptr<ClientHandler> handler) {
    std::unique_lock lock(mutex_);
    controller_ = handler;
}

std::shared_ptr<ClientHandler> Hub::get_controller() {
    std::shared_lock lock(mutex_);
    return controller_;
}