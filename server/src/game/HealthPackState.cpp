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

void HealthPackState::Respawn(double newX, double newY)
{
    x = newX;
    y = newY;
    z = 0.0;
    active = true;
    respawnTimerSeconds = 0.0;
}
}
