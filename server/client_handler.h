#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class Hub;

class ClientHandler : public std::enable_shared_from_this<ClientHandler> {
    websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
    beast::flat_buffer buffer_;
    std::string role_; // "controller", "bot", "unknown"
    std::string id_;   // bot_id if role_ == "bot"
    std::shared_ptr<Hub> hub_;
    std::string admin_token_hash_;
    std::string registration_token_hash_;

public:
    ClientHandler(tcp::socket&& socket, net::ssl::context& ssl_ctx,
                  std::shared_ptr<Hub> hub,
                  const std::string& admin_hash,
                  const std::string& reg_hash);
    void start();
    void send(const std::string& message);

private:
    void on_handshake(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void handle_message(const std::string& msg);
    void authenticate(const nlohmann::json& j);
    void process_controller_message(const nlohmann::json& j);
};