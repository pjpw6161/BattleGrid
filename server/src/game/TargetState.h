#pragma once

#include <cstdint>

namespace battlegrid
{
struct TargetState
{
    std::uint64_t targetId = 0;
    double x = 0.0;
    double y = 0.0;
    int hp = 100;
    int maxHp = 100;
    double radius = 80.0;
    bool alive = true;

    bool IsAlive() const;
    void ApplyDamage(int damage);
};
}
