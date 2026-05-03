#include "net/WebSocketServer.h"

#include "core/Logger.h"
#include "game/RoomManager.h"
#include "net/Session.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace battlegrid
{
namespace
{
using boost::asio::ip::tcp;

void ThrowOnError(const boost::system::error_code& error, const std::string& operation)
{
    if (error)
    {
        throw std::runtime_error(operation + " failed: " + error.message());
    }
}

std::string EndpointToString(const tcp::endpoint& endpoint)
{
    return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}
}

WebSocketServer::WebSocketServer(const ServerConfig& serverConfig)
    : config(serverConfig),
      ioContext(),
      acceptor(ioContext),
      roomManager(std::make_shared<RoomManager>()),
      nextPlayerId(1)
{
    boost::system::error_code error;
    const auto address = boost::asio::ip::make_address(config.host, error);
    ThrowOnError(error, "Parse listen address");

    const tcp::endpoint endpoint(address, config.port);

    acceptor.open(endpoint.protocol(), error);
    ThrowOnError(error, "Open acceptor");

    acceptor.set_option(boost::asio::socket_base::reuse_address(true), error);
    ThrowOnError(error, "Set reuse_address");

    acceptor.bind(endpoint, error);
    ThrowOnError(error, "Bind acceptor");

    acceptor.listen(boost::asio::socket_base::max_listen_connections, error);
    ThrowOnError(error, "Listen");
}

WebSocketServer::~WebSocketServer()
{
    Stop();
}

void WebSocketServer::Run()
{
    Logger::Info(
        "WebSocket server listening on ws://" + config.host + ":"
        + std::to_string(config.port)
    );

    StartAccept();
    ioContext.run();
}

void WebSocketServer::Stop()
{
    boost::system::error_code error;

    if (acceptor.is_open())
    {
        acceptor.close(error);
        if (error)
        {
            Logger::Error("Close acceptor failed: " + error.message());
        }
    }

    ioContext.stop();
}

void WebSocketServer::StartAccept()
{
    acceptor.async_accept(
        [this](const boost::system::error_code& error, tcp::socket socket)
        {
            HandleAccept(error, std::move(socket));
        }
    );
}

void WebSocketServer::HandleAccept(
    const boost::system::error_code& error,
    tcp::socket socket
)
{
    if (error)
    {
        if (acceptor.is_open())
        {
            Logger::Error("Accept failed: " + error.message());
            StartAccept();
        }
        return;
    }

    boost::system::error_code endpointError;
    const tcp::endpoint remoteEndpoint = socket.remote_endpoint(endpointError);
    if (endpointError)
    {
        Logger::Warn("Accepted WebSocket connection from unknown endpoint.");
    }
    else
    {
        Logger::Info("Accepted WebSocket connection from " + EndpointToString(remoteEndpoint));
    }

    auto allocatePlayerId = [this]()
    {
        return nextPlayerId.fetch_add(1, std::memory_order_relaxed);
    };

    std::make_shared<Session>(
        std::move(socket),
        roomManager,
        std::move(allocatePlayerId)
    )->Start();

    if (acceptor.is_open())
    {
        StartAccept();
    }
}
}
