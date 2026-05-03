#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace battlegrid
{
class JsonProtocol
{
public:
    static std::optional<nlohmann::json> Parse(const std::string& text);
    static std::optional<std::string> Type(const nlohmann::json& message);

    static std::string Error(const std::string& message);
    static std::string Pong();
    static std::string JoinOk(
        std::uint64_t playerId,
        std::uint64_t roomId,
        const std::string& nickname
    );
    static std::string InputAck(std::uint64_t sequence, std::uint64_t playerId);
    static std::string RoomState(const nlohmann::json& roomState);
};
}
