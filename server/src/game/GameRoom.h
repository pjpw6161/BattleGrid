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
#include <random>
#include <utility>
#include <vector>
#include <string>
#include <unordered_map>

namespace battlegrid
{
struct ArenaBounds2D
{
    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;
};

struct ArenaPoint
{
    std::uint64_t id = 0;
    std::string label;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct HealthPackInitialSpawn
{
    std::uint64_t healthPackId = 0;
    std::size_t spawnPointIndex = 0;
};

class GameRoom
{
public:
    explicit GameRoom(std::uint64_t roomId);

    std::uint64_t GetRoomId() const;
    std::string GetRoomName() const;
    std::string GetRoomFlowState() const;
    std::uint64_t GetHostPlayerId() const;
    std::size_t GetMaxPlayers() const;
    void ConfigureLobbyRoom(
        const std::string& inRoomName,
        std::uint64_t inHostPlayerId,
        std::size_t inMaxPlayers
    );
    bool AddPlayer(std::uint64_t playerId, const std::string& nickname);
    bool RemovePlayer(std::uint64_t playerId);
    bool HasPlayer(std::uint64_t playerId) const;
    bool CanJoinRoom() const;
    bool IsEmpty() const;
    bool SetReady(std::uint64_t playerId, bool bReady);
    bool StartMatchFromLobby(
        std::uint64_t requestingPlayerId,
        std::vector<std::string>& outNotReadyPlayers,
        std::string& outReason
    );
    nlohmann::json BuildRoomListEntryJson() const;
    nlohmann::json BuildRoomStateJson() const;
    bool UpdateInput(std::uint64_t playerId, const PlayerInput& input);
    std::size_t GetPlayerCount() const;
    void Tick(double deltaSeconds, std::uint64_t tickNumber);
    nlohmann::json BuildSnapshotJson(std::uint64_t tickNumber) const;
    nlohmann::json ToDebugJson() const;
    std::uint64_t ResetMatch();
    std::uint64_t ApplySafeDemoMode();
    bool ApplyDemoPreset(const std::string& presetName);
    void ApplyMapProfile(const std::string& profileName);
    bool ApplyRuntimeMapMarkers(
        const std::string& profileName,
        const std::vector<ArenaPoint>& sharedSpawns,
        const std::vector<ArenaPoint>& healSpawns
    );
    bool ApplyClientBotHitClaim(
        std::uint64_t playerId,
        std::uint64_t shotId,
        std::uint64_t botId,
        int requestedDamage,
        bool bHeadshot,
        double hitX,
        double hitY,
        double hitZ,
        std::string& outReason
    );
    bool ApplyClientPlayerHitClaim(
        std::uint64_t playerId,
        std::uint64_t shotId,
        std::uint64_t targetPlayerId,
        int requestedDamage,
        bool bHeadshot,
        double hitX,
        double hitY,
        double hitZ,
        std::string& outReason
    );
    void SetBotAttacksEnabled(bool bEnabled);
    bool AreBotAttacksEnabled() const;
    void ApplyBotDifficulty(const std::string& difficulty);
    void SetBotCombatConfig(int bodyDamage, int headDamage, double cooldownSeconds);
    void SetHitboxConfig(
        double botBodyScale,
        double botHeadScale,
        double playerBodyScale,
        double playerHeadScale
    );
    void SetAutoEndMatchByTimer(bool bEnabled);

private:
    void ApplyMapProfileUnlocked(const std::string& profileName);
    void ApplyRuntimeMapMarkersUnlocked(
        const std::string& profileName,
        const std::vector<ArenaPoint>& sharedSpawns,
        const std::vector<ArenaPoint>& healSpawns
    );
    void LogMapProfile() const;
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
    bool IsInsideArena(double x, double y) const;
    double ClampArenaX(double x) const;
    double ClampArenaY(double y) const;
    double ClampBotX(double x) const;
    double ClampBotY(double y) const;
    std::pair<double, double> ClampToBotArea(double x, double y) const;
    std::size_t SelectRandomIndex(std::size_t count) const;
    bool IsSpawnTooCloseToAlivePlayer(const ArenaPoint& spawnPoint, std::uint64_t ignoredPlayerId) const;
    ArenaPoint ChoosePlayerSpawnPoint(const PlayerState& player) const;
    ArenaPoint ChooseBotSpawnPoint(std::uint64_t botId) const;
    std::size_t GetInitialBotWaypointIndex(const ArenaPoint& spawn) const;
    std::size_t GetNextBotWaypointIndex(const BotState& bot) const;
    void AssignBotWaypoint(BotState& bot, std::size_t waypointIndex) const;
    void AssignNextBotWaypoint(BotState& bot) const;
    void AssignNearbyBotWanderTarget(BotState& bot) const;
    void AssignRespawnPosition(PlayerState& player) const;
    void UpdateHealthPacks(double deltaSeconds);
    void CheckHealthPackPickups();
    ArenaPoint ChooseHealthPackSpawnPoint(std::uint64_t healthPackId) const;
    void AddCombatEvent(const std::string& type, const std::string& message);
    void AddCombatEvent(const CombatEvent& event);
    nlohmann::json BuildEventsJson() const;
    void ApplyPlayerDamageToBot(
        PlayerState& shooter,
        BotState& hitBot,
        int damage,
        bool bHeadshot,
        const std::string& hitGroup,
        double hitX,
        double hitY,
        double hitZ,
        bool bVerboseHitLog
    );
    void ProcessHitscanFire(PlayerState& shooter, const PlayerInput& input);
    void CheckMatchEndCondition();
    std::uint64_t DetermineWinnerPlayerId() const;
    nlohmann::json BuildScoreboardJson() const;
    void RefreshPlayerScore(PlayerState& player);
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
    nlohmann::json BuildArenaPointsJson(
        const std::vector<ArenaPoint>& points,
        const char* idFieldName
    ) const;
    nlohmann::json BuildArenaBoundsJson() const;
    nlohmann::json BuildBotAreaBoundsJson() const;
    nlohmann::json BuildArenaLayoutJson(
        bool bIncludeSpawnLists,
        bool bIncludeTargetCores
    ) const;

    std::uint64_t roomId;
    std::string roomName;
    std::uint64_t hostPlayerId;
    std::string roomFlowState;
    std::size_t maxPlayers;
    double matchEndingReturnSeconds;
    double matchEndingTimerSeconds;
    std::size_t desiredBotCount;
    std::unordered_map<std::uint64_t, PlayerState> players;
    MatchState matchState;
    mutable std::unordered_map<std::uint64_t, BotState> bots;
    mutable std::unordered_map<std::uint64_t, HealthPackState> healthPacks;
    std::unordered_map<std::uint64_t, ProjectileState> projectiles;
    mutable std::unordered_map<std::uint64_t, TargetState> targets;
    mutable std::vector<ArenaPoint> healthPackSpawnPoints;
    std::string mapProfile;
    ArenaBounds2D arenaBounds;
    ArenaBounds2D botAreaBounds;
    std::vector<ArenaPoint> playerSpawnPoints;
    std::vector<ArenaPoint> targetCorePositions;
    std::vector<ArenaPoint> botSpawnPoints;
    std::vector<ArenaPoint> botWanderPoints;
    std::vector<ArenaPoint> healthPackProfileSpawnPoints;
    std::vector<HealthPackInitialSpawn> initialHealthPackSpawns;
    std::vector<ArenaPoint> runtimeSharedSpawnPoints;
    std::vector<ArenaPoint> runtimeHealSpawnPoints;
    bool bHasRuntimeMapMarkers;
    std::uint64_t nextProjectileId;
    std::uint64_t healthPackRespawnCounter;
    std::deque<CombatEvent> recentEvents;
    std::uint64_t nextEventId;
    std::size_t MaxRecentEvents;
    double serverTimeSeconds;
    double lastBotAttackTickLogTime;
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
    double botFireChance;
    double botWanderRadius;
    double botWanderStepMin;
    double botWanderStepMax;
    double botWanderWaitMin;
    double botWanderWaitMax;
    double BotWanderRepathSeconds;
    bool bUseClientHitClaimsForBots;
    bool bUseClientHitClaimsForPlayers;
    bool bVerboseBotShotEvents;
    bool bVerboseInputLogs;
    bool bEnableBot2DFallbackHit;
    double Bot2DFallbackRadiusScale;
    double botBodyHitboxScale;
    double botHeadHitboxScale;
    double playerBodyHitboxScale;
    double playerHeadshotHitboxScale;
    bool bVerboseHitscanCandidateLogs;
    bool bVerboseBotStateLogs;
    bool bUseClientFireOriginForHitscan;
    double ClientFireOriginWarningDistance;
    double MaxAcceptedClientFireOriginDistance;
    bool bUseClientPositionForPlayerMovement;
    double MaxClientPositionDeltaPerSecond;
    double MaxClientPositionSnapDistance;
    double MinAcceptedClientZ;
    double MaxAcceptedClientZ;
    double MaxClientZDeltaPerSecond;
    bool bRejectExtremeClientZ;
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
    mutable std::mt19937 spawnRandom;
    mutable std::mutex mutex;
};
}
