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
    bool alive = true;
    bool invincible = false;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double speed = 600.0;
    int maxHp = 100;
    int hp = 100;
    int score = 0;
    int deaths = 0;
    int kills = 0;
    int playerKills = 0;
    int targetKills = 0;
    int botKills = 0;
    double respawnTimerSeconds = 0.0;
    double invincibleTimerSeconds = 0.0;
    double bodyRadius = 60.0;
    double headRadius = 35.0;
    double bodyHeight = 90.0;
    double headHeight = 160.0;
    std::uint64_t lastProcessedFireSeq = 0;
};
}
