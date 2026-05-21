#include "bot_handler.h"
#include "common/config_manager.h"

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: meshcompute-bot <config.json>\n";
            return 1;
        }
        ConfigManager::instance().load(argv[1]);
        auto cfg = ConfigManager::instance().get("bot");
        std::string url = cfg["server_url"];
        std::string token = cfg["registration_token"];
        std::string bot_id = cfg.value("bot_id", "");
        BotHandler bot(url, token, bot_id);
        bot.start();
    } catch (std::exception& e) {
        spdlog::error("Bot error: {}", e.what());
        return 1;
    }
    return 0;
}