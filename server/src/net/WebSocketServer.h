#pragma once

#include "config/ServerConfig.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace battlegrid
{
class RoomManager;
class Session;

class WebSocketServer
{
public:
    explicit WebSocketServer(const ServerConfig& config);
    ~WebSocketServer();

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    void Run();
    void Stop();

private:
    void StartAccept();
    void HandleAccept(
        const boost::system::error_code& error,
        boost::asio::ip::tcp::socket socket
    );
    void StartGameTickTimer();
    void HandleGameTick(const boost::system::error_code& error);
    void BroadcastSnapshot(const std::string& snapshot);

    ServerConfig config;
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::steady_timer tickTimer;
    std::shared_ptr<RoomManager> roomManager;
    std::atomic_uint64_t nextPlayerId;
    std::chrono::steady_clock::duration tickInterval;
    std::uint64_t tickNumber;
    std::mutex sessionsMutex;
    std::vector<std::weak_ptr<Session>> sessions;
};
}
