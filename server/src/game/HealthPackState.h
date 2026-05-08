#pragma once

#include <cstdint>

namespace battlegrid
{
struct HealthPackState
{
    std::uint64_t healthPackId = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool active = true;
    int healAmount = 35;
    double pickupRadius = 120.0;
    double respawnTimerSeconds = 0.0;
    double respawnDelaySeconds = 15.0;

    bool IsActive() const;
    void Deactivate();
    void Respawn(double newX, double newY);
};
}
