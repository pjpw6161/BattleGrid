#pragma once

#include "game/PlayerInput.h"

#include <cstdint>
#include <string>

namespace battlegrid
{
struct PlayerState
{
    PlayerState() = default;
    PlayerState(std::uint64_t playerId, std::uint64_t roomId, std::string nickname);

    std::uint64_t playerId = 0;
    std::uint64_t roomId = 0;
    std::string nickname;
    PlayerInput latestInput;
    bool connected = true;
};
}
