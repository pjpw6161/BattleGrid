#include "game/ProjectileState.h"

#include <cmath>

namespace battlegrid
{
void ProjectileState::NormalizeDirection()
{
    const double length = std::sqrt((dirX * dirX) + (dirY * dirY));
    if (length <= 0.0001)
    {
        dirX = 1.0;
        dirY = 0.0;
        return;
    }

    dirX /= length;
    dirY /= length;
}
}
