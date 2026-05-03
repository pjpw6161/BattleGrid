#include "game/GameRoom.h"

#include "core/Logger.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace battlegrid
{
namespace
{
constexpr double ArenaMin = -2000.0;
constexpr double ArenaMax = 2000.0;

nlohmann::json BuildPlayerSnapshotJson(const PlayerState& player)
{
    nlohmann::json playerJson;
    playerJson["player_id"] = player.playerId;
    playerJson["nickname"] = player.nickname;
    playerJson["x"] = player.x;
    playerJson["y"] = player.y;
    playerJson["hp"] = player.hp;
    playerJson["score"] = player.score;
    playerJson["last_seq"] = player.latestInput.seq;
    return playerJson;
}
}

GameRoom::GameRoom(std::uint64_t inRoomId)
    : roomId(inRoomId),
      players(),
      mutex()
{
}

std::uint64_t GameRoom::GetRoomId() const
{
    return roomId;
}

bool GameRoom::AddPlayer(std::uint64_t playerId, const std::string& nickname)
{
    std::lock_guard lock(mutex);
    const auto [iterator, inserted] = players.emplace(
        playerId,
        PlayerState(playerId, roomId, nickname)
    );

    if (!inserted)
    {
        iterator->second.nickname = nickname;
        iterator->second.connected = true;
    }

    return inserted;
}

bool GameRoom::RemovePlayer(std::uint64_t playerId)
{
    std::lock_guard lock(mutex);
    return players.erase(playerId) > 0;
}

bool GameRoom::HasPlayer(std::uint64_t playerId) const
{
    std::lock_guard lock(mutex);
    return players.find(playerId) != players.end();
}

bool GameRoom::UpdateInput(std::uint64_t playerId, const PlayerInput& input)
{
    std::lock_guard lock(mutex);
    const auto player = players.find(playerId);
    if (player == players.end())
    {
        return false;
    }

    player->second.latestInput = input;
    return true;
}

std::size_t GameRoom::GetPlayerCount() const
{
    std::lock_guard lock(mutex);
    return players.size();
}

void GameRoom::Tick(double deltaSeconds, std::uint64_t tickNumber)
{
    std::lock_guard lock(mutex);
    for (auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);

        if (!player.connected)
        {
            continue;
        }

        double moveX = player.latestInput.moveX;
        double moveY = player.latestInput.moveY;
        const double length = std::sqrt((moveX * moveX) + (moveY * moveY));
        if (length > 1.0)
        {
            moveX /= length;
            moveY /= length;
        }

        player.x += moveX * player.speed * deltaSeconds;
        player.y += moveY * player.speed * deltaSeconds;
        player.x = std::clamp(player.x, ArenaMin, ArenaMax);
        player.y = std::clamp(player.y, ArenaMin, ArenaMax);

        if (tickNumber % 60 == 0)
        {
            std::ostringstream logMessage;
            logMessage
                << "Player position player_id=" << player.playerId
                << " x=" << player.x
                << " y=" << player.y
                << " move_x=" << moveX
                << " move_y=" << moveY;
            Logger::Info(logMessage.str());
        }
    }
}

nlohmann::json GameRoom::BuildSnapshotJson(std::uint64_t tickNumber) const
{
    std::lock_guard lock(mutex);

    nlohmann::json json;
    json["type"] = "snapshot";
    json["tick"] = tickNumber;
    json["room_id"] = roomId;
    json["players"] = nlohmann::json::array();

    for (const auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);

        if (!player.connected)
        {
            continue;
        }

        json["players"].push_back(BuildPlayerSnapshotJson(player));
    }

    return json;
}

nlohmann::json GameRoom::ToDebugJson() const
{
    std::lock_guard lock(mutex);

    nlohmann::json json;
    json["room_id"] = roomId;
    json["player_count"] = players.size();
    json["players"] = nlohmann::json::array();

    for (const auto& [playerId, player] : players)
    {
        const PlayerInput& input = player.latestInput;

        nlohmann::json inputJson;
        inputJson["seq"] = input.seq;
        inputJson["move_x"] = input.moveX;
        inputJson["move_y"] = input.moveY;
        inputJson["aim_x"] = input.aimX;
        inputJson["aim_y"] = input.aimY;
        inputJson["fire"] = input.fire;

        nlohmann::json playerJson;
        playerJson["player_id"] = playerId;
        playerJson["room_id"] = player.roomId;
        playerJson["nickname"] = player.nickname;
        playerJson["connected"] = player.connected;
        playerJson["x"] = player.x;
        playerJson["y"] = player.y;
        playerJson["speed"] = player.speed;
        playerJson["hp"] = player.hp;
        playerJson["score"] = player.score;
        playerJson["latest_input"] = std::move(inputJson);

        json["players"].push_back(std::move(playerJson));
    }

    return json;
}
}
