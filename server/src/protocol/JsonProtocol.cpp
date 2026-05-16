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
    response["bot_attack_damage"] = roomState.value("bot_attack_damage", 4);
    response["bot_headshot_damage"] = roomState.value("bot_headshot_damage", 8);
    response["bot_attack_cooldown"] = roomState.value("bot_attack_cooldown", 0.5);
    response["bot_config"] = roomState.value(
        "bot_config",
        nlohmann::json{
            {"damage", 4},
            {"head", 8},
            {"cooldown", 0.5},
        }
    );
    response["bot_body_hitbox_scale"] = roomState.value("bot_body_hitbox_scale", 0.64);
    response["bot_head_hitbox_scale"] = roomState.value("bot_head_hitbox_scale", 0.64);
    response["player_body_hitbox_scale"] =
        roomState.value("player_body_hitbox_scale", 0.64);
    response["player_headshot_hitbox_scale"] =
        roomState.value("player_headshot_hitbox_scale", 0.64);
    response["hitbox_config"] = roomState.value(
        "hitbox_config",
        nlohmann::json{
            {"bot_body_scale", 0.64},
            {"bot_head_scale", 0.64},
            {"player_body_scale", 0.64},
            {"player_head_scale", 0.64},
        }
    );
    response["bot_hitbox"] = roomState.value("bot_hitbox", nlohmann::json::object());
    response["player_hitbox"] = roomState.value("player_hitbox", nlohmann::json::object());
    response["bot_detect_range"] = roomState.value("bot_detect_range", 1000.0);
    response["bot_attack_range"] = roomState.value("bot_attack_range", 950.0);
    response["bot_move_speed"] = roomState.value("bot_move_speed", 500.0);
    response["bot_fire_interval"] = roomState.value("bot_fire_interval", 0.5);
    response["bot_fire_chance"] = roomState.value("bot_fire_chance", 0.55);
    response["bot_aim_spread"] = roomState.value("bot_aim_spread", 12.0);
    response["max_bots_targeting_one_player"] =
        roomState.value("max_bots_targeting_one_player", 4);
    response["max_bots_shooting_one_player"] =
        roomState.value("max_bots_shooting_one_player", 2);
    response["bot_wander_radius"] = roomState.value("bot_wander_radius", 350.0);
    response["bot_wander_step_min"] = roomState.value("bot_wander_step_min", 120.0);
    response["bot_wander_step_max"] = roomState.value("bot_wander_step_max", 350.0);
    response["bot_wander_wait_min"] = roomState.value("bot_wander_wait_min", 0.6);
    response["bot_wander_wait_max"] = roomState.value("bot_wander_wait_max", 1.8);
    response["bot_repath_interval"] = roomState.value("bot_repath_interval", 0.8);
    response["use_client_hit_claims_for_bots"] =
        roomState.value("use_client_hit_claims_for_bots", true);
    response["use_client_hit_claims_for_players"] =
        roomState.value("use_client_hit_claims_for_players", true);
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
