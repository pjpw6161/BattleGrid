#include "server/GameServer.h"

#include "core/Logger.h"

#include <string>
#include <utility>

namespace battlegrid
{
GameServer::GameServer(ServerConfig serverConfig)
    : config(std::move(serverConfig)),
      gameLoop(config.tickRate)
{
}

void GameServer::Run()
{
    Logger::Info("Starting BattleGrid server.");
    Logger::Info("Host: " + config.host);
    Logger::Info("Port: " + std::to_string(config.port));
    Logger::Info("Tick rate: " + std::to_string(config.tickRate));

    gameLoop.Start();

    Logger::Info("Server initialized successfully.");
    Logger::Info("Networking is not implemented yet.");
}

void GameServer::Shutdown()
{
    gameLoop.Stop();
    Logger::Info("Server shutdown complete.");
}
}
