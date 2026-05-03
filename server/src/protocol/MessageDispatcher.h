#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace battlegrid
{
class MessageDispatcher
{
public:
    explicit MessageDispatcher(std::uint64_t playerId);

    std::string Dispatch(const std::string& text) const;

private:
    std::string DispatchParsedMessage(const nlohmann::json& message) const;
    std::string HandleJoin(const nlohmann::json& message) const;
    std::string HandleInput(const nlohmann::json& message) const;

    std::uint64_t playerId;
};
}
