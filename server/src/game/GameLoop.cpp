#include "game/GameLoop.h"

#include "core/Logger.h"

#include <algorithm>
#include <string>

namespace battlegrid
{
GameLoop::GameLoop(int requestedTickRate)
    : tickRate(std::max(1, requestedTickRate)),
      tickInterval(1.0 / static_cast<double>(tickRate)),
      running(false)
{
}

void GameLoop::Start()
{
    running = true;
    Logger::Info(
        "Game loop ready at " + std::to_string(tickRate)
        + " ticks per second."
    );
}

void GameLoop::Stop()
{
    running = false;
}

void GameLoop::Tick()
{
    if (!running)
    {
        return;
    }

    Logger::Info("Game loop tick.");
}
}
