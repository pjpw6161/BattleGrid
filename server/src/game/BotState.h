#pragma once

#include <cstdint>
#include <string>

namespace battlegrid
{
struct BotState
{
    std::uint64_t botId = 0;
    std::string name;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double yaw = 0.0;
    int hp = 100;
    int maxHp = 100;
    bool alive = true;
    bool invincible = false;
    double respawnTimerSeconds = 0.0;
    double invincibleTimerSeconds = 0.0;
    double speed = 500.0;
    double bodyRadius = 90.0;
    double headRadius = 45.0;
    double bodyHeight = 90.0;
    double headHeight = 160.0;
    std::uint64_t targetPlayerId = 0;
    double wanderTargetX = 0.0;
    double wanderTargetY = 0.0;
    double decisionTimerSeconds = 0.0;
    double attackCooldownSeconds = 1.0;
    double attackTimerSeconds = 0.0;
    int ammo = 30;
    int magazineSize = 30;
    bool reloading = false;
    double reloadTimerSeconds = 0.0;
    double reloadTimeSeconds = 2.5;
    double fireCooldownSeconds = 0.0;
    double fireIntervalSeconds = 0.45;
    double aimSpreadDegrees = 12.0;
    double attackRange = 1400.0;
    double preferredCombatRange = 900.0;
    bool connected = true;

    bool IsAlive() const;
    bool CanBeDamaged() const;
    bool CanFire() const;
    void ApplyDamage(int damage);
    void Kill();
    void Respawn(double newX, double newY);
    void StartReload();
    void FinishReload();
    void ConsumeAmmo();
    void TickWeapon(double deltaSeconds);
};
}
