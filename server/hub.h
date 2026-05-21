#pragma once
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include "client_handler.h"

class Hub {
public:
    void register_bot(const std::string& bot_id, std::shared_ptr<ClientHandler> handler);
    void unregister_bot(const std::string& bot_id);
    std::shared_ptr<ClientHandler> get_bot(const std::string& bot_id);
    std::vector<std::string> list_bots();

    void set_controller(std::shared_ptr<ClientHandler> handler);
    std::shared_ptr<ClientHandler> get_controller();

private:
    std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ClientHandler>> bots_;
    std::shared_ptr<ClientHandler> controller_;
};