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

std::string ReadString(
    const nlohmann::json& message,
    const char* fieldName,
    const std::string& defaultValue
)
{
    const auto value = message.find(fieldName);
    if (value == message.end() || !value->is_string())
    {
        return defaultValue;
    }

    return value->get<std::string>();
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

    if (*type == "debug_restart_match")
    {
        return HandleDebugRestartMatch();
    }

    if (*type == "debug_apply_demo_mode")
    {
        return HandleDebugApplyDemoMode();
    }

    if (*type == "debug_set_bot_attacks")
    {
        return HandleDebugSetBotAttacks(message);
    }

    if (*type == "debug_set_bot_difficulty")
    {
        return HandleDebugSetBotDifficulty(message);
    }

    if (*type == "debug_set_match_timer")
    {
        return HandleDebugSetMatchTimer(message);
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
    const bool hasFireOrigin = ReadBool(message, "has_fire_origin", false);
    const double fireOriginX = ReadDouble(message, "fire_origin_x", 0.0);
    const double fireOriginY = ReadDouble(message, "fire_origin_y", 0.0);
    const double fireOriginZ = ReadDouble(message, "fire_origin_z", 0.0);
    const bool hasClientPosition = ReadBool(message, "has_client_position", false);
    const double clientX = ReadDouble(message, "client_x", 0.0);
    const double clientY = ReadDouble(message, "client_y", 0.0);
    const double clientZ = ReadDouble(message, "client_z", 0.0);
    double shotDirX = ReadDouble(message, "shot_dir_x", aimX);
    double shotDirY = ReadDouble(message, "shot_dir_y", aimY);
    double shotDirZ = ReadDouble(message, "shot_dir_z", 0.0);
    const double shotDirection2dLengthSquared =
        (shotDirX * shotDirX) + (shotDirY * shotDirY);
    if (shotDirection2dLengthSquared <= 0.0001)
    {
        shotDirX = aimX;
        shotDirY = aimY;
        shotDirZ = 0.0;
    }

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
    input.hasFireOrigin = hasFireOrigin;
    input.fireOriginX = fireOriginX;
    input.fireOriginY = fireOriginY;
    input.fireOriginZ = fireOriginZ;
    input.hasClientPosition = hasClientPosition;
    input.clientX = clientX;
    input.clientY = clientY;
    input.clientZ = clientZ;

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
            << " spread=" << spreadDegrees
            << " shot_dir=(" << shotDirX << "," << shotDirY << "," << shotDirZ << ")"
            << " has_fire_origin=" << (hasFireOrigin ? "true" : "false")
            << " fire_origin=(" << fireOriginX << "," << fireOriginY << "," << fireOriginZ << ")"
            << " has_client_position=" << (hasClientPosition ? "true" : "false")
            << " client_position=(" << clientX << "," << clientY << "," << clientZ << ")"
            << " aim=(" << aimX << "," << aimY << ")";

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

std::string MessageDispatcher::HandleDebugRestartMatch()
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

    const std::uint64_t matchId = room->ResetMatch();
    return JsonProtocol::MatchRestarted(matchId);
}

std::string MessageDispatcher::HandleDebugApplyDemoMode()
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

    room->ApplySafeDemoMode();
    return JsonProtocol::DebugOk("safe demo mode applied");
}

std::string MessageDispatcher::HandleDebugSetBotAttacks(const nlohmann::json& message)
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

    const bool bEnabled = ReadBool(message, "enabled", true);
    room->SetBotAttacksEnabled(bEnabled);

    return JsonProtocol::DebugOk(
        bEnabled ? "bot attacks enabled" : "bot attacks disabled"
    );
}

std::string MessageDispatcher::HandleDebugSetBotDifficulty(const nlohmann::json& message)
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

    const std::string difficulty = ReadString(message, "difficulty", "normal");
    if (difficulty != "easy" && difficulty != "normal" && difficulty != "hard")
    {
        return JsonProtocol::Error("invalid bot difficulty");
    }

    room->ApplyBotDifficulty(difficulty);

    return JsonProtocol::DebugOk("bot difficulty set to " + difficulty);
}

std::string MessageDispatcher::HandleDebugSetMatchTimer(const nlohmann::json& message)
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

    const bool bEnabled = ReadBool(message, "enabled", true);
    room->SetAutoEndMatchByTimer(bEnabled);

    return JsonProtocol::DebugOk(
        bEnabled
            ? "auto end match by timer enabled"
            : "auto end match by timer disabled"
    );
}
}
