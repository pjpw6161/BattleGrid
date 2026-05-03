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

    const PlayerInput input{
        sequence,
        moveX,
        moveY,
        aimX,
        aimY,
        fire
    };

    if (!room->UpdateInput(sessionState.playerId, input))
    {
        return JsonProtocol::Error("player not in room");
    }

    std::ostringstream logMessage;
    logMessage
        << "Stored input room_id=" << sessionState.roomId
        << " player_id=" << sessionState.playerId
        << " seq=" << sequence
        << " move_x=" << moveX
        << " move_y=" << moveY
        << " aim_x=" << aimX
        << " aim_y=" << aimY
        << " fire=" << (fire ? "true" : "false");

    Logger::Info(logMessage.str());

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
