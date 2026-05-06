#include "protocol/JsonProtocol.h"

namespace battlegrid
{
std::optional<nlohmann::json> JsonProtocol::Parse(const std::string& text)
{
    try
    {
        return nlohmann::json::parse(text);
    }
    catch (const nlohmann::json::parse_error&)
    {
        return std::nullopt;
    }
}

std::optional<std::string> JsonProtocol::Type(const nlohmann::json& message)
{
    if (!message.is_object())
    {
        return std::nullopt;
    }

    const auto type = message.find("type");
    if (type == message.end() || !type->is_string())
    {
        return std::nullopt;
    }

    return type->get<std::string>();
}

std::string JsonProtocol::Error(const std::string& message)
{
    nlohmann::ordered_json response;
    response["type"] = "error";
    response["message"] = message;
    return response.dump();
}

std::string JsonProtocol::Pong()
{
    nlohmann::ordered_json response;
    response["type"] = "pong";
    return response.dump();
}

std::string JsonProtocol::JoinOk(
    std::uint64_t playerId,
    std::uint64_t roomId,
    const std::string& nickname
)
{
    nlohmann::ordered_json response;
    response["type"] = "join_ok";
    response["player_id"] = playerId;
    response["room_id"] = roomId;
    response["nickname"] = nickname;
    return response.dump();
}

std::string JsonProtocol::InputAck(std::uint64_t sequence, std::uint64_t playerId)
{
    nlohmann::ordered_json response;
    response["type"] = "input_ack";
    response["seq"] = sequence;
    response["player_id"] = playerId;
    return response.dump();
}

std::string JsonProtocol::MatchRestarted(std::uint64_t matchId)
{
    nlohmann::ordered_json response;
    response["type"] = "match_restarted";
    response["match_id"] = matchId;
    return response.dump();
}

std::string JsonProtocol::DebugOk(const std::string& message)
{
    nlohmann::ordered_json response;
    response["type"] = "debug_ok";
    response["message"] = message;
    return response.dump();
}

std::string JsonProtocol::RoomState(const nlohmann::json& roomState)
{
    nlohmann::ordered_json response;
    response["type"] = "room_state";
    response["room_id"] = roomState.value("room_id", 0);
    response["player_count"] = roomState.value("player_count", 0);
    response["projectile_count"] = roomState.value("projectile_count", 0);
    response["target_count"] = roomState.value("target_count", 0);
    response["bot_count"] = roomState.value("bot_count", 0);
    response["bot_attacks_enabled"] = roomState.value("bot_attacks_enabled", true);
    response["bot_difficulty"] = roomState.value(
        "bot_difficulty",
        std::string("normal")
    );
    response["bot_attack_damage"] = roomState.value("bot_attack_damage", 20);
    response["bot_attack_cooldown"] = roomState.value("bot_attack_cooldown", 1.0);
    response["bot_detect_range"] = roomState.value("bot_detect_range", 1500.0);
    response["bot_attack_range"] = roomState.value("bot_attack_range", 900.0);
    response["bot_move_speed"] = roomState.value("bot_move_speed", 500.0);
    response["auto_end_match_by_timer"] = roomState.value("auto_end_match_by_timer", true);
    response["health_pack_count"] = roomState.value("health_pack_count", 0);
    response["active_health_pack_count"] = roomState.value("active_health_pack_count", 0);
    response["match"] = roomState.contains("match")
        ? roomState.at("match")
        : nlohmann::json::object();
    response["scoreboard"] = roomState.contains("scoreboard")
        ? roomState.at("scoreboard")
        : nlohmann::json::array();
    response["events"] = roomState.contains("events")
        ? roomState.at("events")
        : nlohmann::json::array();
    response["players"] = roomState.contains("players")
        ? roomState.at("players")
        : nlohmann::json::array();
    response["bots"] = roomState.contains("bots")
        ? roomState.at("bots")
        : nlohmann::json::array();
    response["health_packs"] = roomState.contains("health_packs")
        ? roomState.at("health_packs")
        : nlohmann::json::array();
    response["projectiles"] = roomState.contains("projectiles")
        ? roomState.at("projectiles")
        : nlohmann::json::array();
    response["targets"] = roomState.contains("targets")
        ? roomState.at("targets")
        : nlohmann::json::array();
    return response.dump();
}
}
