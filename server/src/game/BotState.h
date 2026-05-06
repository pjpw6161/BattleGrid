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
    double bodyRadius = 60.0;
    double headRadius = 35.0;
    double bodyHeight = 90.0;
    double headHeight = 160.0;
    std::uint64_t targetPlayerId = 0;
    double wanderTargetX = 0.0;
    double wanderTargetY = 0.0;
    double decisionTimerSeconds = 0.0;
    double attackCooldownSeconds = 1.0;
    double attackTimerSeconds = 0.0;
    bool connected = true;

    bool IsAlive() const;
    bool CanBeDamaged() const;
    void ApplyDamage(int damage);
    void Kill();
    void Respawn(double newX, double newY);
};
}
