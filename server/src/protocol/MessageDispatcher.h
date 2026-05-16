#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace battlegrid
{
class GameRoom;
class RoomManager;

struct SessionState
{
    std::uint64_t playerId = 0;
    std::uint64_t roomId = 0;
    std::string nickname;
    bool joined = false;
};

class MessageDispatcher
{
public:
    MessageDispatcher(
        SessionState& sessionState,
        std::shared_ptr<RoomManager> roomManager,
        std::function<std::uint64_t()> allocatePlayerId
    );

    std::string Dispatch(const std::string& text);

private:
    std::string DispatchParsedMessage(const nlohmann::json& message);
    std::string HandleSetNickname(const nlohmann::json& message);
    std::string HandleListRooms();
    std::string HandleCreateRoom();
    std::string HandleJoinRoom(const nlohmann::json& message);
    std::string HandleLeaveRoom(bool bLeaveToLobby);
    std::string HandleSetReady(const nlohmann::json& message);
    std::string HandleStartMatch();
    std::string HandleJoin(const nlohmann::json& message);
    std::string HandleInput(const nlohmann::json& message);
    std::string HandleClientHitClaim(const nlohmann::json& message);
    std::string HandleDebugRoom();
    std::string HandleDebugRestartMatch();
    std::string HandleDebugApplyDemoMode();
    std::string HandleDebugApplyDemoPreset(const nlohmann::json& message);
    std::string HandleDebugSetMapMarkers(const nlohmann::json& message);
    std::string HandleDebugSetBotAttacks(const nlohmann::json& message);
    std::string HandleDebugSetBotDifficulty(const nlohmann::json& message);
    std::string HandleDebugSetBotConfig(const nlohmann::json& message);
    std::string HandleDebugSetHitboxConfig(const nlohmann::json& message);
    std::string HandleDebugSetMatchTimer(const nlohmann::json& message);
    std::shared_ptr<GameRoom> GetActiveRoomOrDefault() const;

    SessionState& sessionState;
    std::shared_ptr<RoomManager> roomManager;
    std::function<std::uint64_t()> allocatePlayerId;
    bool bVerboseInputLogs;
};
}
