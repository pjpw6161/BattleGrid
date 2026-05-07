#include "game/ProjectileState.h"

#include <cmath>

namespace battlegrid
{
void ProjectileState::NormalizeDirection()
{
    const double length = std::sqrt((dirX * dirX) + (dirY * dirY) + (dirZ * dirZ));
    if (length <= 0.0001)
    {
        dirX = 1.0;
        dirY = 0.0;
        dirZ = 0.0;
        return;
    }

    dirX /= length;
    dirY /= length;
    dirZ /= length;
}
}
