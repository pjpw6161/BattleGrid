#pragma once

#include <cstdint>

namespace battlegrid
{
struct PlayerInput
{
    std::uint64_t seq = 0;
    double moveX = 0.0;
    double moveY = 0.0;
    double aimX = 0.0;
    double aimY = 0.0;
    bool fire = false;
};
}
