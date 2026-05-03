#pragma once

#include "config/ServerConfig.h"
#include "game/GameLoop.h"

#include <memory>

namespace battlegrid
{
class WebSocketServer;

class GameServer
{
public:
    explicit GameServer(ServerConfig config);
    ~GameServer();

    void Run();
    void Shutdown();

private:
    ServerConfig config;
    GameLoop gameLoop;
    std::unique_ptr<WebSocketServer> webSocketServer;
};
}
