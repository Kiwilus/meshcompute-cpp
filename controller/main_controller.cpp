#include "mesh_controller.h"
#include "common/config_manager.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: meshcompute-controller <config.json>\n";
            return 1;
        }
        ConfigManager::instance().load(argv[1]);
        auto cfg = ConfigManager::instance().get("controller");
        std::string url = cfg["server_url"];
        std::string token = cfg["auth_token"];
        MeshController ctrl(url, token);
        ctrl.connect();
        ctrl.interactive();
    } catch (std::exception& e) {
        spdlog::error("Controller error: {}", e.what());
        return 1;
    }
    return 0;
}