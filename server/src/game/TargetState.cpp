#include "game/TargetState.h"

#include <algorithm>

namespace battlegrid
{
bool TargetState::IsAlive() const
{
    return alive && hp > 0;
}

void TargetState::ApplyDamage(int damage)
{
    if (!IsAlive())
    {
        return;
    }

    hp = std::max(0, hp - std::max(0, damage));
    if (hp <= 0)
    {
        alive = false;
    }
}
}
