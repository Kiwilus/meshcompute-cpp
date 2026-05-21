#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <nlohmann/json.hpp>

class MeshController {
public:
    MeshController(const std::string& server_url, const std::string& auth_token);
    void connect();
    void interactive();

private:
    void do_auth();
    void read_loop();
    void handle_message(const std::string& msg);
    void send(const std::string& msg);

    std::string server_url_;
    std::string auth_token_;
    net::io_context ioc_;
    net::ssl::context ssl_ctx_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
};