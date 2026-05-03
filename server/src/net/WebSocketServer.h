#pragma once

#include "config/ServerConfig.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include <atomic>
#include <memory>

namespace battlegrid
{
class RoomManager;

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

    ServerConfig config;
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::acceptor acceptor;
    std::shared_ptr<RoomManager> roomManager;
    std::atomic_uint64_t nextPlayerId;
};
}
