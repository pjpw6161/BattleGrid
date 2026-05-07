#pragma once

#include <cstdint>
#include <string>

namespace battlegrid
{
struct ProjectileState
{
    std::uint64_t projectileId = 0;
    std::uint64_t ownerPlayerId = 0;
    std::string ownerType = "player";
    std::uint64_t ownerBotId = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double dirX = 1.0;
    double dirY = 0.0;
    double dirZ = 0.0;
    double speed = 1200.0;
    double ageSeconds = 0.0;
    double maxLifetimeSeconds = 2.0;
    double lifeTimeSeconds = 0.35;
    int damage = 20;
    double radius = 20.0;
    bool active = true;
    bool visualOnly = true;
    double startX = 0.0;
    double startY = 0.0;
    double startZ = 0.0;
    double endX = 0.0;
    double endY = 0.0;
    double endZ = 0.0;

    void NormalizeDirection();
};
}
