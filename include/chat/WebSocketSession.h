#pragma once

#include <coroutine>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <spdlog/spdlog.h>

#include "UserInfo.hpp"
#include "Database.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

class WebSocketSession;
// using MessageHandler = std::function<asio::awaitable<void>(WebSocketSession&, const json&)>;
// auto consoleLogger = spdlog::stdout_color_mt("ConsoleLogger");

extern std::map<std::string, std::shared_ptr<WebSocketSession>> userSessionMap;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(tcp::socket socket, const std::string& dbHost, const std::string& dbUser, const std::string& dbPassword, const std::string& dbName);

    void start();

private:
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    // std::unordered_map<std::string, MessageHandler> messageHandlers;
    Database db_;

    asio::awaitable<void> onRead(std::size_t bytes_transferred);

    std::shared_ptr<WebSocketSession> findSessionById(const std::string& userId);
};
