#pragma once

#include "game/GameRoom.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace battlegrid
{
class RoomManager
{
public:
    RoomManager();

    std::shared_ptr<GameRoom> GetDefaultRoom();
    std::shared_ptr<GameRoom> GetRoom(std::uint64_t roomId);
    std::uint64_t GetDefaultRoomId() const;
    std::shared_ptr<GameRoom> CreateRoom(
        std::uint64_t hostPlayerId,
        const std::string& hostNickname
    );
    bool RemovePlayerFromRoom(std::uint64_t playerId, std::uint64_t roomId);
    bool RegisterNickname(
        std::uint64_t playerId,
        const std::string& nickname,
        std::string& outReason
    );
    void UnregisterPlayer(std::uint64_t playerId);
    nlohmann::json BuildRoomListMessageJson() const;
    nlohmann::json BuildRoomStateJson(std::uint64_t roomId) const;
    nlohmann::json BuildRoomSnapshotJson(
        std::uint64_t roomId,
        std::uint64_t tickNumber
    ) const;
    void TickAll(double deltaSeconds, std::uint64_t tickNumber);
    nlohmann::json BuildDefaultRoomSnapshotJson(std::uint64_t tickNumber) const;

private:
    std::shared_ptr<GameRoom> defaultRoom;
    std::unordered_map<std::uint64_t, std::shared_ptr<GameRoom>> rooms;
    std::unordered_map<std::uint64_t, std::string> nicknamesByPlayer;
    std::unordered_map<std::string, std::uint64_t> playerByNickname;
    std::uint64_t nextRoomId;
};
}
