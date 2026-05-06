#include "game/BotState.h"

#include <algorithm>

namespace battlegrid
{
bool BotState::IsAlive() const
{
    return connected && alive && hp > 0;
}

bool BotState::CanBeDamaged() const
{
    return IsAlive() && !invincible;
}

void BotState::ApplyDamage(int damage)
{
    if (!CanBeDamaged())
    {
        return;
    }

    hp = std::max(0, hp - std::max(0, damage));
    if (hp <= 0)
    {
        Kill();
    }
}

void BotState::Kill()
{
    alive = false;
    invincible = false;
    hp = 0;
}

void BotState::Respawn(double newX, double newY)
{
    x = newX;
    y = newY;
    z = 0.0;
    hp = maxHp;
    alive = true;
    invincible = true;
    invincibleTimerSeconds = 1.5;
    respawnTimerSeconds = 0.0;
    targetPlayerId = 0;
    attackTimerSeconds = 0.0;
}
}
