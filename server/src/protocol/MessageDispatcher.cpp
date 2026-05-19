#include "protocol/MessageDispatcher.h"

#include "core/Logger.h"
#include "game/GameRoom.h"
#include "game/PlayerInput.h"
#include "game/RoomManager.h"
#include "protocol/JsonProtocol.h"

#include <nlohmann/json.hpp>

#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

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

bool IsUtf8KoreanSyllable(unsigned char first, unsigned char second, unsigned char third)
{
    const int codepoint =
        ((first & 0x0F) << 12)
        | ((second & 0x3F) << 6)
        | (third & 0x3F);
    return codepoint >= 0xAC00 && codepoint <= 0xD7A3;
}

bool ValidateNickname(const std::string& nickname, std::string& outReason)
{
    outReason.clear();
    if (nickname.empty())
    {
        outReason = "nickname is empty";
        return false;
    }

    std::size_t charCount = 0;
    for (std::size_t index = 0; index < nickname.size();)
    {
        const unsigned char c = static_cast<unsigned char>(nickname[index]);
        if (c <= 0x7F)
        {
            if (std::isspace(c))
            {
                outReason = "spaces are not allowed";
                return false;
            }
            const bool bAllowedAscii =
                (c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z')
                || (c >= '0' && c <= '9')
                || c == '_';
            if (!bAllowedAscii)
            {
                outReason = "nickname contains invalid characters";
                return false;
            }
            ++index;
        }
        else if (
            index + 2 < nickname.size()
            && (c & 0xF0) == 0xE0
            && (static_cast<unsigned char>(nickname[index + 1]) & 0xC0) == 0x80
            && (static_cast<unsigned char>(nickname[index + 2]) & 0xC0) == 0x80
            && IsUtf8KoreanSyllable(
                c,
                static_cast<unsigned char>(nickname[index + 1]),
                static_cast<unsigned char>(nickname[index + 2])
            )
        )
        {
            index += 3;
        }
        else
        {
            outReason = "nickname contains invalid characters";
            return false;
        }

        ++charCount;
        if (charCount > 10)
        {
            outReason = "nickname is too long";
            return false;
        }
    }

    return true;
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

std::optional<std::vector<ArenaPoint>> ReadMapMarkerArray(
    const nlohmann::json& message,
    const char* fieldName,
    const std::string& fallbackLabelPrefix
)
{
    const auto value = message.find(fieldName);
    if (value == message.end() || !value->is_array())
    {
        return std::nullopt;
    }

    std::vector<ArenaPoint> points;
    points.reserve(value->size());

    std::uint64_t nextId = 1;
    for (const nlohmann::json& markerJson : *value)
    {
        if (!markerJson.is_object())
        {
            return std::nullopt;
        }

        const double x = ReadDouble(markerJson, "x", std::numeric_limits<double>::quiet_NaN());
        const double y = ReadDouble(markerJson, "y", std::numeric_limits<double>::quiet_NaN());
        const double z = ReadDouble(markerJson, "z", std::numeric_limits<double>::quiet_NaN());
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        {
            return std::nullopt;
        }

        std::string label = ReadString(markerJson, "label", "");
        if (label.empty())
        {
            label = fallbackLabelPrefix + std::to_string(nextId);
        }

        ArenaPoint point;
        point.id = nextId++;
        point.label = label;
        point.x = x;
        point.y = y;
        point.z = z;
        points.push_back(std::move(point));
    }

    return points;
}
}

MessageDispatcher::MessageDispatcher(
    SessionState& inSessionState,
    std::shared_ptr<RoomManager> inRoomManager,
    std::function<std::uint64_t()> inAllocatePlayerId
)
    : sessionState(inSessionState),
      roomManager(std::move(inRoomManager)),
      allocatePlayerId(std::move(inAllocatePlayerId)),
      bVerboseInputLogs(false)
{
}

std::shared_ptr<GameRoom> MessageDispatcher::GetActiveRoomOrDefault() const
{
    if (!roomManager)
    {
        return nullptr;
    }
    if (sessionState.joined && sessionState.roomId != 0)
    {
        if (std::shared_ptr<GameRoom> room = roomManager->GetRoom(sessionState.roomId))
        {
            return room;
        }
    }
    return roomManager->GetDefaultRoom();
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

    if (*type == "set_nickname")
    {
        return HandleSetNickname(message);
    }

    if (*type == "list_rooms")
    {
        return HandleListRooms();
    }

    if (*type == "create_room")
    {
        return HandleCreateRoom();
    }

    if (*type == "join_room")
    {
        return HandleJoinRoom(message);
    }

    if (*type == "leave_room")
    {
        return HandleLeaveRoom(false);
    }

    if (*type == "leave_to_lobby")
    {
        return HandleLeaveRoom(true);
    }

    if (*type == "set_ready")
    {
        return HandleSetReady(message);
    }

    if (*type == "start_match")
    {
        return HandleStartMatch();
    }

    if (*type == "input")
    {
        return HandleInput(message);
    }

    if (*type == "client_hit_claim")
    {
        return HandleClientHitClaim(message);
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

    if (*type == "debug_apply_demo_preset")
    {
        return HandleDebugApplyDemoPreset(message);
    }

    if (*type == "debug_set_map_markers")
    {
        return HandleDebugSetMapMarkers(message);
    }

    if (*type == "debug_set_bot_attacks")
    {
        return HandleDebugSetBotAttacks(message);
    }

    if (*type == "debug_set_bot_difficulty")
    {
        return HandleDebugSetBotDifficulty(message);
    }

    if (*type == "debug_set_bot_config")
    {
        return HandleDebugSetBotConfig(message);
    }

    if (*type == "debug_set_hitbox_config")
    {
        return HandleDebugSetHitboxConfig(message);
    }

    if (*type == "debug_set_match_timer")
    {
        return HandleDebugSetMatchTimer(message);
    }

    return JsonProtocol::Error("unknown message type");
}

std::string MessageDispatcher::HandleSetNickname(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    const std::string nickname = ReadString(message, "nickname", "");
    std::string reason;
    if (!ValidateNickname(nickname, reason))
    {
        nlohmann::json response;
        response["type"] = "nickname_error";
        response["reason"] = reason;
        return response.dump();
    }

    if (sessionState.playerId == 0)
    {
        sessionState.playerId = allocatePlayerId ? allocatePlayerId() : 0;
    }

    if (!roomManager->RegisterNickname(sessionState.playerId, nickname, reason))
    {
        nlohmann::json response;
        response["type"] = "nickname_error";
        response["reason"] = reason.empty() ? "nickname unavailable" : reason;
        return response.dump();
    }

    sessionState.nickname = nickname;

    nlohmann::json response;
    response["type"] = "nickname_ok";
    response["player_id"] = sessionState.playerId;
    response["nickname"] = sessionState.nickname;

    Logger::Info(
        "Nickname accepted player_id=" + std::to_string(sessionState.playerId)
        + " nickname=" + sessionState.nickname
    );
    return response.dump();
}

std::string MessageDispatcher::HandleListRooms()
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    return roomManager->BuildRoomListMessageJson().dump();
}

std::string MessageDispatcher::HandleCreateRoom()
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }
    if (sessionState.playerId == 0 || sessionState.nickname.empty())
    {
        return JsonProtocol::Error("nickname required");
    }

    if (sessionState.joined && sessionState.roomId != 0)
    {
        roomManager->RemovePlayerFromRoom(sessionState.playerId, sessionState.roomId);
    }

    std::shared_ptr<GameRoom> room =
        roomManager->CreateRoom(sessionState.playerId, sessionState.nickname);
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    sessionState.roomId = room->GetRoomId();
    sessionState.joined = true;

    Logger::Info(
        "[BattleGrid] room_created room_id="
        + std::to_string(sessionState.roomId)
        + " host="
        + std::to_string(sessionState.playerId)
    );

    nlohmann::json response;
    response["type"] = "room_joined";
    response["room_id"] = sessionState.roomId;
    response["room_name"] = room->GetRoomName();
    response["host_player_id"] = room->GetHostPlayerId();
    return response.dump();
}

std::string MessageDispatcher::HandleJoinRoom(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }
    if (sessionState.playerId == 0 || sessionState.nickname.empty())
    {
        return JsonProtocol::Error("nickname required");
    }

    const std::uint64_t requestedRoomId = ReadUnsigned(message, "room_id", 0);
    std::shared_ptr<GameRoom> room = roomManager->GetRoom(requestedRoomId);
    if (!room)
    {
        return JsonProtocol::Error("room not found");
    }
    if (!room->CanJoinRoom())
    {
        return JsonProtocol::Error("room cannot be joined");
    }

    if (sessionState.joined && sessionState.roomId != 0 && sessionState.roomId != requestedRoomId)
    {
        roomManager->RemovePlayerFromRoom(sessionState.playerId, sessionState.roomId);
    }

    if (!room->AddPlayer(sessionState.playerId, sessionState.nickname))
    {
        if (!room->HasPlayer(sessionState.playerId))
        {
            return JsonProtocol::Error("room is full");
        }
    }

    sessionState.roomId = requestedRoomId;
    sessionState.joined = true;

    Logger::Info(
        "[BattleGrid] room_joined room_id="
        + std::to_string(sessionState.roomId)
        + " player="
        + std::to_string(sessionState.playerId)
    );

    nlohmann::json response;
    response["type"] = "room_joined";
    response["room_id"] = sessionState.roomId;
    response["room_name"] = room->GetRoomName();
    response["host_player_id"] = room->GetHostPlayerId();
    return response.dump();
}

std::string MessageDispatcher::HandleLeaveRoom(bool bLeaveToLobby)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    if (sessionState.joined && sessionState.roomId != 0)
    {
        roomManager->RemovePlayerFromRoom(sessionState.playerId, sessionState.roomId);
    }

    sessionState.joined = false;
    sessionState.roomId = 0;

    nlohmann::json response;
    response["type"] = bLeaveToLobby ? "left_to_lobby" : "room_left";
    response["room_list"] = roomManager->BuildRoomListMessageJson()["rooms"];
    return response.dump();
}

std::string MessageDispatcher::HandleSetReady(const nlohmann::json& message)
{
    if (!sessionState.joined || sessionState.roomId == 0)
    {
        return JsonProtocol::Error("not in room");
    }

    std::shared_ptr<GameRoom> room = roomManager ? roomManager->GetRoom(sessionState.roomId) : nullptr;
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    room->SetReady(sessionState.playerId, ReadBool(message, "ready", false));
    return room->BuildRoomStateJson().dump();
}

std::string MessageDispatcher::HandleStartMatch()
{
    if (!sessionState.joined || sessionState.roomId == 0)
    {
        return JsonProtocol::Error("not in room");
    }

    std::shared_ptr<GameRoom> room = roomManager ? roomManager->GetRoom(sessionState.roomId) : nullptr;
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::vector<std::string> notReadyPlayers;
    std::string reason;
    if (!room->StartMatchFromLobby(sessionState.playerId, notReadyPlayers, reason))
    {
        std::ostringstream failedLog;
        failedLog
            << "[BattleGrid] start_match failed room_id=" << sessionState.roomId
            << " reason=" << (reason.empty() ? "start_failed" : reason)
            << " not_ready=";
        for (std::size_t index = 0; index < notReadyPlayers.size(); ++index)
        {
            failedLog << (index == 0 ? "" : ",") << notReadyPlayers[index];
        }
        Logger::Info(failedLog.str());

        nlohmann::json response;
        response["type"] = "start_match_failed";
        response["reason"] = reason.empty() ? "start_failed" : reason;
        response["not_ready_players"] = notReadyPlayers;
        return response.dump();
    }

    Logger::Info(
        "[BattleGrid] start_match accepted room_id="
        + std::to_string(room->GetRoomId())
    );

    nlohmann::json response;
    response["type"] = "match_started";
    response["room_id"] = room->GetRoomId();
    response["duration"] = 600;
    response["target_score"] = 30;
    return response.dump();
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

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    sessionState.playerId = allocatePlayerId ? allocatePlayerId() : 0;
    sessionState.roomId = room->GetRoomId();
    sessionState.nickname = ExtractNickname(message);
    std::string nicknameReason;
    roomManager->RegisterNickname(sessionState.playerId, sessionState.nickname, nicknameReason);
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
    const bool hasClientWorldPosition =
        ReadBool(message, "has_client_world_position", false)
        || (
            message.contains("client_world_x")
            && message.contains("client_world_y")
            && message.contains("client_world_z")
        );
    const double clientWorldX = ReadDouble(message, "client_world_x", clientX);
    const double clientWorldY = ReadDouble(message, "client_world_y", clientY);
    const double clientWorldZ = ReadDouble(message, "client_world_z", clientZ);
    const bool hasClientYaw = message.contains("client_yaw");
    const double clientYaw = ReadDouble(message, "client_yaw", 0.0);
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

    std::shared_ptr<GameRoom> room = roomManager->GetRoom(sessionState.roomId);
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
    input.hasClientWorldPosition = hasClientWorldPosition;
    input.clientWorldX = clientWorldX;
    input.clientWorldY = clientWorldY;
    input.clientWorldZ = clientWorldZ;
    input.hasClientYaw = hasClientYaw;
    input.clientYaw = clientYaw;

    if (!room->UpdateInput(sessionState.playerId, input))
    {
        return JsonProtocol::Error("player not in room");
    }

    if (bVerboseInputLogs)
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
            << " has_client_world_position=" << (hasClientWorldPosition ? "true" : "false")
            << " client_world_position=(" << clientWorldX << "," << clientWorldY << "," << clientWorldZ << ")"
            << " client_yaw=" << clientYaw
            << " aim=(" << aimX << "," << aimY << ")";

        Logger::Info(logMessage.str());
    }

    return JsonProtocol::InputAck(sequence, sessionState.playerId);
}

std::string MessageDispatcher::HandleClientHitClaim(const nlohmann::json& message)
{
    if (!sessionState.joined)
    {
        return JsonProtocol::Error("not joined");
    }

    const std::uint64_t shotId = ReadUnsigned(message, "shot_id", 0);
    const std::uint64_t messagePlayerId = ReadUnsigned(message, "player_id", 0);
    const std::string targetType = ReadString(message, "target_type", "");
    const std::uint64_t botId = ReadUnsigned(message, "bot_id", 0);
    const std::uint64_t targetPlayerId = ReadUnsigned(message, "target_player_id", 0);
    const int damage = ReadInt(message, "damage", 0);
    const bool bHeadshot = ReadBool(message, "headshot", false);
    const double hitX = ReadDouble(message, "hit_x", std::numeric_limits<double>::quiet_NaN());
    const double hitY = ReadDouble(message, "hit_y", std::numeric_limits<double>::quiet_NaN());
    const double hitZ = ReadDouble(message, "hit_z", std::numeric_limits<double>::quiet_NaN());

    if (messagePlayerId != sessionState.playerId)
    {
        return JsonProtocol::Error("player_id mismatch");
    }

    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = roomManager->GetRoom(sessionState.roomId);
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::string reason;
    bool bAccepted = false;
    const bool bTargetsBot =
        targetType == "bot" || (targetType.empty() && botId > 0);
    const bool bTargetsPlayer =
        targetType == "player" || (targetType.empty() && targetPlayerId > 0);
    if (bTargetsBot)
    {
        bAccepted = room->ApplyClientBotHitClaim(
            sessionState.playerId,
            shotId,
            botId,
            damage,
            bHeadshot,
            hitX,
            hitY,
            hitZ,
            reason
        );
    }
    else if (bTargetsPlayer)
    {
        bAccepted = room->ApplyClientPlayerHitClaim(
            sessionState.playerId,
            shotId,
            targetPlayerId,
            damage,
            bHeadshot,
            hitX,
            hitY,
            hitZ,
            reason
        );
    }
    else
    {
        return JsonProtocol::Error("unsupported hit claim target");
    }

    nlohmann::json response;
    response["type"] = "client_hit_claim_ack";
    response["accepted"] = bAccepted;
    response["success"] = bAccepted;
    response["message"] = reason;
    response["shot_id"] = shotId;
    response["target_type"] = bTargetsPlayer ? "player" : (bTargetsBot ? "bot" : targetType);
    response["bot_id"] = botId;
    response["target_player_id"] = targetPlayerId;
    return response.dump();
}

std::string MessageDispatcher::HandleDebugRoom()
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    nlohmann::json roomState = room->ToDebugJson();
    roomState["type"] = "room_state";

    std::ostringstream logMessage;
    logMessage
        << "[BattleGridServer] debug_room runtime markers="
        << (roomState.value("has_runtime_map_markers", false) ? "true" : "false")
        << " shared=" << roomState.value("shared_spawn_count", 0)
        << " heal=" << roomState.value("heal_spawn_count", 0);
    Logger::Info(logMessage.str());

    return roomState.dump();
}

std::string MessageDispatcher::HandleDebugRestartMatch()
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
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

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    room->ApplySafeDemoMode();
    return JsonProtocol::DebugOk("safe demo mode applied");
}

std::string MessageDispatcher::HandleDebugApplyDemoPreset(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    const std::string presetName = ReadString(message, "preset", "");
    if (!room->ApplyDemoPreset(presetName))
    {
        return JsonProtocol::Error("invalid demo preset");
    }

    return JsonProtocol::DebugOk("demo preset applied: " + presetName);
}

std::string MessageDispatcher::HandleDebugSetMapMarkers(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    const std::string profileName = ReadString(message, "map_profile", "level_runtime");
    const auto sharedSpawnsJson = message.find("shared_spawns");
    const auto healSpawnsJson = message.find("heal_spawns");
    const std::size_t sharedSpawnCount =
        sharedSpawnsJson != message.end() && sharedSpawnsJson->is_array()
            ? sharedSpawnsJson->size()
            : 0;
    const std::size_t healSpawnCount =
        healSpawnsJson != message.end() && healSpawnsJson->is_array()
            ? healSpawnsJson->size()
            : 0;

    std::ostringstream receivedLog;
    receivedLog
        << "[BattleGridServer] Received debug_set_map_markers room_id="
        << room->GetRoomId()
        << " shared="
        << sharedSpawnCount
        << " heal=" << healSpawnCount
        << " profile=" << (profileName.empty() ? "level_runtime" : profileName);
    Logger::Info(receivedLog.str());

    const std::optional<std::vector<ArenaPoint>> sharedSpawns =
        ReadMapMarkerArray(message, "shared_spawns", "BG_Spawn_");
    const std::optional<std::vector<ArenaPoint>> healSpawns =
        ReadMapMarkerArray(message, "heal_spawns", "BG_HealSpawn_");
    if (!sharedSpawns || !healSpawns || sharedSpawns->empty() || healSpawns->empty())
    {
        return JsonProtocol::Error("invalid map markers");
    }

    if (!room->ApplyRuntimeMapMarkers(profileName.empty() ? "level_runtime" : profileName, *sharedSpawns, *healSpawns))
    {
        return JsonProtocol::Error("invalid map markers");
    }

    nlohmann::json response;
    response["type"] = "debug_ok";
    response["message"] = "map markers applied";
    response["room_id"] = room->GetRoomId();
    response["map_profile"] = profileName.empty() ? "level_runtime" : profileName;
    response["shared_spawn_count"] = sharedSpawns->size();
    response["heal_spawn_count"] = healSpawns->size();
    nlohmann::json roomDebug = room->ToDebugJson();
    response["arena_bounds"] = roomDebug["arena_bounds"];
    return response.dump();
}

std::string MessageDispatcher::HandleDebugSetBotAttacks(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
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

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
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

std::string MessageDispatcher::HandleDebugSetBotConfig(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    const int damage = ReadInt(message, "damage", -1);
    const int headDamage = ReadInt(message, "head", -1);
    const double cooldown = ReadDouble(message, "cooldown", -1.0);
    room->SetBotCombatConfig(damage, headDamage, cooldown);

    nlohmann::json response;
    response["type"] = "debug_ok";
    response["message"] = "bot config updated";
    response["damage"] = damage;
    response["head"] = headDamage;
    response["cooldown"] = cooldown;
    return response.dump();
}

std::string MessageDispatcher::HandleDebugSetHitboxConfig(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
    if (!room)
    {
        return JsonProtocol::Error("room unavailable");
    }

    const double botBodyScale = ReadDouble(message, "bot_body_scale", -1.0);
    const double botHeadScale = ReadDouble(message, "bot_head_scale", -1.0);
    const double playerBodyScale = ReadDouble(message, "player_body_scale", -1.0);
    const double playerHeadScale = ReadDouble(message, "player_head_scale", -1.0);
    room->SetHitboxConfig(
        botBodyScale,
        botHeadScale,
        playerBodyScale,
        playerHeadScale
    );

    nlohmann::json response;
    response["type"] = "debug_ok";
    response["message"] = "hitbox config updated";
    response["bot_body_scale"] = botBodyScale;
    response["bot_head_scale"] = botHeadScale;
    response["player_body_scale"] = playerBodyScale;
    response["player_head_scale"] = playerHeadScale;
    return response.dump();
}

std::string MessageDispatcher::HandleDebugSetMatchTimer(const nlohmann::json& message)
{
    if (!roomManager)
    {
        return JsonProtocol::Error("room unavailable");
    }

    std::shared_ptr<GameRoom> room = GetActiveRoomOrDefault();
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
