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

std::string JsonProtocol::RoomState(const nlohmann::json& roomState)
{
    nlohmann::ordered_json response;
    response["type"] = "room_state";
    response["room_id"] = roomState.value("room_id", 0);
    response["player_count"] = roomState.value("player_count", 0);
    response["projectile_count"] = roomState.value("projectile_count", 0);
    response["players"] = roomState.contains("players")
        ? roomState.at("players")
        : nlohmann::json::array();
    response["projectiles"] = roomState.contains("projectiles")
        ? roomState.at("projectiles")
        : nlohmann::json::array();
    return response.dump();
}
}
