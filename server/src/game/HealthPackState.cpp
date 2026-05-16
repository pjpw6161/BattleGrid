#include "game/HealthPackState.h"

namespace battlegrid
{
bool HealthPackState::IsActive() const
{
    return active;
}

void HealthPackState::Deactivate()
{
    active = false;
    respawnTimerSeconds = respawnDelaySeconds;
}

void HealthPackState::Respawn(double newX, double newY, double newZ)
{
    x = newX;
    y = newY;
    z = newZ;
    active = true;
    respawnTimerSeconds = 0.0;
}
}
