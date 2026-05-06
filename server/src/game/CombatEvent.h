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
    double serverTimeSeconds = 0.0;
    std::uint64_t actorPlayerId = 0;
    std::uint64_t targetPlayerId = 0;
    std::uint64_t botId = 0;
    std::uint64_t targetId = 0;
    std::uint64_t healthPackId = 0;
    bool headshot = false;

    nlohmann::json ToJson() const;
};
}
