#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <iostream>
#include <thread>
#include "hub.h"
#include "client_handler.h"
#include "common/config_manager.h"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: meshcompute-server <config.json>\n";
            return 1;
        }
        ConfigManager::instance().load(argv[1]);
        auto cfg = ConfigManager::instance().get("server");
        std::string host = cfg.value("host", "0.0.0.0");
        unsigned short port = cfg.value("port", 8443);
        std::string cert_file = cfg["tls_cert_file"];
        std::string key_file = cfg["tls_key_file"];
        std::string admin_hash = cfg["admin_token_hash"];
        std::string reg_hash = cfg["registration_token_hash"];

        net::io_context ioc{1};
        net::ssl::context ssl_ctx(net::ssl::context::tlsv12_server);
        ssl_ctx.use_certificate_file(cert_file, net::ssl::context::pem);
        ssl_ctx.use_private_key_file(key_file, net::ssl::context::pem);

        ssl_ctx.set_options(
            net::ssl::context::default_workarounds |
            net::ssl::context::no_sslv2 |
            net::ssl::context::single_dh_use);
        ssl_ctx.set_verify_mode(net::ssl::verify_none);

        tcp::acceptor acceptor(ioc, tcp::endpoint(net::ip::make_address(host), port));
        auto hub = std::make_shared<Hub>();

        auto work_guard = net::make_work_guard(ioc);
        std::thread ioc_thread([&ioc] { ioc.run(); });

        spdlog::info("Server starting on {}:{}", host, port);
        while (true) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            spdlog::info("New TCP connection accepted");
            std::make_shared<ClientHandler>(std::move(socket), ssl_ctx, hub, admin_hash, reg_hash)->start();
        }
        ioc_thread.join();
    } catch (std::exception& e) {
        spdlog::error("Server exception: {}", e.what());
        return 1;
    }
    return 0;
}