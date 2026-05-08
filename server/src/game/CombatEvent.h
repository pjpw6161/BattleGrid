#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace battlegrid
{
struct CombatEvent
{
    std::uint64_t eventId = 0;
    std::string type;
    std::string message;
    std::string shortMessage;
    double serverTimeSeconds = 0.0;
    std::uint64_t actorPlayerId = 0;
    std::uint64_t targetPlayerId = 0;
    std::uint64_t botId = 0;
    std::uint64_t targetId = 0;
    std::uint64_t healthPackId = 0;
    bool headshot = false;
    bool victimIsPlayer = false;
    bool victimIsBot = false;
    bool killerIsBot = false;
    bool killerIsPlayer = false;
    int damage = 0;
    int healAmount = 0;
    std::string hitGroup;
    double hitX = 0.0;
    double hitY = 0.0;
    double hitZ = 0.0;

    nlohmann::json ToJson() const;
};
}
