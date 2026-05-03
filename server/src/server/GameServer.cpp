#include "server/GameServer.h"

#include "core/Logger.h"
#include "net/WebSocketServer.h"

#include <memory>
#include <string>
#include <utility>

namespace battlegrid
{
GameServer::GameServer(ServerConfig serverConfig)
    : config(std::move(serverConfig)),
      gameLoop(config.tickRate)
{
}

GameServer::~GameServer() = default;

void GameServer::Run()
{
    Logger::Info("Starting BattleGrid server.");
    Logger::Info("Host: " + config.host);
    Logger::Info("Port: " + std::to_string(config.port));
    Logger::Info("Tick rate: " + std::to_string(config.tickRate));

    gameLoop.Start();

    webSocketServer = std::make_unique<WebSocketServer>(config);

    Logger::Info("Server initialized successfully.");
    Logger::Info("WebSocket game server is ready.");

    webSocketServer->Run();
}

void GameServer::Shutdown()
{
    if (webSocketServer)
    {
        webSocketServer->Stop();
    }

    gameLoop.Stop();
    Logger::Info("Server shutdown complete.");
}
}
