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
    std::string combatState = "wander";
    std::string fireBlockReason;
    double distanceToTarget = 0.0;
    bool inDetectRange = false;
    bool inAttackRange = false;
    bool hasLineOfFire = true;
    bool wantsToShoot = false;
    bool allowedToShoot = false;
    bool canFire = false;
    std::string lastFireReason;
    int currentShootersForTarget = 0;
    double targetReconsiderTimerSeconds = 0.0;
    double homeX = 0.0;
    double homeY = 0.0;
    double wanderTargetX = 0.0;
    double wanderTargetY = 0.0;
    double wanderWaitTimerSeconds = 0.0;
    double decisionTimerSeconds = 0.0;
    double stuckTimerSeconds = 0.0;
    double lastXForStuck = 0.0;
    double lastYForStuck = 0.0;
    std::uint64_t currentWaypointIndex = 0;
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
    void Respawn(double newX, double newY, double newZ = 0.0);
    void StartReload();
    void FinishReload();
    void ConsumeAmmo();
    void TickWeapon(double deltaSeconds);
};
}
