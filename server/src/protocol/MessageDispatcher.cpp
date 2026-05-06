#include "protocol/MessageDispatcher.h"

#include "core/Logger.h"
#include "game/GameRoom.h"
#include "game/PlayerInput.h"
#include "game/RoomManager.h"
#include "protocol/JsonProtocol.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <sstream>
#include <utility>

namespace battlegrid
{
namespace
{
std::string ExtractNickname(const nlohmann::json& message)
{
    const auto nickname = message.find("nickname");
    if (nickname == message.end() || !nickname->is_string())
    {
        return "anonymous";
    }

    const std::string value = nickname->get<std::string>();
    return value.empty() ? std::string("anonymous") : value;
}

std::uint64_t ReadUnsigned(
    const nlohmann::json& message,
    const char* fieldName,
    std::uint64_t defaultValue
)
{
    const auto value = message.find(fieldName);
    if (value == message.end() || !value->is_number())
    {
        return defaultValue;
    }

    const double number = value->get<double>();
    if (number < 0.0)
    {
        return defaultValue;
    }

    return static_cast<std::uint64_t>(number);
}

double ReadDouble(
    const nlohmann::json& message,
    const char* fieldName,
    double defaultValue
)
{
    const auto value = message.find(fieldName);
    if (value == message.end() || !value->is_number())
    {
        return defaultValue;
    }

    return value->get<double>();
}

bool ReadBool(
    const nlohmann::json& message,
    const char* fieldName,
    bool defaultValue
)
{
    const auto value = message.find(fieldName);
    if (value == message.end() || !value->is_boolean())
    {
        return defaultValue;
    }

    return value->get<bool>();
}

int ReadInt(
    const nlohmann::json& message,
    const char* fieldName,
    int defaultValue
)
{
    const auto value = message.find(fieldName);
    if (value == message.end() || !value->is_number())
    {
        return defaultValue;
    }

    return static_cast<int>(value->get<double>());
}
}

MessageDispatcher::MessageDispatcher(
    SessionState& inSessionState,
    std::shared_ptr<RoomManager> inRoomManager,
    std::function<std::uint64_t()> inAllocatePlayerId
)
    : sessionState(inSessionState),
      roomManager(std::move(inRoomManager)),
      allocatePlayerId(std::move(inAllocatePlayerId))
{
}

std::string MessageDispatcher::Dispatch(const std::string& text)
{
    const std::optional<nlohmann::json> parsed = JsonProtocol::Parse(text);
    if (!parsed)
    {
        return JsonProtocol::Error("invalid json");
    }

    return DispatchParsedMessage(*parsed);
}

std::string MessageDispatcher::DispatchParsedMessage(const nlohmann::json& message)
{
    const std::optional<std::string> type = JsonProtocol::Type(message);
    if (!type)
    {
        return JsonProtocol::Error("unknown message type");
    }

    if (*type == "ping")
    {
        return JsonProtocol::Pong();
    }

    if (*type == "join")
    {
        return HandleJoin(message);
    }

    if (*type == "input")
    {
        return HandleInput(message);
    }

    if (*type == "debug_room")
    {
        return HandleDebugRoom();
    }

    return JsonProtocol::Error("unknown message type");
}

std::string MessageDispatcher::HandleJoin(const nlohmann::json& message)
{
    if (sessionState.joined)
    {
        return JsonProtocol::JoinOk(
            sessionState.playerId,
            sessionState.roomId,
            sessionState.nickname
        );
    }

    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = roomManager->GetDefaultRoom();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    sessionState.playerId = allocatePlayerId ? allocatePlayerId() : 0;
    sessionState.roomId = room->GetRoomId();
    sessionState.nickname = ExtractNickname(message);
    sessionState.joined = true;

    room->AddPlayer(sessionState.playerId, sessionState.nickname);

    Logger::Info(
        "Player joined room_id=" + std::to_string(sessionState.roomId)
        + " player_id=" + std::to_string(sessionState.playerId)
        + " nickname=" + sessionState.nickname
    );

    return JsonProtocol::JoinOk(
        sessionState.playerId,
        sessionState.roomId,
        sessionState.nickname
    );
}

std::string MessageDispatcher::HandleInput(const nlohmann::json& message)
{
    if (!sessionState.joined)
    {
        return JsonProtocol::Error("not joined");
    }

    const std::uint64_t sequence = ReadUnsigned(message, "seq", 0);
    const std::uint64_t messagePlayerId = ReadUnsigned(message, "player_id", 0);
    const double moveX = ReadDouble(message, "move_x", 0.0);
    const double moveY = ReadDouble(message, "move_y", 0.0);
    const double aimX = ReadDouble(message, "aim_x", 0.0);
    const double aimY = ReadDouble(message, "aim_y", 0.0);
    const bool fire = ReadBool(message, "fire", false);
    const bool reload = ReadBool(message, "reload", false);
    const bool ads = ReadBool(message, "ads", false);
    const bool sprint = ReadBool(message, "sprint", false);
    const bool jump = ReadBool(message, "jump", false);
    const int ammo = ReadInt(message, "ammo", 0);
    const double spreadDegrees = ReadDouble(message, "spread_deg", 0.0);
    const double shotDirX = ReadDouble(message, "shot_dir_x", aimX);
    const double shotDirY = ReadDouble(message, "shot_dir_y", aimY);
    const double shotDirZ = ReadDouble(message, "shot_dir_z", 0.0);

    if (messagePlayerId != sessionState.playerId)
    {
        return JsonProtocol::Error("player_id mismatch");
    }

    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = roomManager->GetDefaultRoom();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    PlayerInput input;
    input.seq = sequence;
    input.moveX = moveX;
    input.moveY = moveY;
    input.aimX = aimX;
    input.aimY = aimY;
    input.fire = fire;
    input.reload = reload;
    input.ads = ads;
    input.sprint = sprint;
    input.jump = jump;
    input.ammo = ammo;
    input.spreadDegrees = spreadDegrees;
    input.shotDirX = shotDirX;
    input.shotDirY = shotDirY;
    input.shotDirZ = shotDirZ;

    if (!room->UpdateInput(sessionState.playerId, input))
    {
        return JsonProtocol::Error("player not in room");
    }

    if (fire || reload || sequence <= 3 || sequence % 60 == 0)
    {
        std::ostringstream logMessage;
        logMessage
            << "Stored input player_id=" << sessionState.playerId
            << " seq=" << sequence
            << " fire=" << (fire ? "true" : "false")
            << " reload=" << (reload ? "true" : "false")
            << " ads=" << (ads ? "true" : "false")
            << " sprint=" << (sprint ? "true" : "false")
            << " jump=" << (jump ? "true" : "false")
            << " ammo=" << ammo
            << " spread=" << spreadDegrees;

        Logger::Info(logMessage.str());
    }

    return JsonProtocol::InputAck(sequence, sessionState.playerId);
}

std::string MessageDispatcher::HandleDebugRoom()
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = roomManager->GetDefaultRoom();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    return JsonProtocol::RoomState(room->ToDebugJson());
}
}
