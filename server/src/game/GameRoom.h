#pragma once

#include "game/PlayerInput.h"
#include "game/PlayerState.h"
#include "game/BotState.h"
#include "game/CombatEvent.h"
#include "game/HealthPackState.h"
#include "game/MatchState.h"
#include "game/ProjectileState.h"
#include "game/TargetState.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <utility>
#include <vector>
#include <string>
#include <unordered_map>

namespace battlegrid
{
class GameRoom
{
public:
    explicit GameRoom(std::uint64_t roomId);

    std::uint64_t GetRoomId() const;
    bool AddPlayer(std::uint64_t playerId, const std::string& nickname);
    bool RemovePlayer(std::uint64_t playerId);
    bool HasPlayer(std::uint64_t playerId) const;
    bool UpdateInput(std::uint64_t playerId, const PlayerInput& input);
    std::size_t GetPlayerCount() const;
    void Tick(double deltaSeconds, std::uint64_t tickNumber);
    nlohmann::json BuildSnapshotJson(std::uint64_t tickNumber) const;
    nlohmann::json ToDebugJson() const;
    std::uint64_t ResetMatch();
    std::uint64_t ApplySafeDemoMode();
    bool ApplyDemoPreset(const std::string& presetName);
    void SetBotAttacksEnabled(bool bEnabled);
    bool AreBotAttacksEnabled() const;
    void ApplyBotDifficulty(const std::string& difficulty);
    void SetAutoEndMatchByTimer(bool bEnabled);

private:
    void InitializeDefaultTargets() const;
    void InitializeDefaultBots() const;
    void InitializeHealthPackSpawnPoints() const;
    void InitializeDefaultHealthPacks() const;
    void SpawnProjectile(std::uint64_t ownerPlayerId, double dirX, double dirY, double dirZ);
    void SpawnBotProjectile(const BotState& bot, double dirX, double dirY, double dirZ);
    void CreateVisualTracer(
        const std::string& ownerType,
        std::uint64_t ownerPlayerId,
        std::uint64_t ownerBotId,
        double startX,
        double startY,
        double startZ,
        double dirX,
        double dirY,
        double dirZ,
        double rangeOrLength,
        double lifeTimeSeconds
    );
    void ProcessPlayerRespawns(double deltaSeconds);
    void UpdateBotRespawns(double deltaSeconds);
    void UpdateBots(double deltaSeconds);
    void UpdateBotAI(double deltaSeconds);
    void ProcessBotShot(BotState& bot, PlayerState& targetPlayer);
    bool IsInsideBotArea(double x, double y) const;
    double ClampBotX(double x) const;
    double ClampBotY(double y) const;
    std::pair<double, double> ClampToBotArea(double x, double y) const;
    void UpdateHealthPacks(double deltaSeconds);
    void CheckHealthPackPickups();
    std::pair<double, double> ChooseHealthPackSpawnPoint(std::uint64_t healthPackId) const;
    void AddCombatEvent(const std::string& type, const std::string& message);
    void AddCombatEvent(const CombatEvent& event);
    nlohmann::json BuildEventsJson() const;
    void ProcessHitscanFire(PlayerState& shooter, const PlayerInput& input);
    void CheckMatchEndCondition();
    std::uint64_t DetermineWinnerPlayerId() const;
    nlohmann::json BuildScoreboardJson() const;
    std::uint64_t ResetRoomForDemoUnlocked(
        const std::string& primaryEventType,
        const std::string& primaryEventMessage,
        bool bAddMatchRestartedEvent,
        bool bAddMatchStartedEvent
    );
    void ApplyBotDifficultyUnlocked(const std::string& difficulty);
    void UpdateProjectiles(double deltaSeconds);
    void UpdateProjectileTargetCollisions();
    void RemoveInactiveProjectiles();

    std::uint64_t roomId;
    std::unordered_map<std::uint64_t, PlayerState> players;
    MatchState matchState;
    mutable std::unordered_map<std::uint64_t, BotState> bots;
    mutable std::unordered_map<std::uint64_t, HealthPackState> healthPacks;
    std::unordered_map<std::uint64_t, ProjectileState> projectiles;
    mutable std::unordered_map<std::uint64_t, TargetState> targets;
    mutable std::vector<std::pair<double, double>> healthPackSpawnPoints;
    std::uint64_t nextProjectileId;
    std::uint64_t healthPackRespawnCounter;
    std::deque<CombatEvent> recentEvents;
    std::uint64_t nextEventId;
    std::size_t MaxRecentEvents;
    double serverTimeSeconds;
    bool bBotAttacksEnabled;
    std::string botDifficulty;
    std::string currentDemoPreset;
    bool bAutoEndMatchByTimer;
    bool bTargetsEnabled;
    double botDetectRange;
    double botAttackRange;
    int botAttackDamage;
    int botHeadshotDamage;
    double botAttackCooldownSeconds;
    double botMoveSpeed;
    double botFireIntervalSeconds;
    double botAimSpreadDegrees;
    bool bVerboseBotShotEvents;
    bool bVerboseInputLogs;
    bool bEnableBot2DFallbackHit;
    double Bot2DFallbackRadiusScale;
    bool bVerboseHitscanCandidateLogs;
    bool bVerboseBotStateLogs;
    bool bUseClientFireOriginForHitscan;
    double ClientFireOriginWarningDistance;
    double MaxAcceptedClientFireOriginDistance;
    bool bUseClientPositionForPlayerMovement;
    double MaxClientPositionDeltaPerSecond;
    double MaxClientPositionSnapDistance;
    int MaxBotsTargetingOnePlayer;
    int MaxBotsShootingOnePlayer;
    double BotTargetReconsiderSeconds;
    double BotShotRandomDelayMin;
    double BotShotRandomDelayMax;
    double BotRecentDamageGraceSeconds;
    double respawnInvincibleSeconds;
    mutable bool targetsInitialized;
    mutable bool botsInitialized;
    mutable bool healthPacksInitialized;
    mutable std::mutex mutex;
};
}
