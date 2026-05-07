#include "game/GameRoom.h"

#include "core/Logger.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

namespace battlegrid
{
namespace
{
constexpr double ArenaMinX = -1800.0;
constexpr double ArenaMaxX = 1800.0;
constexpr double ArenaMinY = -1200.0;
constexpr double ArenaMaxY = 1200.0;
constexpr double ArenaWidth = ArenaMaxX - ArenaMinX;
constexpr double ArenaHeight = ArenaMaxY - ArenaMinY;
constexpr double ProjectileSpawnForwardOffset = 50.0;
constexpr int BodyDamage = 20;
constexpr int HeadshotDamage = 40;
constexpr int BotBodyDamage = 10;
constexpr int BotHeadshotDamage = 20;
constexpr double HitscanRange = 3000.0;
constexpr double VisualTracerLength = 1800.0;
constexpr double VisualTracerLifetimeSeconds = 0.35;
constexpr double BotTracerLifetimeSeconds = 0.4;
constexpr double PlayerRespawnSeconds = 8.0;
constexpr double PlayerInvincibleSeconds = 1.5;
constexpr int BotMaxHp = 100;
constexpr double BotRespawnSeconds = 8.0;
constexpr double BotInvincibleSeconds = 1.5;
constexpr int BotKillScore = 1;
constexpr int PlayerKillScore = 1;
constexpr int TargetKillScore = 0;
constexpr const char* SafeDemoDifficulty = "easy";
constexpr bool bSafeDemoBotAttacksEnabled = false;
constexpr bool bSafeDemoAutoEndMatchByTimer = false;
constexpr bool bUseHitscanDamage = true;
constexpr bool bProjectileCollisionDamageEnabled = false;
constexpr double FireOriginHeight = 100.0;
constexpr double TargetCenterZ = 80.0;

struct ArenaPoint
{
    std::uint64_t id = 0;
    const char* label = "";
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct HealthPackInitialSpawn
{
    std::uint64_t healthPackId = 0;
    std::size_t spawnPointIndex = 0;
};

struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

constexpr std::array<ArenaPoint, 4> PlayerSpawnPoints = {{
    {1, "P1", -1200.0, 0.0, 0.0},
    {2, "P2", 1200.0, 0.0, 0.0},
    {3, "P3", 0.0, 900.0, 0.0},
    {4, "P4", 0.0, -900.0, 0.0},
}};

constexpr std::array<ArenaPoint, 5> TargetCorePositions = {{
    {1, "CORE-1", 0.0, 0.0, 0.0},
    {2, "CORE-2", 700.0, 500.0, 0.0},
    {3, "CORE-3", 700.0, -500.0, 0.0},
    {4, "CORE-4", -700.0, 500.0, 0.0},
    {5, "CORE-5", -700.0, -500.0, 0.0},
}};

constexpr std::array<ArenaPoint, 8> BotSpawnPoints = {{
    {1, "BOT-1", -300.0, 300.0, 0.0},
    {2, "BOT-2", -300.0, -300.0, 0.0},
    {3, "BOT-3", 300.0, 300.0, 0.0},
    {4, "BOT-4", 300.0, -300.0, 0.0},
    {5, "BOT-5", 1000.0, 0.0, 0.0},
    {6, "BOT-6", -1000.0, 0.0, 0.0},
    {7, "BOT-7", 0.0, 700.0, 0.0},
    {8, "BOT-8", 0.0, -700.0, 0.0},
}};

constexpr std::array<ArenaPoint, 6> HealthPackSpawnPoints = {{
    {1, "HPACK-SPAWN-1", -1300.0, 700.0, 0.0},
    {2, "HPACK-SPAWN-2", 1300.0, 700.0, 0.0},
    {3, "HPACK-SPAWN-3", 0.0, -1100.0, 0.0},
    {4, "HPACK-SPAWN-4", -1300.0, -700.0, 0.0},
    {5, "HPACK-SPAWN-5", 1300.0, -700.0, 0.0},
    {6, "HPACK-SPAWN-6", 0.0, 1100.0, 0.0},
}};

constexpr std::array<HealthPackInitialSpawn, 3> InitialHealthPackSpawns = {{
    {1, 0},
    {2, 2},
    {3, 4},
}};

struct HitscanHit
{
    enum class Type
    {
        None,
        Target,
        Player,
        Bot,
    };

    Type type = Type::None;
    double distance = std::numeric_limits<double>::max();
    std::uint64_t targetId = 0;
    std::uint64_t victimPlayerId = 0;
    std::uint64_t botId = 0;
    bool headshot = false;
    bool used2dFallback = false;
    int damage = 0;
};

double ClampX(double x)
{
    return std::clamp(x, ArenaMinX, ArenaMaxX);
}

double ClampY(double y)
{
    return std::clamp(y, ArenaMinY, ArenaMaxY);
}

bool IsInsideArena(double x, double y)
{
    return x >= ArenaMinX && x <= ArenaMaxX && y >= ArenaMinY && y <= ArenaMaxY;
}

template <std::size_t Count>
nlohmann::json BuildArenaPointsJson(
    const std::array<ArenaPoint, Count>& points,
    const char* idFieldName
)
{
    nlohmann::json pointsJson = nlohmann::json::array();
    for (const ArenaPoint& point : points)
    {
        nlohmann::json pointJson;
        pointJson[idFieldName] = point.id;
        pointJson["label"] = point.label;
        pointJson["x"] = point.x;
        pointJson["y"] = point.y;
        pointJson["z"] = point.z;
        pointsJson.push_back(std::move(pointJson));
    }

    return pointsJson;
}

nlohmann::json BuildArenaBoundsJson()
{
    nlohmann::json boundsJson;
    boundsJson["min_x"] = ArenaMinX;
    boundsJson["max_x"] = ArenaMaxX;
    boundsJson["min_y"] = ArenaMinY;
    boundsJson["max_y"] = ArenaMaxY;
    return boundsJson;
}

nlohmann::json BuildArenaLayoutJson(bool bIncludeSpawnLists, bool bIncludeTargetCores)
{
    nlohmann::json arenaJson;
    arenaJson["name"] = "BattleGrid PvPvE Kill Race Arena v1";
    arenaJson["bounds"] = BuildArenaBoundsJson();

    if (bIncludeSpawnLists)
    {
        arenaJson["player_spawns"] = BuildArenaPointsJson(PlayerSpawnPoints, "spawn_id");
        if (bIncludeTargetCores)
        {
            arenaJson["target_cores_debug"] = BuildArenaPointsJson(TargetCorePositions, "target_id");
        }
        arenaJson["bot_spawns"] = BuildArenaPointsJson(BotSpawnPoints, "bot_id");
        arenaJson["health_pack_spawns"] = BuildArenaPointsJson(HealthPackSpawnPoints, "spawn_id");
    }

    return arenaJson;
}

double Dot(const Vec3& lhs, const Vec3& rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

Vec3 Subtract(const Vec3& lhs, const Vec3& rhs)
{
    return Vec3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

bool Normalize(Vec3& value)
{
    const double length = std::sqrt(Dot(value, value));
    if (length <= 0.0001)
    {
        return false;
    }

    value.x /= length;
    value.y /= length;
    value.z /= length;
    return true;
}

bool RaySphereIntersection(
    const Vec3& origin,
    const Vec3& direction,
    const Vec3& center,
    double radius,
    double range,
    double& outDistance
)
{
    const Vec3 oc = Subtract(origin, center);
    const double b = 2.0 * Dot(oc, direction);
    const double c = Dot(oc, oc) - (radius * radius);
    const double discriminant = (b * b) - (4.0 * c);
    if (discriminant < 0.0)
    {
        return false;
    }

    const double sqrtDiscriminant = std::sqrt(discriminant);
    double distance = (-b - sqrtDiscriminant) * 0.5;
    if (distance < 0.0)
    {
        distance = (-b + sqrtDiscriminant) * 0.5;
    }

    if (distance < 0.0 || distance > range)
    {
        return false;
    }

    outDistance = distance;
    return true;
}

bool RayCircleIntersection2D(
    const Vec3& origin,
    const Vec3& direction,
    double centerX,
    double centerY,
    double radius,
    double range,
    double& outDistance
)
{
    const double directionLength =
        std::sqrt((direction.x * direction.x) + (direction.y * direction.y));
    if (directionLength <= 0.0001)
    {
        return false;
    }

    const double directionX = direction.x / directionLength;
    const double directionY = direction.y / directionLength;
    const double deltaX = centerX - origin.x;
    const double deltaY = centerY - origin.y;
    const double projectedDistance = (deltaX * directionX) + (deltaY * directionY);
    if (projectedDistance < 0.0 || projectedDistance > range)
    {
        return false;
    }

    const double closestX = origin.x + (directionX * projectedDistance);
    const double closestY = origin.y + (directionY * projectedDistance);
    const double missX = centerX - closestX;
    const double missY = centerY - closestY;
    const double missDistanceSquared = (missX * missX) + (missY * missY);
    if (missDistanceSquared > radius * radius)
    {
        return false;
    }

    outDistance = projectedDistance;
    return true;
}

Vec3 BuildShotDirection(const PlayerInput& input)
{
    Vec3 direction{input.shotDirX, input.shotDirY, input.shotDirZ};
    if (!Normalize(direction))
    {
        direction = Vec3{input.aimX, input.aimY, 0.0};
        if (!Normalize(direction))
        {
            direction = Vec3{1.0, 0.0, 0.0};
        }
    }

    return direction;
}

double RandomUnitValue(double seed)
{
    const double value = std::sin(seed * 12.9898 + 78.233) * 43758.5453;
    return value - std::floor(value);
}

Vec3 ApplyConeSpread(const Vec3& direction, double spreadDegrees, double seed)
{
    Vec3 baseDirection = direction;
    if (!Normalize(baseDirection) || spreadDegrees <= 0.0)
    {
        return baseDirection;
    }

    Vec3 right{-baseDirection.y, baseDirection.x, 0.0};
    if (!Normalize(right))
    {
        right = Vec3{0.0, 1.0, 0.0};
    }

    const double spreadRadians = spreadDegrees * 3.14159265358979323846 / 180.0;
    const double lateralOffset =
        (RandomUnitValue(seed) * 2.0 - 1.0) * std::tan(spreadRadians);
    const double verticalOffset =
        (RandomUnitValue(seed + 17.0) * 2.0 - 1.0) * std::tan(spreadRadians) * 0.5;

    Vec3 spreadDirection{
        baseDirection.x + (right.x * lateralOffset),
        baseDirection.y + (right.y * lateralOffset),
        baseDirection.z + verticalOffset,
    };
    if (!Normalize(spreadDirection))
    {
        return baseDirection;
    }

    return spreadDirection;
}

Vec3 BuildHitPoint(const Vec3& origin, const Vec3& direction, double distance)
{
    if (!std::isfinite(distance) || distance == std::numeric_limits<double>::max())
    {
        return origin;
    }

    return Vec3{
        origin.x + (direction.x * distance),
        origin.y + (direction.y * distance),
        origin.z + (direction.z * distance),
    };
}

std::string GetShotResultTargetName(HitscanHit::Type type)
{
    switch (type)
    {
    case HitscanHit::Type::Target:
        return "core";
    case HitscanHit::Type::Player:
        return "player";
    case HitscanHit::Type::Bot:
        return "bot";
    default:
        return "none";
    }
}

void LogShotResult(
    std::uint64_t shooterId,
    bool bHit,
    HitscanHit::Type targetType,
    int damage,
    bool bHeadshot
)
{
    std::ostringstream logMessage;
    logMessage
        << "Shot result shooter=" << shooterId
        << " result=" << (bHit ? "hit" : "miss")
        << " target=" << GetShotResultTargetName(targetType)
        << " damage=" << damage
        << " headshot=" << (bHeadshot ? "true" : "false");
    Logger::Info(logMessage.str());
}

std::string BuildShotShortMessage(
    HitscanHit::Type targetType,
    std::uint64_t targetId,
    int damage,
    bool bHeadshot
)
{
    if (targetType == HitscanHit::Type::None)
    {
        return "SERVER MISS";
    }

    std::string label = "TARGET";
    if (targetType == HitscanHit::Type::Bot)
    {
        label = "BOT-" + std::to_string(targetId);
    }
    else if (targetType == HitscanHit::Type::Target)
    {
        label = "CORE-" + std::to_string(targetId);
    }
    else if (targetType == HitscanHit::Type::Player)
    {
        label = "PLAYER-" + std::to_string(targetId);
    }

    return std::string(bHeadshot ? "SERVER HEADSHOT " : "SERVER HIT ")
        + label
        + " -"
        + std::to_string(damage);
}

void AssignRespawnPosition(PlayerState& player)
{
    const std::size_t spawnIndex = static_cast<std::size_t>(
        (player.playerId - 1) % PlayerSpawnPoints.size()
    );
    const ArenaPoint& spawnPoint = PlayerSpawnPoints[spawnIndex];

    player.x = spawnPoint.x;
    player.y = spawnPoint.y;
    player.z = spawnPoint.z;

    std::ostringstream logMessage;
    logMessage
        << "Player spawn player_id=" << player.playerId
        << " x=" << player.x
        << " y=" << player.y;
    Logger::Info(logMessage.str());
}

int CalculateKillRaceScore(const PlayerState& player);

nlohmann::json BuildPlayerSnapshotJson(const PlayerState& player)
{
    nlohmann::json playerJson;
    playerJson["player_id"] = player.playerId;
    playerJson["nickname"] = player.nickname;
    playerJson["x"] = player.x;
    playerJson["y"] = player.y;
    playerJson["z"] = player.z;
    playerJson["hp"] = player.hp;
    playerJson["max_hp"] = player.maxHp;
    playerJson["alive"] = player.alive;
    playerJson["invincible"] = player.invincible;
    playerJson["respawn_timer"] = player.respawnTimerSeconds;
    playerJson["invincible_timer"] = player.invincibleTimerSeconds;
    playerJson["score"] = CalculateKillRaceScore(player);
    playerJson["kills"] = player.kills;
    playerJson["deaths"] = player.deaths;
    playerJson["player_kills"] = player.playerKills;
    playerJson["target_kills"] = player.targetKills;
    playerJson["bot_kills"] = player.botKills;
    playerJson["last_seq"] = player.latestInput.seq;
    return playerJson;
}

nlohmann::json BuildBotSnapshotJson(const BotState& bot)
{
    nlohmann::json botJson;
    botJson["bot_id"] = bot.botId;
    botJson["name"] = bot.name;
    botJson["x"] = bot.x;
    botJson["y"] = bot.y;
    botJson["z"] = bot.z;
    botJson["yaw"] = bot.yaw;
    botJson["hp"] = bot.hp;
    botJson["max_hp"] = bot.maxHp;
    botJson["alive"] = bot.alive;
    botJson["invincible"] = bot.invincible;
    botJson["target_player_id"] = bot.targetPlayerId;
    botJson["ammo"] = bot.ammo;
    botJson["magazine_size"] = bot.magazineSize;
    botJson["reloading"] = bot.reloading;
    botJson["reload_timer"] = bot.reloadTimerSeconds;
    botJson["fire_cooldown"] = bot.fireCooldownSeconds;
    botJson["aim_spread_deg"] = bot.aimSpreadDegrees;
    botJson["attack_range"] = bot.attackRange;
    return botJson;
}

nlohmann::json BuildProjectileSnapshotJson(const ProjectileState& projectile)
{
    nlohmann::json projectileJson;
    projectileJson["projectile_id"] = projectile.projectileId;
    projectileJson["owner_player_id"] = projectile.ownerPlayerId;
    projectileJson["owner_type"] = projectile.ownerType;
    projectileJson["owner_bot_id"] = projectile.ownerBotId;
    projectileJson["x"] = projectile.x;
    projectileJson["y"] = projectile.y;
    projectileJson["z"] = projectile.z;
    projectileJson["dir_x"] = projectile.dirX;
    projectileJson["dir_y"] = projectile.dirY;
    projectileJson["dir_z"] = projectile.dirZ;
    projectileJson["visual_only"] = projectile.visualOnly;
    projectileJson["start_x"] = projectile.startX;
    projectileJson["start_y"] = projectile.startY;
    projectileJson["start_z"] = projectile.startZ;
    projectileJson["end_x"] = projectile.endX;
    projectileJson["end_y"] = projectile.endY;
    projectileJson["end_z"] = projectile.endZ;
    return projectileJson;
}

nlohmann::json BuildTargetSnapshotJson(const TargetState& target)
{
    nlohmann::json targetJson;
    targetJson["target_id"] = target.targetId;
    targetJson["x"] = target.x;
    targetJson["y"] = target.y;
    targetJson["hp"] = target.hp;
    targetJson["max_hp"] = target.maxHp;
    targetJson["alive"] = target.alive;
    return targetJson;
}

nlohmann::json BuildHealthPackSnapshotJson(const HealthPackState& healthPack)
{
    nlohmann::json healthPackJson;
    healthPackJson["health_pack_id"] = healthPack.healthPackId;
    healthPackJson["x"] = healthPack.x;
    healthPackJson["y"] = healthPack.y;
    healthPackJson["z"] = healthPack.z;
    healthPackJson["active"] = healthPack.active;
    healthPackJson["heal_amount"] = healthPack.healAmount;
    healthPackJson["respawn_timer"] = healthPack.respawnTimerSeconds;
    return healthPackJson;
}

int CalculateKillRaceScore(const PlayerState& player)
{
    return player.botKills + player.playerKills;
}
}

GameRoom::GameRoom(std::uint64_t inRoomId)
    : roomId(inRoomId),
      players(),
      matchState(),
      bots(),
      healthPacks(),
      projectiles(),
      targets(),
      healthPackSpawnPoints(),
      nextProjectileId(1),
      healthPackRespawnCounter(0),
      recentEvents(),
      nextEventId(1),
      MaxRecentEvents(20),
      serverTimeSeconds(0.0),
      bBotAttacksEnabled(true),
      botDifficulty("normal"),
      bAutoEndMatchByTimer(true),
      bTargetsEnabled(false),
      botDetectRange(1500.0),
      botAttackRange(1400.0),
      botAttackDamage(BotBodyDamage),
      botHeadshotDamage(BotHeadshotDamage),
      botAttackCooldownSeconds(0.5),
      botMoveSpeed(500.0),
      botFireIntervalSeconds(0.5),
      botAimSpreadDegrees(12.0),
      bVerboseBotShotEvents(false),
      targetsInitialized(false),
      botsInitialized(false),
      healthPacksInitialized(false),
      mutex()
{
    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();
}

std::uint64_t GameRoom::GetRoomId() const
{
    return roomId;
}

bool GameRoom::AddPlayer(std::uint64_t playerId, const std::string& nickname)
{
    std::lock_guard lock(mutex);
    const auto [iterator, inserted] = players.emplace(
        playerId,
        PlayerState(playerId, roomId, nickname)
    );

    if (!inserted)
    {
        iterator->second.nickname = nickname;
        iterator->second.connected = true;
        return false;
    }

    PlayerState& player = iterator->second;
    player.hp = player.maxHp;
    player.alive = true;
    player.invincible = false;
    player.respawnTimerSeconds = 0.0;
    player.invincibleTimerSeconds = 0.0;
    AssignRespawnPosition(player);

    return true;
}

bool GameRoom::RemovePlayer(std::uint64_t playerId)
{
    std::lock_guard lock(mutex);
    return players.erase(playerId) > 0;
}

bool GameRoom::HasPlayer(std::uint64_t playerId) const
{
    std::lock_guard lock(mutex);
    return players.find(playerId) != players.end();
}

bool GameRoom::UpdateInput(std::uint64_t playerId, const PlayerInput& input)
{
    std::lock_guard lock(mutex);
    const auto player = players.find(playerId);
    if (player == players.end())
    {
        return false;
    }

    player->second.latestInput = input;
    return true;
}

std::size_t GameRoom::GetPlayerCount() const
{
    std::lock_guard lock(mutex);
    return players.size();
}

void GameRoom::Tick(double deltaSeconds, std::uint64_t tickNumber)
{
    std::lock_guard lock(mutex);
    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();

    serverTimeSeconds += std::max(0.0, deltaSeconds);

    if (matchState.IsGameOver())
    {
        return;
    }

    ProcessPlayerRespawns(deltaSeconds);
    UpdateBotRespawns(deltaSeconds);

    for (auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);

        if (!player.connected || !player.alive)
        {
            continue;
        }

        double moveX = player.latestInput.moveX;
        double moveY = player.latestInput.moveY;
        const double length = std::sqrt((moveX * moveX) + (moveY * moveY));
        if (length > 1.0)
        {
            moveX /= length;
            moveY /= length;
        }

        player.x += moveX * player.speed * deltaSeconds;
        player.y += moveY * player.speed * deltaSeconds;
        player.x = ClampX(player.x);
        player.y = ClampY(player.y);

        if (
            player.latestInput.fire
            && player.latestInput.seq != player.lastProcessedFireSeq
        )
        {
            double projectileDirX = player.latestInput.shotDirX;
            double projectileDirY = player.latestInput.shotDirY;
            double projectileDirZ = player.latestInput.shotDirZ;
            const double shotDirectionLengthSquared =
                (projectileDirX * projectileDirX)
                + (projectileDirY * projectileDirY)
                + (projectileDirZ * projectileDirZ);
            if (shotDirectionLengthSquared <= 0.0001)
            {
                projectileDirX = player.latestInput.aimX;
                projectileDirY = player.latestInput.aimY;
                projectileDirZ = 0.0;
            }

            SpawnProjectile(
                player.playerId,
                projectileDirX,
                projectileDirY,
                projectileDirZ
            );
            if (bUseHitscanDamage)
            {
                ProcessHitscanFire(player, player.latestInput);
            }
            player.lastProcessedFireSeq = player.latestInput.seq;
        }

        if (tickNumber % 60 == 0)
        {
            std::ostringstream logMessage;
            logMessage
                << "Player position player_id=" << player.playerId
                << " x=" << player.x
                << " y=" << player.y
                << " move_x=" << moveX
                << " move_y=" << moveY;
            Logger::Info(logMessage.str());
        }
    }

    UpdateProjectiles(deltaSeconds);
    if (bTargetsEnabled && (bProjectileCollisionDamageEnabled || !bUseHitscanDamage))
    {
        UpdateProjectileTargetCollisions();
    }
    RemoveInactiveProjectiles();
    UpdateBots(deltaSeconds);
    UpdateHealthPacks(deltaSeconds);
    matchState.Tick(deltaSeconds, bAutoEndMatchByTimer);
    CheckMatchEndCondition();
}

nlohmann::json GameRoom::BuildSnapshotJson(std::uint64_t tickNumber) const
{
    std::lock_guard lock(mutex);
    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();

    nlohmann::json json;
    json["type"] = "snapshot";
    json["tick"] = tickNumber;
    json["room_id"] = roomId;
    json["arena"] = BuildArenaLayoutJson(true, bTargetsEnabled);
    json["match"] = matchState.ToJson();
    json["scoreboard"] = BuildScoreboardJson();
    json["events"] = BuildEventsJson();
    json["players"] = nlohmann::json::array();
    json["bots"] = nlohmann::json::array();
    json["health_packs"] = nlohmann::json::array();
    json["projectiles"] = nlohmann::json::array();
    json["targets"] = nlohmann::json::array();

    for (const auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);

        if (!player.connected)
        {
            continue;
        }

        json["players"].push_back(BuildPlayerSnapshotJson(player));
    }

    for (const auto& [projectileId, projectile] : projectiles)
    {
        static_cast<void>(projectileId);

        if (!projectile.active)
        {
            continue;
        }

        json["projectiles"].push_back(BuildProjectileSnapshotJson(projectile));
    }

    for (const auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);
        json["bots"].push_back(BuildBotSnapshotJson(bot));
    }

    for (const auto& [healthPackId, healthPack] : healthPacks)
    {
        static_cast<void>(healthPackId);
        json["health_packs"].push_back(BuildHealthPackSnapshotJson(healthPack));
    }

    if (bTargetsEnabled)
    {
        for (const auto& [targetId, target] : targets)
        {
            static_cast<void>(targetId);
            json["targets"].push_back(BuildTargetSnapshotJson(target));
        }
    }

    return json;
}

nlohmann::json GameRoom::ToDebugJson() const
{
    std::lock_guard lock(mutex);
    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();

    nlohmann::json json;
    json["room_id"] = roomId;
    json["arena"] = BuildArenaLayoutJson(true, true);
    json["match"] = matchState.ToJson();
    json["scoreboard"] = BuildScoreboardJson();
    json["events"] = BuildEventsJson();
    json["player_count"] = players.size();
    json["projectile_count"] = projectiles.size();
    json["targets_enabled"] = bTargetsEnabled;
    json["targets_debug_count"] = targets.size();
    json["bot_count"] = bots.size();
    json["bot_attacks_enabled"] = bBotAttacksEnabled;
    json["bot_difficulty"] = botDifficulty;
    json["bot_attack_damage"] = botAttackDamage;
    json["bot_headshot_damage"] = botHeadshotDamage;
    json["bot_attack_cooldown"] = botAttackCooldownSeconds;
    json["bot_detect_range"] = botDetectRange;
    json["bot_attack_range"] = botAttackRange;
    json["bot_move_speed"] = botMoveSpeed;
    json["bot_fire_interval"] = botFireIntervalSeconds;
    json["bot_aim_spread"] = botAimSpreadDegrees;
    json["bot_shot_events_verbose"] = bVerboseBotShotEvents;
    json["auto_end_match_by_timer"] = bAutoEndMatchByTimer;
    json["body_damage"] = BodyDamage;
    json["headshot_damage"] = HeadshotDamage;
    json["bot_body_damage"] = botAttackDamage;
    json["hitscan_range"] = HitscanRange;
    json["bot_max_hp"] = BotMaxHp;
    json["bot_respawn_seconds"] = BotRespawnSeconds;
    json["bot_invincible_seconds"] = BotInvincibleSeconds;
    json["bot_kill_score"] = BotKillScore;
    json["player_kill_score"] = PlayerKillScore;
    json["target_kill_score"] = TargetKillScore;
    json["health_pack_count"] = healthPacks.size();
    json["players"] = nlohmann::json::array();
    json["bots"] = nlohmann::json::array();
    json["health_packs"] = nlohmann::json::array();
    json["projectiles"] = nlohmann::json::array();
    json["targets_debug"] = nlohmann::json::array();

    for (const auto& [playerId, player] : players)
    {
        const PlayerInput& input = player.latestInput;

        nlohmann::json inputJson;
        inputJson["seq"] = input.seq;
        inputJson["move_x"] = input.moveX;
        inputJson["move_y"] = input.moveY;
        inputJson["aim_x"] = input.aimX;
        inputJson["aim_y"] = input.aimY;
        inputJson["fire"] = input.fire;
        inputJson["reload"] = input.reload;
        inputJson["ads"] = input.ads;
        inputJson["sprint"] = input.sprint;
        inputJson["jump"] = input.jump;
        inputJson["ammo"] = input.ammo;
        inputJson["spread_deg"] = input.spreadDegrees;
        inputJson["shot_dir_x"] = input.shotDirX;
        inputJson["shot_dir_y"] = input.shotDirY;
        inputJson["shot_dir_z"] = input.shotDirZ;

        nlohmann::json playerJson;
        playerJson["player_id"] = playerId;
        playerJson["room_id"] = player.roomId;
        playerJson["nickname"] = player.nickname;
        playerJson["connected"] = player.connected;
        playerJson["alive"] = player.alive;
        playerJson["invincible"] = player.invincible;
        playerJson["x"] = player.x;
        playerJson["y"] = player.y;
        playerJson["z"] = player.z;
        playerJson["speed"] = player.speed;
        playerJson["hp"] = player.hp;
        playerJson["max_hp"] = player.maxHp;
        playerJson["score"] = CalculateKillRaceScore(player);
        playerJson["kills"] = player.kills;
        playerJson["deaths"] = player.deaths;
        playerJson["player_kills"] = player.playerKills;
        playerJson["target_kills"] = player.targetKills;
        playerJson["bot_kills"] = player.botKills;
        playerJson["respawn_timer"] = player.respawnTimerSeconds;
        playerJson["invincible_timer"] = player.invincibleTimerSeconds;
        playerJson["latest_input"] = std::move(inputJson);

        json["players"].push_back(std::move(playerJson));
    }

    for (const auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);
        json["bots"].push_back(BuildBotSnapshotJson(bot));
    }

    std::size_t activeHealthPackCount = 0;
    for (const auto& [healthPackId, healthPack] : healthPacks)
    {
        static_cast<void>(healthPackId);
        if (healthPack.IsActive())
        {
            ++activeHealthPackCount;
        }
        json["health_packs"].push_back(BuildHealthPackSnapshotJson(healthPack));
    }
    json["active_health_pack_count"] = activeHealthPackCount;

    for (const auto& [projectileId, projectile] : projectiles)
    {
        static_cast<void>(projectileId);

        if (!projectile.active)
        {
            continue;
        }

        json["projectiles"].push_back(BuildProjectileSnapshotJson(projectile));
    }

    for (const auto& [targetId, target] : targets)
    {
        static_cast<void>(targetId);
        json["targets_debug"].push_back(BuildTargetSnapshotJson(target));
    }

    return json;
}

std::uint64_t GameRoom::ResetMatch()
{
    std::lock_guard lock(mutex);

    const std::uint64_t nextMatchId = matchState.matchId + 1;
    matchState.Reset();
    matchState.matchId = nextMatchId;
    serverTimeSeconds = 0.0;
    recentEvents.clear();
    nextEventId = 1;

    projectiles.clear();
    nextProjectileId = 1;
    healthPackRespawnCounter = 0;

    targets.clear();
    bots.clear();
    healthPacks.clear();
    targetsInitialized = false;
    botsInitialized = false;
    healthPacksInitialized = false;

    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();

    for (auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);

        player.latestInput = PlayerInput();
        player.alive = true;
        player.invincible = false;
        player.hp = player.maxHp;
        player.score = 0;
        player.deaths = 0;
        player.kills = 0;
        player.playerKills = 0;
        player.targetKills = 0;
        player.botKills = 0;
        player.respawnTimerSeconds = 0.0;
        player.invincibleTimerSeconds = 0.0;
        player.lastProcessedFireSeq = 0;
        AssignRespawnPosition(player);
    }

    AddCombatEvent("match_restarted", "Match restarted");
    Logger::Info("Match restarted match_id=" + std::to_string(matchState.matchId));
    return matchState.matchId;
}

std::uint64_t GameRoom::ApplySafeDemoMode()
{
    std::lock_guard lock(mutex);

    bBotAttacksEnabled = bSafeDemoBotAttacksEnabled;
    bAutoEndMatchByTimer = bSafeDemoAutoEndMatchByTimer;
    ApplyBotDifficultyUnlocked(SafeDemoDifficulty);

    const std::uint64_t nextMatchId = matchState.matchId + 1;
    matchState.Reset();
    matchState.matchId = nextMatchId;
    serverTimeSeconds = 0.0;
    recentEvents.clear();
    nextEventId = 1;

    projectiles.clear();
    nextProjectileId = 1;
    healthPackRespawnCounter = 0;

    targets.clear();
    bots.clear();
    healthPacks.clear();
    targetsInitialized = false;
    botsInitialized = false;
    healthPacksInitialized = false;

    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();

    for (auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);

        player.latestInput = PlayerInput();
        player.alive = true;
        player.invincible = false;
        player.hp = player.maxHp;
        player.score = 0;
        player.deaths = 0;
        player.kills = 0;
        player.playerKills = 0;
        player.targetKills = 0;
        player.botKills = 0;
        player.respawnTimerSeconds = 0.0;
        player.invincibleTimerSeconds = 0.0;
        player.lastProcessedFireSeq = 0;
        AssignRespawnPosition(player);
    }

    AddCombatEvent("demo_mode_applied", "Safe demo mode applied");
    Logger::Info("Safe demo mode applied.");
    return matchState.matchId;
}

void GameRoom::SetBotAttacksEnabled(bool bEnabled)
{
    std::lock_guard lock(mutex);
    bBotAttacksEnabled = bEnabled;
    Logger::Info(
        std::string("Bot attacks enabled=")
        + (bBotAttacksEnabled ? "true" : "false")
    );
}

bool GameRoom::AreBotAttacksEnabled() const
{
    std::lock_guard lock(mutex);
    return bBotAttacksEnabled;
}

void GameRoom::ApplyBotDifficulty(const std::string& difficulty)
{
    std::lock_guard lock(mutex);

    ApplyBotDifficultyUnlocked(difficulty);
    Logger::Info("Bot difficulty set to " + botDifficulty);
}

void GameRoom::ApplyBotDifficultyUnlocked(const std::string& difficulty)
{
    botDifficulty = difficulty;
    if (difficulty == "easy")
    {
        botAttackDamage = BotBodyDamage;
        botHeadshotDamage = BotHeadshotDamage;
        botAttackCooldownSeconds = 0.75;
        botDetectRange = 1000.0;
        botAttackRange = 1200.0;
        botMoveSpeed = 400.0;
        botFireIntervalSeconds = 0.75;
        botAimSpreadDegrees = 18.0;
    }
    else if (difficulty == "hard")
    {
        botAttackDamage = BotBodyDamage;
        botHeadshotDamage = BotHeadshotDamage;
        botAttackCooldownSeconds = 0.35;
        botDetectRange = 1800.0;
        botAttackRange = 1600.0;
        botMoveSpeed = 600.0;
        botFireIntervalSeconds = 0.35;
        botAimSpreadDegrees = 7.0;
    }
    else
    {
        botDifficulty = "normal";
        botAttackDamage = BotBodyDamage;
        botHeadshotDamage = BotHeadshotDamage;
        botAttackCooldownSeconds = 0.5;
        botDetectRange = 1500.0;
        botAttackRange = 1400.0;
        botMoveSpeed = 500.0;
        botFireIntervalSeconds = 0.5;
        botAimSpreadDegrees = 12.0;
    }

    for (auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);
        bot.speed = botMoveSpeed;
        bot.attackCooldownSeconds = botAttackCooldownSeconds;
        bot.attackTimerSeconds = std::min(bot.attackTimerSeconds, botAttackCooldownSeconds);
        bot.attackRange = botAttackRange;
        bot.fireIntervalSeconds = botFireIntervalSeconds;
        bot.aimSpreadDegrees = botAimSpreadDegrees;
        bot.fireCooldownSeconds = std::min(bot.fireCooldownSeconds, bot.fireIntervalSeconds);
    }
}

void GameRoom::SetAutoEndMatchByTimer(bool bEnabled)
{
    std::lock_guard lock(mutex);
    bAutoEndMatchByTimer = bEnabled;
    Logger::Info(
        std::string("Auto end match by timer=")
        + (bAutoEndMatchByTimer ? "true" : "false")
    );
}

void GameRoom::InitializeDefaultTargets() const
{
    if (targetsInitialized && !targets.empty())
    {
        return;
    }

    targets.clear();

    for (const ArenaPoint& corePosition : TargetCorePositions)
    {
        TargetState target;
        target.targetId = corePosition.id;
        target.x = corePosition.x;
        target.y = corePosition.y;
        target.hp = 100;
        target.maxHp = 100;
        target.radius = 80.0;
        target.alive = true;
        targets.emplace(target.targetId, target);
    }

    targetsInitialized = true;
    Logger::Info("Initialized default server cores count=" + std::to_string(targets.size()));
}

void GameRoom::InitializeDefaultBots() const
{
    if (botsInitialized && !bots.empty())
    {
        return;
    }

    bots.clear();

    for (const ArenaPoint& spawn : BotSpawnPoints)
    {
        BotState bot;
        bot.botId = spawn.id;
        bot.name = spawn.label;
        bot.x = spawn.x;
        bot.y = spawn.y;
        bot.z = spawn.z;
        bot.yaw = 0.0;
        bot.hp = BotMaxHp;
        bot.maxHp = BotMaxHp;
        bot.alive = true;
        bot.invincible = false;
        bot.speed = botMoveSpeed;
        bot.wanderTargetX = spawn.x;
        bot.wanderTargetY = spawn.y;
        bot.decisionTimerSeconds = 0.0;
        bot.attackCooldownSeconds = botAttackCooldownSeconds;
        bot.attackTimerSeconds = 0.0;
        bot.ammo = bot.magazineSize;
        bot.reloading = false;
        bot.reloadTimerSeconds = 0.0;
        bot.fireCooldownSeconds = 0.0;
        bot.fireIntervalSeconds = botFireIntervalSeconds;
        bot.aimSpreadDegrees = botAimSpreadDegrees;
        bot.attackRange = botAttackRange;
        bot.preferredCombatRange = 900.0;
        bots.emplace(bot.botId, std::move(bot));
    }

    botsInitialized = true;
    Logger::Info("Initialized default bots count=" + std::to_string(bots.size()));
}

void GameRoom::InitializeHealthPackSpawnPoints() const
{
    if (!healthPackSpawnPoints.empty())
    {
        return;
    }

    healthPackSpawnPoints.reserve(HealthPackSpawnPoints.size());
    for (const ArenaPoint& spawnPoint : HealthPackSpawnPoints)
    {
        healthPackSpawnPoints.emplace_back(spawnPoint.x, spawnPoint.y);
    }
}

void GameRoom::InitializeDefaultHealthPacks() const
{
    if (healthPacksInitialized && !healthPacks.empty())
    {
        return;
    }

    InitializeHealthPackSpawnPoints();
    healthPacks.clear();

    for (const HealthPackInitialSpawn& spawn : InitialHealthPackSpawns)
    {
        if (healthPackSpawnPoints.empty())
        {
            break;
        }

        const std::pair<double, double>& spawnPoint =
            healthPackSpawnPoints[spawn.spawnPointIndex % healthPackSpawnPoints.size()];

        HealthPackState healthPack;
        healthPack.healthPackId = spawn.healthPackId;
        healthPack.x = spawnPoint.first;
        healthPack.y = spawnPoint.second;
        healthPack.z = 0.0;
        healthPack.active = true;
        healthPack.healAmount = 35;
        healthPack.pickupRadius = 90.0;
        healthPack.respawnTimerSeconds = 0.0;
        healthPack.respawnDelaySeconds = 15.0;
        healthPacks.emplace(healthPack.healthPackId, healthPack);
    }

    healthPacksInitialized = true;
    Logger::Info("Initialized health packs count=" + std::to_string(healthPacks.size()));
}

void GameRoom::SpawnProjectile(
    std::uint64_t ownerPlayerId,
    double dirX,
    double dirY,
    double dirZ
)
{
    const auto owner = players.find(ownerPlayerId);
    if (owner == players.end() || !owner->second.connected)
    {
        return;
    }

    const double startX = owner->second.x;
    const double startY = owner->second.y;
    const double startZ = owner->second.z + FireOriginHeight;
    CreateVisualTracer(
        "player",
        ownerPlayerId,
        0,
        startX,
        startY,
        startZ,
        dirX,
        dirY,
        dirZ,
        VisualTracerLength,
        VisualTracerLifetimeSeconds
    );
}

void GameRoom::CreateVisualTracer(
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
)
{
    ProjectileState projectile;
    projectile.projectileId = nextProjectileId++;
    projectile.ownerPlayerId = ownerPlayerId;
    projectile.ownerType = ownerType;
    projectile.ownerBotId = ownerBotId;
    projectile.visualOnly = true;
    projectile.damage = 0;
    projectile.lifeTimeSeconds = lifeTimeSeconds > 0.0
        ? lifeTimeSeconds
        : VisualTracerLifetimeSeconds;
    projectile.maxLifetimeSeconds = projectile.lifeTimeSeconds;
    projectile.dirX = dirX;
    projectile.dirY = dirY;
    projectile.dirZ = dirZ;
    projectile.NormalizeDirection();

    projectile.startX = startX + (projectile.dirX * ProjectileSpawnForwardOffset);
    projectile.startY = startY + (projectile.dirY * ProjectileSpawnForwardOffset);
    projectile.startZ = startZ + (projectile.dirZ * ProjectileSpawnForwardOffset);
    projectile.x = projectile.startX;
    projectile.y = projectile.startY;
    projectile.z = projectile.startZ;

    const double tracerLength = rangeOrLength > 0.0 ? rangeOrLength : VisualTracerLength;
    projectile.endX = projectile.startX + (projectile.dirX * tracerLength);
    projectile.endY = projectile.startY + (projectile.dirY * tracerLength);
    projectile.endZ = projectile.startZ + (projectile.dirZ * tracerLength);
    projectile.speed = tracerLength / projectile.lifeTimeSeconds;

    projectiles.emplace(projectile.projectileId, projectile);

    std::ostringstream logMessage;
    logMessage
        << "Visual tracer spawned id=" << projectile.projectileId
        << " owner_type=" << projectile.ownerType
        << " owner_player=" << projectile.ownerPlayerId
        << " owner_bot=" << projectile.ownerBotId
        << " start=(" << projectile.startX << "," << projectile.startY << "," << projectile.startZ << ")"
        << " end=(" << projectile.endX << "," << projectile.endY << "," << projectile.endZ << ")"
        << " dir_x=" << projectile.dirX
        << " dir_y=" << projectile.dirY
        << " dir_z=" << projectile.dirZ;
    Logger::Info(logMessage.str());
}

void GameRoom::SpawnBotProjectile(
    const BotState& bot,
    double dirX,
    double dirY,
    double dirZ
)
{
    if (!bot.IsAlive())
    {
        return;
    }

    CreateVisualTracer(
        "bot",
        0,
        bot.botId,
        bot.x,
        bot.y,
        bot.z + FireOriginHeight,
        dirX,
        dirY,
        dirZ,
        VisualTracerLength,
        BotTracerLifetimeSeconds
    );
}

void GameRoom::ProcessPlayerRespawns(double deltaSeconds)
{
    for (auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);

        if (!player.connected)
        {
            continue;
        }

        if (!player.alive)
        {
            player.respawnTimerSeconds -= deltaSeconds;
            if (player.respawnTimerSeconds > 0.0)
            {
                continue;
            }

            player.alive = true;
            player.invincible = true;
            player.hp = player.maxHp;
            player.respawnTimerSeconds = 0.0;
            player.invincibleTimerSeconds = PlayerInvincibleSeconds;
            player.latestInput.moveX = 0.0;
            player.latestInput.moveY = 0.0;
            player.latestInput.fire = false;
            AssignRespawnPosition(player);

            Logger::Info("Player respawned player_id=" + std::to_string(player.playerId));
            CombatEvent event;
            event.type = "player_respawned";
            event.message = player.nickname + " respawned";
            event.actorPlayerId = player.playerId;
            AddCombatEvent(event);
            continue;
        }

        if (player.invincible)
        {
            player.invincibleTimerSeconds -= deltaSeconds;
            if (player.invincibleTimerSeconds <= 0.0)
            {
                player.invincible = false;
                player.invincibleTimerSeconds = 0.0;
            }
        }
    }
}

void GameRoom::UpdateBotRespawns(double deltaSeconds)
{
    for (auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);

        if (!bot.connected)
        {
            continue;
        }

        if (!bot.alive)
        {
            bot.respawnTimerSeconds -= deltaSeconds;
            if (bot.respawnTimerSeconds > 0.0)
            {
                continue;
            }

            const std::size_t spawnIndex = static_cast<std::size_t>(
                (bot.botId - 1) % BotSpawnPoints.size()
            );
            const ArenaPoint& spawnPoint = BotSpawnPoints[spawnIndex];
            bot.Respawn(spawnPoint.x, spawnPoint.y);
            bot.invincibleTimerSeconds = BotInvincibleSeconds;
            bot.speed = botMoveSpeed;
            bot.attackRange = botAttackRange;
            bot.fireIntervalSeconds = botFireIntervalSeconds;
            bot.aimSpreadDegrees = botAimSpreadDegrees;

            Logger::Info("Bot respawned bot_id=" + std::to_string(bot.botId));
            continue;
        }

        if (bot.invincible)
        {
            bot.invincibleTimerSeconds -= deltaSeconds;
            if (bot.invincibleTimerSeconds <= 0.0)
            {
                bot.invincible = false;
                bot.invincibleTimerSeconds = 0.0;
            }
        }
    }
}

void GameRoom::UpdateBots(double deltaSeconds)
{
    UpdateBotAI(deltaSeconds);
}

void GameRoom::ProcessBotShot(BotState& bot, PlayerState& targetPlayer)
{
    if (
        !bBotAttacksEnabled
        || matchState.IsGameOver()
        || !bot.CanFire()
        || !targetPlayer.connected
        || !targetPlayer.alive
        || targetPlayer.invincible
    )
    {
        return;
    }

    const Vec3 origin{bot.x, bot.y, bot.z + FireOriginHeight};
    Vec3 direction{
        targetPlayer.x - origin.x,
        targetPlayer.y - origin.y,
        (targetPlayer.z + targetPlayer.bodyHeight) - origin.z,
    };
    if (!Normalize(direction))
    {
        direction = Vec3{1.0, 0.0, 0.0};
    }

    const double spreadSeed =
        (serverTimeSeconds * 31.0)
        + (static_cast<double>(bot.botId) * 101.0)
        + (static_cast<double>(bot.ammo) * 7.0);
    direction = ApplyConeSpread(direction, bot.aimSpreadDegrees, spreadSeed);

    bot.ConsumeAmmo();
    SpawnBotProjectile(bot, direction.x, direction.y, direction.z);

    bool bHit = false;
    bool bHeadshot = false;
    int damage = 0;
    double hitDistance = 0.0;

    const Vec3 headCenter{
        targetPlayer.x,
        targetPlayer.y,
        targetPlayer.z + targetPlayer.headHeight,
    };
    if (
        RaySphereIntersection(
            origin,
            direction,
            headCenter,
            targetPlayer.headRadius,
            HitscanRange,
            hitDistance
        )
    )
    {
        bHit = true;
        bHeadshot = true;
        damage = botHeadshotDamage;
    }
    else
    {
        const Vec3 bodyCenter{
            targetPlayer.x,
            targetPlayer.y,
            targetPlayer.z + targetPlayer.bodyHeight,
        };
        if (
            RaySphereIntersection(
                origin,
                direction,
                bodyCenter,
                targetPlayer.bodyRadius,
                HitscanRange,
                hitDistance
            )
        )
        {
            bHit = true;
            damage = botAttackDamage;
        }
    }

    if (!bHit)
    {
        if (bVerboseBotShotEvents)
        {
            CombatEvent missEvent;
            missEvent.type = "bot_shot_miss";
            missEvent.message = bot.name + " missed " + targetPlayer.nickname;
            missEvent.shortMessage = bot.name + " missed";
            missEvent.botId = bot.botId;
            missEvent.targetPlayerId = targetPlayer.playerId;
            missEvent.killerIsBot = true;
            missEvent.hitGroup = "miss";
            AddCombatEvent(missEvent);
            Logger::Info(
                "Bot shot missed bot_id=" + std::to_string(bot.botId)
                + " player_id=" + std::to_string(targetPlayer.playerId)
            );
        }
        return;
    }

    targetPlayer.hp = std::max(0, targetPlayer.hp - damage);

    const Vec3 hitPoint = BuildHitPoint(origin, direction, hitDistance);
    CombatEvent hitEvent;
    hitEvent.type = "bot_shot_hit_player";
    hitEvent.message = bot.name
        + (bHeadshot ? " headshot " : " hit ")
        + targetPlayer.nickname
        + " for "
        + std::to_string(damage);
    hitEvent.shortMessage = bot.name
        + (bHeadshot ? " HEADSHOT " : " HIT ")
        + targetPlayer.nickname
        + " -"
        + std::to_string(damage);
    hitEvent.botId = bot.botId;
    hitEvent.targetPlayerId = targetPlayer.playerId;
    hitEvent.headshot = bHeadshot;
    hitEvent.killerIsBot = true;
    hitEvent.victimIsPlayer = true;
    hitEvent.damage = damage;
    hitEvent.hitGroup = bHeadshot ? "head" : "body";
    hitEvent.hitX = hitPoint.x;
    hitEvent.hitY = hitPoint.y;
    hitEvent.hitZ = hitPoint.z;
    AddCombatEvent(hitEvent);

    std::ostringstream hitLogMessage;
    hitLogMessage
        << "Bot shot hit bot_id=" << bot.botId
        << " player_id=" << targetPlayer.playerId
        << " damage=" << damage
        << " headshot=" << (bHeadshot ? "true" : "false")
        << " hp=" << targetPlayer.hp
        << "/" << targetPlayer.maxHp;
    Logger::Info(hitLogMessage.str());

    if (targetPlayer.hp > 0 || !targetPlayer.alive)
    {
        return;
    }

    targetPlayer.alive = false;
    targetPlayer.invincible = false;
    targetPlayer.deaths += 1;
    targetPlayer.respawnTimerSeconds = PlayerRespawnSeconds;
    targetPlayer.invincibleTimerSeconds = 0.0;
    targetPlayer.latestInput.fire = false;
    targetPlayer.latestInput.moveX = 0.0;
    targetPlayer.latestInput.moveY = 0.0;

    std::ostringstream killLogMessage;
    killLogMessage
        << "Bot killed player bot_id=" << bot.botId
        << " player_id=" << targetPlayer.playerId;
    Logger::Info(killLogMessage.str());

    CombatEvent killEvent;
    killEvent.type = "bot_killed_player";
    killEvent.message = bot.name + " killed " + targetPlayer.nickname;
    killEvent.shortMessage = bot.name + " killed " + targetPlayer.nickname;
    killEvent.botId = bot.botId;
    killEvent.targetPlayerId = targetPlayer.playerId;
    killEvent.victimIsPlayer = true;
    killEvent.killerIsBot = true;
    AddCombatEvent(killEvent);
}

void GameRoom::UpdateBotAI(double deltaSeconds)
{
    for (auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);

        if (!bot.IsAlive())
        {
            continue;
        }

        bot.TickWeapon(deltaSeconds);
        bot.decisionTimerSeconds -= deltaSeconds;

        PlayerState* targetPlayer = nullptr;
        double bestDistanceSquared = botDetectRange * botDetectRange;

        for (auto& [playerId, player] : players)
        {
            static_cast<void>(playerId);

            if (!player.connected || !player.alive || player.invincible)
            {
                continue;
            }

            const double deltaX = player.x - bot.x;
            const double deltaY = player.y - bot.y;
            const double distanceSquared = (deltaX * deltaX) + (deltaY * deltaY);
            if (distanceSquared <= bestDistanceSquared)
            {
                bestDistanceSquared = distanceSquared;
                targetPlayer = &player;
            }
        }

        double moveX = 0.0;
        double moveY = 0.0;

        if (targetPlayer)
        {
            bot.targetPlayerId = targetPlayer->playerId;
            moveX = targetPlayer->x - bot.x;
            moveY = targetPlayer->y - bot.y;
            const double distance = std::sqrt((moveX * moveX) + (moveY * moveY));

            if (distance > 0.0001)
            {
                bot.yaw = std::atan2(moveY, moveX) * 180.0 / 3.14159265358979323846;
            }

            const double effectiveAttackRange = bot.attackRange > 0.0
                ? bot.attackRange
                : botAttackRange;
            if (distance <= effectiveAttackRange)
            {
                // Bot attacks disabled means bots stop shooting; players can
                // still damage bots through server hitscan.
                ProcessBotShot(bot, *targetPlayer);
                if (distance <= bot.preferredCombatRange)
                {
                    continue;
                }
            }

            if (distance > 0.0001)
            {
                moveX /= distance;
                moveY /= distance;
            }
        }
        else
        {
            bot.targetPlayerId = 0;
            const double deltaX = bot.wanderTargetX - bot.x;
            const double deltaY = bot.wanderTargetY - bot.y;
            const double distance = std::sqrt((deltaX * deltaX) + (deltaY * deltaY));

            if (distance < 80.0 || bot.decisionTimerSeconds <= 0.0)
            {
                const double botFactor = static_cast<double>(bot.botId);
                bot.wanderTargetX = ClampX(
                    std::fmod(
                        (botFactor * 733.0) + (bot.x * 0.37) + ArenaWidth,
                        ArenaWidth
                    ) + ArenaMinX
                );
                bot.wanderTargetY = ClampY(
                    std::fmod(
                        (botFactor * 419.0) + (bot.y * 0.53) + ArenaHeight,
                        ArenaHeight
                    ) + ArenaMinY
                );
                bot.decisionTimerSeconds = 2.0 + static_cast<double>(bot.botId % 3);
            }

            moveX = bot.wanderTargetX - bot.x;
            moveY = bot.wanderTargetY - bot.y;
            const double moveLength = std::sqrt((moveX * moveX) + (moveY * moveY));
            if (moveLength > 0.0001)
            {
                moveX /= moveLength;
                moveY /= moveLength;
                bot.yaw = std::atan2(moveY, moveX) * 180.0 / 3.14159265358979323846;
            }
        }

        bot.x = ClampX(bot.x + (moveX * bot.speed * deltaSeconds));
        bot.y = ClampY(bot.y + (moveY * bot.speed * deltaSeconds));
    }
}

void GameRoom::UpdateHealthPacks(double deltaSeconds)
{
    for (auto& [healthPackId, healthPack] : healthPacks)
    {
        if (healthPack.IsActive())
        {
            continue;
        }

        healthPack.respawnTimerSeconds -= deltaSeconds;
        if (healthPack.respawnTimerSeconds > 0.0)
        {
            continue;
        }

        ++healthPackRespawnCounter;
        const std::pair<double, double> spawnPoint =
            ChooseHealthPackSpawnPoint(healthPackId);
        healthPack.Respawn(spawnPoint.first, spawnPoint.second);

        std::ostringstream logMessage;
        logMessage
            << "Health pack respawned health_pack_id=" << healthPack.healthPackId
            << " x=" << healthPack.x
            << " y=" << healthPack.y;
        Logger::Info(logMessage.str());
    }

    CheckHealthPackPickups();
}

void GameRoom::CheckHealthPackPickups()
{
    for (auto& [healthPackId, healthPack] : healthPacks)
    {
        static_cast<void>(healthPackId);

        if (!healthPack.IsActive())
        {
            continue;
        }

        for (auto& [playerId, player] : players)
        {
            static_cast<void>(playerId);

            if (!player.connected || !player.alive || player.hp >= player.maxHp)
            {
                continue;
            }

            const double deltaX = player.x - healthPack.x;
            const double deltaY = player.y - healthPack.y;
            const double pickupRadiusSquared =
                healthPack.pickupRadius * healthPack.pickupRadius;
            const double distanceSquared = (deltaX * deltaX) + (deltaY * deltaY);
            if (distanceSquared > pickupRadiusSquared)
            {
                continue;
            }

            player.hp = std::min(player.maxHp, player.hp + healthPack.healAmount);
            healthPack.Deactivate();

            std::ostringstream logMessage;
            logMessage
                << "Player picked health pack player_id=" << player.playerId
                << " health_pack_id=" << healthPack.healthPackId
                << " hp=" << player.hp
                << "/" << player.maxHp;
            Logger::Info(logMessage.str());

            CombatEvent event;
            event.type = "health_pack_picked";
            event.message = player.nickname + " picked up HealthPack +" + std::to_string(healthPack.healAmount);
            event.actorPlayerId = player.playerId;
            event.healthPackId = healthPack.healthPackId;
            AddCombatEvent(event);
            break;
        }
    }
}

std::pair<double, double> GameRoom::ChooseHealthPackSpawnPoint(
    std::uint64_t healthPackId
) const
{
    InitializeHealthPackSpawnPoints();
    if (healthPackSpawnPoints.empty())
    {
        return {0.0, 0.0};
    }

    const std::size_t spawnIndex = static_cast<std::size_t>(
        (healthPackId + healthPackRespawnCounter) % healthPackSpawnPoints.size()
    );
    return healthPackSpawnPoints[spawnIndex];
}

void GameRoom::AddCombatEvent(const std::string& type, const std::string& message)
{
    CombatEvent event;
    event.type = type;
    event.message = message;
    AddCombatEvent(event);
}

void GameRoom::AddCombatEvent(const CombatEvent& event)
{
    CombatEvent storedEvent = event;
    storedEvent.eventId = nextEventId++;
    if (storedEvent.serverTimeSeconds <= 0.0)
    {
        storedEvent.serverTimeSeconds = serverTimeSeconds;
    }

    recentEvents.push_back(storedEvent);
    while (recentEvents.size() > MaxRecentEvents)
    {
        recentEvents.pop_front();
    }

    Logger::Info("Event: " + storedEvent.message);
}

nlohmann::json GameRoom::BuildEventsJson() const
{
    nlohmann::json eventsJson = nlohmann::json::array();
    for (const CombatEvent& event : recentEvents)
    {
        eventsJson.push_back(event.ToJson());
    }

    return eventsJson;
}

void GameRoom::CheckMatchEndCondition()
{
    if (matchState.IsGameOver() && matchState.winnerPlayerId != 0)
    {
        return;
    }

    bool bShouldEndMatch = matchState.IsGameOver();
    if (!bShouldEndMatch)
    {
        for (const auto& [playerId, player] : players)
        {
            static_cast<void>(playerId);

            if (player.connected && CalculateKillRaceScore(player) >= matchState.targetScore)
            {
                bShouldEndMatch = true;
                break;
            }
        }
    }

    if (!bShouldEndMatch)
    {
        return;
    }

    const std::uint64_t winnerPlayerId = DetermineWinnerPlayerId();
    const auto winner = players.find(winnerPlayerId);
    const std::string winnerNickname = winner != players.end()
        ? winner->second.nickname
        : std::string();
    const int winnerScore = winner != players.end() ? CalculateKillRaceScore(winner->second) : 0;

    matchState.EndMatch(winnerPlayerId, winnerNickname);

    std::ostringstream logMessage;
    logMessage
        << "Match ended winner=" << winnerPlayerId
        << " nickname=" << winnerNickname
        << " score=" << winnerScore;
    Logger::Info(logMessage.str());

    CombatEvent event;
    event.type = "match_ended";
    event.message = "Match ended. Winner: "
        + (winnerNickname.empty() ? std::string("P") + std::to_string(winnerPlayerId) : winnerNickname);
    event.actorPlayerId = winnerPlayerId;
    AddCombatEvent(event);
}

std::uint64_t GameRoom::DetermineWinnerPlayerId() const
{
    std::uint64_t bestPlayerId = 0;
    const PlayerState* bestPlayer = nullptr;

    for (const auto& [playerId, player] : players)
    {
        if (!player.connected)
        {
            continue;
        }

        if (!bestPlayer)
        {
            bestPlayer = &player;
            bestPlayerId = playerId;
            continue;
        }

        const int playerScore = CalculateKillRaceScore(player);
        const int bestScore = CalculateKillRaceScore(*bestPlayer);

        const bool bIsBetter =
            playerScore > bestScore
            || (
                playerScore == bestScore
                && player.playerKills > bestPlayer->playerKills
            )
            || (
                playerScore == bestScore
                && player.playerKills == bestPlayer->playerKills
                && player.botKills > bestPlayer->botKills
            )
            || (
                playerScore == bestScore
                && player.playerKills == bestPlayer->playerKills
                && player.botKills == bestPlayer->botKills
                && player.deaths < bestPlayer->deaths
            )
            || (
                playerScore == bestScore
                && player.playerKills == bestPlayer->playerKills
                && player.botKills == bestPlayer->botKills
                && player.deaths == bestPlayer->deaths
                && player.playerId < bestPlayer->playerId
            );

        if (bIsBetter)
        {
            bestPlayer = &player;
            bestPlayerId = playerId;
        }
    }

    return bestPlayerId;
}

nlohmann::json GameRoom::BuildScoreboardJson() const
{
    std::vector<const PlayerState*> sortedPlayers;
    sortedPlayers.reserve(players.size());

    for (const auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);
        if (player.connected)
        {
            sortedPlayers.push_back(&player);
        }
    }

    std::sort(
        sortedPlayers.begin(),
        sortedPlayers.end(),
        [](const PlayerState* lhs, const PlayerState* rhs)
        {
            const int lhsScore = CalculateKillRaceScore(*lhs);
            const int rhsScore = CalculateKillRaceScore(*rhs);
            if (lhsScore != rhsScore)
            {
                return lhsScore > rhsScore;
            }
            if (lhs->playerKills != rhs->playerKills)
            {
                return lhs->playerKills > rhs->playerKills;
            }
            if (lhs->botKills != rhs->botKills)
            {
                return lhs->botKills > rhs->botKills;
            }
            if (lhs->deaths != rhs->deaths)
            {
                return lhs->deaths < rhs->deaths;
            }
            return lhs->playerId < rhs->playerId;
        }
    );

    nlohmann::json scoreboard = nlohmann::json::array();
    for (const PlayerState* player : sortedPlayers)
    {
        nlohmann::json entry;
        entry["player_id"] = player->playerId;
        entry["nickname"] = player->nickname;
        const int killRaceScore = CalculateKillRaceScore(*player);
        entry["score"] = killRaceScore;
        entry["kills"] = killRaceScore;
        entry["deaths"] = player->deaths;
        entry["bot_kills"] = player->botKills;
        entry["player_kills"] = player->playerKills;
        entry["target_kills"] = player->targetKills;
        entry["hp"] = player->hp;
        entry["alive"] = player->alive;
        scoreboard.push_back(std::move(entry));
    }

    return scoreboard;
}

void GameRoom::ProcessHitscanFire(PlayerState& shooter, const PlayerInput& input)
{
    if (!shooter.alive)
    {
        return;
    }

    if (matchState.IsGameOver())
    {
        return;
    }

    const Vec3 origin{shooter.x, shooter.y, shooter.z + FireOriginHeight};
    const Vec3 direction = BuildShotDirection(input);

    std::ostringstream fireLogMessage;
    fireLogMessage
        << "Hitscan fire shooter=" << shooter.playerId
        << " seq=" << input.seq
        << " origin=(" << origin.x << "," << origin.y << "," << origin.z << ")"
        << " dir=(" << direction.x << "," << direction.y << "," << direction.z << ")"
        << " ads=" << (input.ads ? "true" : "false")
        << " spread=" << input.spreadDegrees;
    Logger::Info(fireLogMessage.str());

    HitscanHit bestHit;

    if (bTargetsEnabled)
    {
        for (const auto& [targetId, target] : targets)
        {
            if (!target.IsAlive())
            {
                continue;
            }

            double hitDistance = 0.0;
            const Vec3 center{target.x, target.y, TargetCenterZ};
            if (
                RaySphereIntersection(
                    origin,
                    direction,
                    center,
                    target.radius,
                    HitscanRange,
                    hitDistance
                )
                && hitDistance < bestHit.distance
            )
            {
                bestHit.type = HitscanHit::Type::Target;
                bestHit.distance = hitDistance;
                bestHit.targetId = targetId;
                bestHit.victimPlayerId = 0;
                bestHit.headshot = false;
                bestHit.used2dFallback = false;
                bestHit.damage = BodyDamage;
            }
        }
    }

    for (const auto& [botId, bot] : bots)
    {
        // Bot attack toggles only stop bots from damaging players. Bots remain
        // valid player hitscan targets for movement/combat demos.
        if (!bot.CanBeDamaged())
        {
            continue;
        }

        double hitDistance = 0.0;
        const Vec3 headCenter{bot.x, bot.y, bot.z + bot.headHeight};
        if (
            RaySphereIntersection(
                origin,
                direction,
                headCenter,
                bot.headRadius,
                HitscanRange,
                hitDistance
            )
            && hitDistance < bestHit.distance
        )
        {
            bestHit.type = HitscanHit::Type::Bot;
            bestHit.distance = hitDistance;
            bestHit.targetId = 0;
            bestHit.victimPlayerId = 0;
            bestHit.botId = botId;
            bestHit.headshot = true;
            bestHit.used2dFallback = false;
            bestHit.damage = HeadshotDamage;
            continue;
        }

        const Vec3 bodyCenter{bot.x, bot.y, bot.z + bot.bodyHeight};
        if (
            RaySphereIntersection(
                origin,
                direction,
                bodyCenter,
                bot.bodyRadius,
                HitscanRange,
                hitDistance
            )
            && hitDistance < bestHit.distance
        )
        {
            bestHit.type = HitscanHit::Type::Bot;
            bestHit.distance = hitDistance;
            bestHit.targetId = 0;
            bestHit.victimPlayerId = 0;
            bestHit.botId = botId;
            bestHit.headshot = false;
            bestHit.used2dFallback = false;
            bestHit.damage = BodyDamage;
        }

        double fallbackDistance = 0.0;
        if (
            RayCircleIntersection2D(
                origin,
                direction,
                bot.x,
                bot.y,
                bot.bodyRadius,
                HitscanRange,
                fallbackDistance
            )
            && fallbackDistance < bestHit.distance
        )
        {
            bestHit.type = HitscanHit::Type::Bot;
            bestHit.distance = fallbackDistance;
            bestHit.targetId = 0;
            bestHit.victimPlayerId = 0;
            bestHit.botId = botId;
            bestHit.headshot = false;
            bestHit.used2dFallback = true;
            bestHit.damage = BodyDamage;
        }
    }

    for (const auto& [victimId, victim] : players)
    {
        if (
            victimId == shooter.playerId
            || !victim.connected
            || !victim.alive
            || victim.invincible
        )
        {
            continue;
        }

        double hitDistance = 0.0;
        const Vec3 headCenter{victim.x, victim.y, victim.z + victim.headHeight};
        if (
            RaySphereIntersection(
                origin,
                direction,
                headCenter,
                victim.headRadius,
                HitscanRange,
                hitDistance
            )
            && hitDistance < bestHit.distance
        )
        {
            bestHit.type = HitscanHit::Type::Player;
            bestHit.distance = hitDistance;
            bestHit.targetId = 0;
            bestHit.victimPlayerId = victimId;
            bestHit.headshot = true;
            bestHit.used2dFallback = false;
            bestHit.damage = HeadshotDamage;
            continue;
        }

        const Vec3 bodyCenter{victim.x, victim.y, victim.z + victim.bodyHeight};
        if (
            RaySphereIntersection(
                origin,
                direction,
                bodyCenter,
                victim.bodyRadius,
                HitscanRange,
                hitDistance
            )
            && hitDistance < bestHit.distance
        )
        {
            bestHit.type = HitscanHit::Type::Player;
            bestHit.distance = hitDistance;
            bestHit.targetId = 0;
            bestHit.victimPlayerId = victimId;
            bestHit.headshot = false;
            bestHit.used2dFallback = false;
            bestHit.damage = BodyDamage;
        }
    }

    if (bestHit.type == HitscanHit::Type::Target)
    {
        const auto target = targets.find(bestHit.targetId);
        if (target == targets.end())
        {
            return;
        }

        target->second.ApplyDamage(bestHit.damage);

        std::ostringstream hitLogMessage;
        hitLogMessage
            << "Hitscan target hit shooter=" << shooter.playerId
            << " target=" << bestHit.targetId
            << " damage=" << bestHit.damage
            << " hp=" << target->second.hp
            << "/" << target->second.maxHp;
        Logger::Info(hitLogMessage.str());

        const Vec3 hitPoint = BuildHitPoint(origin, direction, bestHit.distance);
        CombatEvent shotEvent;
        shotEvent.type = "shot_hit_target";
        shotEvent.message = shooter.nickname
            + " hit CORE-"
            + std::to_string(bestHit.targetId)
            + " for "
            + std::to_string(bestHit.damage);
        shotEvent.shortMessage = BuildShotShortMessage(
            bestHit.type,
            bestHit.targetId,
            bestHit.damage,
            bestHit.headshot
        );
        shotEvent.actorPlayerId = shooter.playerId;
        shotEvent.targetId = bestHit.targetId;
        shotEvent.damage = bestHit.damage;
        shotEvent.hitGroup = "core";
        shotEvent.hitX = hitPoint.x;
        shotEvent.hitY = hitPoint.y;
        shotEvent.hitZ = hitPoint.z;
        AddCombatEvent(shotEvent);
        LogShotResult(
            shooter.playerId,
            true,
            bestHit.type,
            bestHit.damage,
            bestHit.headshot
        );

        if (!target->second.IsAlive())
        {
            shooter.targetKills += 1;
            shooter.score = CalculateKillRaceScore(shooter);

            std::ostringstream destroyLogMessage;
            destroyLogMessage
                << "Server target destroyed target_id=" << bestHit.targetId
                << " shooter=" << shooter.playerId
                << " score=" << shooter.score;
            Logger::Info(destroyLogMessage.str());

            CombatEvent event;
            event.type = "target_destroyed";
            event.message = shooter.nickname + " destroyed CORE-" + std::to_string(bestHit.targetId);
            event.actorPlayerId = shooter.playerId;
            event.targetId = bestHit.targetId;
            AddCombatEvent(event);
        }

        return;
    }

    if (bestHit.type == HitscanHit::Type::Bot)
    {
        const auto bot = bots.find(bestHit.botId);
        if (bot == bots.end())
        {
            return;
        }

        BotState& hitBot = bot->second;
        hitBot.ApplyDamage(bestHit.damage);

        std::ostringstream hitLogMessage;
        hitLogMessage
            << "Hitscan bot hit shooter=" << shooter.playerId
            << " bot=" << hitBot.botId
            << " headshot=" << (bestHit.headshot ? "true" : "false")
            << " damage=" << bestHit.damage
            << " hp=" << hitBot.hp
            << "/" << hitBot.maxHp;
        Logger::Info(hitLogMessage.str());

        if (bestHit.used2dFallback)
        {
            std::ostringstream fallbackLogMessage;
            fallbackLogMessage
                << "Hitscan bot body hit by 2D fallback shooter=" << shooter.playerId
                << " bot=" << hitBot.botId
                << " damage=" << bestHit.damage
                << " hp=" << hitBot.hp
                << "/" << hitBot.maxHp;
            Logger::Info(fallbackLogMessage.str());
        }

        const Vec3 hitPoint = BuildHitPoint(origin, direction, bestHit.distance);
        CombatEvent shotEvent;
        shotEvent.type = "shot_hit_bot";
        shotEvent.message = shooter.nickname
            + (bestHit.headshot ? " headshot " : " hit ")
            + hitBot.name
            + " for "
            + std::to_string(bestHit.damage);
        shotEvent.shortMessage = BuildShotShortMessage(
            bestHit.type,
            hitBot.botId,
            bestHit.damage,
            bestHit.headshot
        );
        shotEvent.actorPlayerId = shooter.playerId;
        shotEvent.botId = hitBot.botId;
        shotEvent.headshot = bestHit.headshot;
        shotEvent.damage = bestHit.damage;
        shotEvent.hitGroup = bestHit.headshot ? "head" : "body";
        shotEvent.hitX = hitPoint.x;
        shotEvent.hitY = hitPoint.y;
        shotEvent.hitZ = hitPoint.z;
        AddCombatEvent(shotEvent);
        LogShotResult(
            shooter.playerId,
            true,
            bestHit.type,
            bestHit.damage,
            bestHit.headshot
        );

        if (!hitBot.IsAlive())
        {
            hitBot.respawnTimerSeconds = BotRespawnSeconds;
            shooter.kills += 1;
            shooter.botKills += 1;
            shooter.score = CalculateKillRaceScore(shooter);

            std::ostringstream killLogMessage;
            killLogMessage
                << "Bot killed bot_id=" << hitBot.botId
                << " shooter=" << shooter.playerId
                << " score=" << shooter.score;
            Logger::Info(killLogMessage.str());

            CombatEvent event;
            event.type = "bot_killed";
            event.message = shooter.nickname
                + (bestHit.headshot ? " headshot " : " killed ")
                + hitBot.name;
            event.actorPlayerId = shooter.playerId;
            event.botId = hitBot.botId;
            event.headshot = bestHit.headshot;
            event.victimIsBot = true;
            event.killerIsPlayer = true;
            AddCombatEvent(event);
        }

        return;
    }

    if (bestHit.type == HitscanHit::Type::Player)
    {
        const auto victim = players.find(bestHit.victimPlayerId);
        if (victim == players.end())
        {
            return;
        }

        PlayerState& victimPlayer = victim->second;
        victimPlayer.hp = std::max(0, victimPlayer.hp - bestHit.damage);

        std::ostringstream hitLogMessage;
        hitLogMessage
            << "Hitscan player hit shooter=" << shooter.playerId
            << " victim=" << victimPlayer.playerId
            << " damage=" << bestHit.damage
            << " headshot=" << (bestHit.headshot ? "true" : "false")
            << " hp=" << victimPlayer.hp
            << "/" << victimPlayer.maxHp;
        Logger::Info(hitLogMessage.str());

        const Vec3 hitPoint = BuildHitPoint(origin, direction, bestHit.distance);
        CombatEvent shotEvent;
        shotEvent.type = "shot_hit_player";
        shotEvent.message = shooter.nickname
            + (bestHit.headshot ? " headshot " : " hit ")
            + victimPlayer.nickname
            + " for "
            + std::to_string(bestHit.damage);
        shotEvent.shortMessage = BuildShotShortMessage(
            bestHit.type,
            victimPlayer.playerId,
            bestHit.damage,
            bestHit.headshot
        );
        shotEvent.actorPlayerId = shooter.playerId;
        shotEvent.targetPlayerId = victimPlayer.playerId;
        shotEvent.headshot = bestHit.headshot;
        shotEvent.damage = bestHit.damage;
        shotEvent.hitGroup = bestHit.headshot ? "head" : "body";
        shotEvent.hitX = hitPoint.x;
        shotEvent.hitY = hitPoint.y;
        shotEvent.hitZ = hitPoint.z;
        AddCombatEvent(shotEvent);
        LogShotResult(
            shooter.playerId,
            true,
            bestHit.type,
            bestHit.damage,
            bestHit.headshot
        );

        if (victimPlayer.hp <= 0)
        {
            victimPlayer.alive = false;
            victimPlayer.invincible = false;
            victimPlayer.deaths += 1;
            victimPlayer.respawnTimerSeconds = PlayerRespawnSeconds;
            victimPlayer.invincibleTimerSeconds = 0.0;
            victimPlayer.latestInput.fire = false;
            victimPlayer.latestInput.moveX = 0.0;
            victimPlayer.latestInput.moveY = 0.0;

            shooter.kills += 1;
            shooter.playerKills += 1;
            shooter.score = CalculateKillRaceScore(shooter);

            std::ostringstream killLogMessage;
            killLogMessage
                << "Player killed killer=" << shooter.playerId
                << " victim=" << victimPlayer.playerId
                << " headshot=" << (bestHit.headshot ? "true" : "false")
                << " killer_score=" << shooter.score;
            Logger::Info(killLogMessage.str());

            CombatEvent event;
            event.type = "player_killed";
            event.message = shooter.nickname
                + (bestHit.headshot ? " headshot " : " killed ")
                + victimPlayer.nickname;
            event.actorPlayerId = shooter.playerId;
            event.targetPlayerId = victimPlayer.playerId;
            event.headshot = bestHit.headshot;
            event.victimIsPlayer = true;
            event.killerIsPlayer = true;
            AddCombatEvent(event);
        }

        return;
    }

    Logger::Info(
        "Hitscan missed shooter=" + std::to_string(shooter.playerId)
        + " seq=" + std::to_string(input.seq)
    );

    CombatEvent missEvent;
    missEvent.type = "shot_miss";
    missEvent.message = shooter.nickname + " missed";
    missEvent.shortMessage = BuildShotShortMessage(
        HitscanHit::Type::None,
        0,
        0,
        false
    );
    missEvent.actorPlayerId = shooter.playerId;
    missEvent.hitGroup = "miss";
    missEvent.hitX = origin.x;
    missEvent.hitY = origin.y;
    missEvent.hitZ = origin.z;
    AddCombatEvent(missEvent);
    LogShotResult(
        shooter.playerId,
        false,
        HitscanHit::Type::None,
        0,
        false
    );
}

void GameRoom::UpdateProjectiles(double deltaSeconds)
{
    for (auto& [projectileId, projectile] : projectiles)
    {
        static_cast<void>(projectileId);

        if (!projectile.active)
        {
            continue;
        }

        projectile.x += projectile.dirX * projectile.speed * deltaSeconds;
        projectile.y += projectile.dirY * projectile.speed * deltaSeconds;
        projectile.z += projectile.dirZ * projectile.speed * deltaSeconds;
        projectile.ageSeconds += deltaSeconds;

        const double lifetime = projectile.lifeTimeSeconds > 0.0
            ? projectile.lifeTimeSeconds
            : projectile.maxLifetimeSeconds;
        if (
            projectile.ageSeconds > lifetime
            || (!projectile.visualOnly && !IsInsideArena(projectile.x, projectile.y))
        )
        {
            projectile.active = false;
        }
    }
}

void GameRoom::UpdateProjectileTargetCollisions()
{
    if (!bTargetsEnabled)
    {
        return;
    }

    for (auto& [projectileId, projectile] : projectiles)
    {
        if (!projectile.active)
        {
            continue;
        }

        if (projectile.visualOnly)
        {
            continue;
        }

        for (auto& [targetId, target] : targets)
        {
            if (!target.IsAlive())
            {
                continue;
            }

            const double deltaX = projectile.x - target.x;
            const double deltaY = projectile.y - target.y;
            const double collisionRadius = projectile.radius + target.radius;
            const double distanceSquared = (deltaX * deltaX) + (deltaY * deltaY);

            if (distanceSquared > collisionRadius * collisionRadius)
            {
                continue;
            }

            target.ApplyDamage(projectile.damage);
            projectile.active = false;

            std::ostringstream hitLogMessage;
            hitLogMessage
                << "Projectile hit target projectile_id=" << projectileId
                << " target_id=" << targetId
                << " hp=" << target.hp
                << "/" << target.maxHp;
            Logger::Info(hitLogMessage.str());

            if (!target.IsAlive())
            {
                const auto owner = players.find(projectile.ownerPlayerId);
                int score = 0;
                if (owner != players.end())
                {
                    owner->second.targetKills += 1;
                    owner->second.score = CalculateKillRaceScore(owner->second);
                    CombatEvent event;
                    event.type = "target_destroyed";
                    event.message = owner->second.nickname + " destroyed CORE-" + std::to_string(targetId);
                    event.actorPlayerId = owner->second.playerId;
                    event.targetId = targetId;
                    AddCombatEvent(event);
                    score = owner->second.score;
                }

                std::ostringstream destroyLogMessage;
                destroyLogMessage
                    << "Target destroyed target_id=" << targetId
                    << " owner=" << projectile.ownerPlayerId
                    << " score=" << score;
                Logger::Info(destroyLogMessage.str());
            }

            break;
        }
    }
}

void GameRoom::RemoveInactiveProjectiles()
{
    for (auto iterator = projectiles.begin(); iterator != projectiles.end();)
    {
        if (!iterator->second.active)
        {
            iterator = projectiles.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}
}
