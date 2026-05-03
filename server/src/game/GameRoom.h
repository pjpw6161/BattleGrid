#pragma once

#include "game/PlayerInput.h"
#include "game/PlayerState.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace battlegrid
{
class GameRoom
{
public:
    explicit GameRoom(std::uint64_t roomId);

    std::uint64_t GetRoomId() const;
    bool AddPlayer(std::uint64_t playerId, const std::string& nickname);
    bool RemovePlayer(std::uint64_t playerId);
    bool HasPlayer(std::uint64_t playerId) const;
    bool UpdateInput(std::uint64_t playerId, const PlayerInput& input);
    std::size_t GetPlayerCount() const;
    void Tick(double deltaSeconds, std::uint64_t tickNumber);
    nlohmann::json BuildSnapshotJson(std::uint64_t tickNumber) const;
    nlohmann::json ToDebugJson() const;

private:
    std::uint64_t roomId;
    std::unordered_map<std::uint64_t, PlayerState> players;
    mutable std::mutex mutex;
};
}
