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

bool BotState::CanFire() const
{
    return IsAlive() && !reloading && ammo > 0 && fireCooldownSeconds <= 0.0;
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
    combatState = "dead";
    fireBlockReason.clear();
    distanceToTarget = 0.0;
    wantsToShoot = false;
    allowedToShoot = false;
    currentShootersForTarget = 0;
}

void BotState::Respawn(double newX, double newY, double newZ)
{
    x = newX;
    y = newY;
    z = newZ;
    hp = maxHp;
    alive = true;
    invincible = true;
    invincibleTimerSeconds = 1.5;
    respawnTimerSeconds = 0.0;
    targetPlayerId = 0;
    combatState = "wander";
    fireBlockReason.clear();
    distanceToTarget = 0.0;
    hasLineOfFire = true;
    wantsToShoot = false;
    allowedToShoot = false;
    currentShootersForTarget = 0;
    targetReconsiderTimerSeconds = 0.0;
    stuckTimerSeconds = 0.0;
    lastXForStuck = x;
    lastYForStuck = y;
    currentWaypointIndex = 0;
    attackTimerSeconds = 0.0;
    ammo = magazineSize;
    reloading = false;
    reloadTimerSeconds = 0.0;
    fireCooldownSeconds = 0.0;
}

void BotState::StartReload()
{
    if (reloading || ammo >= magazineSize)
    {
        return;
    }

    reloading = true;
    reloadTimerSeconds = reloadTimeSeconds;
}

void BotState::FinishReload()
{
    ammo = magazineSize;
    reloading = false;
    reloadTimerSeconds = 0.0;
}

void BotState::ConsumeAmmo()
{
    if (!CanFire())
    {
        return;
    }

    --ammo;
    fireCooldownSeconds = fireIntervalSeconds;
}

void BotState::TickWeapon(double deltaSeconds)
{
    fireCooldownSeconds = std::max(0.0, fireCooldownSeconds - deltaSeconds);

    if (reloading)
    {
        reloadTimerSeconds -= deltaSeconds;
        if (reloadTimerSeconds <= 0.0)
        {
            FinishReload();
        }
        return;
    }

    if (ammo <= 0)
    {
        StartReload();
    }
}
}
