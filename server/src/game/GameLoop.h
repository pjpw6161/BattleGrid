#pragma once

#include <chrono>

namespace battlegrid
{
class GameLoop
{
public:
    explicit GameLoop(int tickRate);

    void Start();
    void Stop();
    void Tick();

private:
    int tickRate;
    std::chrono::duration<double> tickInterval;
    bool running;
};
}
