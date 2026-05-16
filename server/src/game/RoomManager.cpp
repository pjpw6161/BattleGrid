#include "game/RoomManager.h"

#include <algorithm>
#include <memory>
#include <string>

namespace battlegrid
{
namespace
{
constexpr std::uint64_t DefaultRoomId = 1;
}

RoomManager::RoomManager()
    : defaultRoom(std::make_shared<GameRoom>(DefaultRoomId)),
      rooms(),
      nicknamesByPlayer(),
      playerByNickname(),
      nextRoomId(DefaultRoomId + 1)
{
    rooms.emplace(DefaultRoomId, defaultRoom);
}

std::shared_ptr<GameRoom> RoomManager::GetDefaultRoom()
{
    return defaultRoom;
}

std::shared_ptr<GameRoom> RoomManager::GetRoom(std::uint64_t roomId)
{
    const auto room = rooms.find(roomId);
    return room != rooms.end() ? room->second : nullptr;
}

std::uint64_t RoomManager::GetDefaultRoomId() const
{
    return DefaultRoomId;
}

std::shared_ptr<GameRoom> RoomManager::CreateRoom(
    std::uint64_t hostPlayerId,
    const std::string& hostNickname
)
{
    const std::uint64_t roomId = nextRoomId++;
    std::shared_ptr<GameRoom> room = std::make_shared<GameRoom>(roomId);
    room->ConfigureLobbyRoom(hostNickname + "'s room", hostPlayerId, 6);
    room->AddPlayer(hostPlayerId, hostNickname);
    rooms.emplace(roomId, room);
    return room;
}

bool RoomManager::RemovePlayerFromRoom(std::uint64_t playerId, std::uint64_t roomId)
{
    std::shared_ptr<GameRoom> room = GetRoom(roomId);
    if (!room)
    {
        return false;
    }

    const bool bRemoved = room->RemovePlayer(playerId);
    if (roomId != DefaultRoomId && room->IsEmpty())
    {
        rooms.erase(roomId);
    }
    return bRemoved;
}

bool RoomManager::RegisterNickname(
    std::uint64_t playerId,
    const std::string& nickname,
    std::string& outReason
)
{
    outReason.clear();
    const auto existingForName = playerByNickname.find(nickname);
    if (existingForName != playerByNickname.end() && existingForName->second != playerId)
    {
        outReason = "nickname already in use";
        return false;
    }

    const auto existingForPlayer = nicknamesByPlayer.find(playerId);
    if (existingForPlayer != nicknamesByPlayer.end())
    {
        playerByNickname.erase(existingForPlayer->second);
    }

    nicknamesByPlayer[playerId] = nickname;
    playerByNickname[nickname] = playerId;
    return true;
}

void RoomManager::UnregisterPlayer(std::uint64_t playerId)
{
    const auto nickname = nicknamesByPlayer.find(playerId);
    if (nickname != nicknamesByPlayer.end())
    {
        playerByNickname.erase(nickname->second);
        nicknamesByPlayer.erase(nickname);
    }

    for (auto iterator = rooms.begin(); iterator != rooms.end();)
    {
        std::shared_ptr<GameRoom> room = iterator->second;
        if (room && room->HasPlayer(playerId))
        {
            room->RemovePlayer(playerId);
        }

        if (iterator->first != DefaultRoomId && room && room->IsEmpty())
        {
            iterator = rooms.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}

nlohmann::json RoomManager::BuildRoomListMessageJson() const
{
    nlohmann::json json;
    json["type"] = "room_list";
    json["rooms"] = nlohmann::json::array();
    for (const auto& [roomId, room] : rooms)
    {
        if (!room || roomId == DefaultRoomId)
        {
            continue;
        }
        json["rooms"].push_back(room->BuildRoomListEntryJson());
    }
    return json;
}

nlohmann::json RoomManager::BuildRoomStateJson(std::uint64_t roomId) const
{
    const auto room = rooms.find(roomId);
    if (room == rooms.end() || !room->second)
    {
        nlohmann::json json;
        json["type"] = "room_state";
        json["room_id"] = roomId;
        json["state"] = "missing";
        json["players"] = nlohmann::json::array();
        return json;
    }

    return room->second->BuildRoomStateJson();
}

nlohmann::json RoomManager::BuildRoomSnapshotJson(
    std::uint64_t roomId,
    std::uint64_t tickNumber
) const
{
    const auto room = rooms.find(roomId);
    return room != rooms.end() && room->second
        ? room->second->BuildSnapshotJson(tickNumber)
        : nlohmann::json::object();
}

void RoomManager::TickAll(double deltaSeconds, std::uint64_t tickNumber)
{
    for (const auto& [roomId, room] : rooms)
    {
        static_cast<void>(roomId);
        if (room)
        {
            room->Tick(deltaSeconds, tickNumber);
        }
    }
}

nlohmann::json RoomManager::BuildDefaultRoomSnapshotJson(std::uint64_t tickNumber) const
{
    return defaultRoom->BuildSnapshotJson(tickNumber);
}
}
