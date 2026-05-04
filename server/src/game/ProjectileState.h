#pragma once

#include <cstdint>

namespace battlegrid
{
struct ProjectileState
{
    std::uint64_t projectileId = 0;
    std::uint64_t ownerPlayerId = 0;
    double x = 0.0;
    double y = 0.0;
    double dirX = 1.0;
    double dirY = 0.0;
    double speed = 1200.0;
    double ageSeconds = 0.0;
    double maxLifetimeSeconds = 2.0;
    int damage = 20;
    double radius = 20.0;
    bool active = true;

    void NormalizeDirection();
};
}
