#include "net/WebSocketServer.h"

#include "core/Logger.h"
#include "game/RoomManager.h"
#include "net/Session.h"

#include <boost/asio/error.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
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
      tickTimer(ioContext),
      roomManager(std::make_shared<RoomManager>()),
      nextPlayerId(1),
      tickInterval(std::chrono::duration_cast<std::chrono::steady_clock::duration>(
          std::chrono::duration<double>(1.0 / static_cast<double>(std::max(1, config.tickRate)))
      )),
      tickNumber(0),
      sessionsMutex(),
      sessions()
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
    StartGameTickTimer();
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

    try
    {
        tickTimer.cancel();
    }
    catch (const boost::system::system_error& cancelError)
    {
        Logger::Warn("Cancel tick timer failed: " + cancelError.code().message());
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

    std::shared_ptr<Session> session = std::make_shared<Session>(
        std::move(socket),
        roomManager,
        std::move(allocatePlayerId)
    );

    {
        std::lock_guard lock(sessionsMutex);
        sessions.push_back(session);
    }

    session->Start();

    if (acceptor.is_open())
    {
        StartAccept();
    }
}

void WebSocketServer::StartGameTickTimer()
{
    tickTimer.expires_after(tickInterval);
    tickTimer.async_wait(
        [this](const boost::system::error_code& error)
        {
            HandleGameTick(error);
        }
    );
}

void WebSocketServer::HandleGameTick(const boost::system::error_code& error)
{
    if (error == boost::asio::error::operation_aborted)
    {
        return;
    }

    if (error)
    {
        Logger::Error("Game tick timer failed: " + error.message());
        return;
    }

    ++tickNumber;

    const double deltaSeconds = std::chrono::duration<double>(tickInterval).count();
    roomManager->TickAll(deltaSeconds, tickNumber);

    const nlohmann::json snapshotJson =
        roomManager->BuildDefaultRoomSnapshotJson(tickNumber);
    BroadcastSnapshot(snapshotJson.dump());

    if (tickNumber % 30 == 0)
    {
        Logger::Info(
            "Snapshot tick=" + std::to_string(tickNumber)
            + " players=" + std::to_string(snapshotJson["players"].size())
        );
    }

    if (acceptor.is_open())
    {
        StartGameTickTimer();
    }
}

void WebSocketServer::BroadcastSnapshot(const std::string& snapshot)
{
    std::lock_guard lock(sessionsMutex);

    auto iterator = sessions.begin();
    while (iterator != sessions.end())
    {
        if (std::shared_ptr<Session> session = iterator->lock())
        {
            if (session->IsJoined())
            {
                session->SendText(snapshot);
            }

            ++iterator;
        }
        else
        {
            iterator = sessions.erase(iterator);
        }
    }
}
}
