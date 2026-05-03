#include "protocol/MessageDispatcher.h"

#include "core/Logger.h"
#include "protocol/JsonProtocol.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <sstream>

namespace battlegrid
{
namespace
{
constexpr std::uint64_t FixedRoomId = 1;

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

std::int64_t ReadInteger(
    const nlohmann::json& message,
    const char* fieldName,
    std::int64_t defaultValue
)
{
    const auto value = message.find(fieldName);
    if (value == message.end() || !value->is_number())
    {
        return defaultValue;
    }

    return static_cast<std::int64_t>(value->get<double>());
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

MessageDispatcher::MessageDispatcher(std::uint64_t sessionPlayerId)
    : playerId(sessionPlayerId)
{
}

std::string MessageDispatcher::Dispatch(const std::string& text) const
{
    const std::optional<nlohmann::json> parsed = JsonProtocol::Parse(text);
    if (!parsed)
    {
        return JsonProtocol::Error("invalid json");
    }

    return DispatchParsedMessage(*parsed);
}

std::string MessageDispatcher::DispatchParsedMessage(const nlohmann::json& message) const
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

    return JsonProtocol::Error("unknown message type");
}

std::string MessageDispatcher::HandleJoin(const nlohmann::json& message) const
{
    return JsonProtocol::JoinOk(playerId, FixedRoomId, ExtractNickname(message));
}

std::string MessageDispatcher::HandleInput(const nlohmann::json& message) const
{
    const std::int64_t sequence = ReadInteger(message, "seq", 0);
    const std::int64_t messagePlayerId = ReadInteger(message, "player_id", 0);
    const double moveX = ReadDouble(message, "move_x", 0.0);
    const double moveY = ReadDouble(message, "move_y", 0.0);
    const double aimX = ReadDouble(message, "aim_x", 0.0);
    const double aimY = ReadDouble(message, "aim_y", 0.0);
    const bool fire = ReadBool(message, "fire", false);

    std::ostringstream logMessage;
    logMessage
        << "Input player_id=" << messagePlayerId
        << " seq=" << sequence
        << " move_x=" << moveX
        << " move_y=" << moveY
        << " aim_x=" << aimX
        << " aim_y=" << aimY
        << " fire=" << (fire ? "true" : "false");

    Logger::Info(logMessage.str());

    return JsonProtocol::InputAck(sequence);
}
}
