#pragma once

#include "config/ServerConfig.h"
#include "game/GameLoop.h"

namespace battlegrid
{
class GameServer
{
public:
    explicit GameServer(ServerConfig config);

    void Run();
    void Shutdown();

private:
    ServerConfig config;
    GameLoop gameLoop;
};
}
