#include "game/GameRoom.h"

#include "core/Logger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace battlegrid
{
namespace
{
constexpr double MinPlayerZ = -1000.0;
constexpr double MaxPlayerZ = 5000.0;

int CalculateKillRaceScore(const PlayerState& player)
{
    return player.botKills + player.playerKills;
}
constexpr double BotStuckMovementThreshold = 10.0;
constexpr double BotStuckTimeoutSeconds = 2.0;
constexpr double TwoPi = 6.28318530717958647692;
constexpr double ProjectileSpawnForwardOffset = 50.0;
constexpr int BodyDamage = 20;
constexpr int HeadshotDamage = 40;
constexpr int BotBodyDamage = 4;
constexpr int BotHeadshotDamage = 8;
constexpr double DefaultBotBodyHitboxScale = 0.64;
constexpr double DefaultBotHeadHitboxScale = 0.64;
constexpr double DefaultPlayerBodyHitboxScale = 0.64;
constexpr double DefaultPlayerHeadshotHitboxScale = 0.64;
constexpr double MinHitboxScale = 0.3;
constexpr double MaxHitboxScale = 1.5;
constexpr double HitscanRange = 3000.0;
constexpr double VisualTracerLength = 1800.0;
constexpr double VisualTracerLifetimeSeconds = 0.35;
constexpr double BotTracerLifetimeSeconds = 0.4;
constexpr double PlayerRespawnSeconds = 8.0;
constexpr int HealthPackHealAmount = 35;
constexpr double HealthPackPickupRadius = 120.0;
constexpr double HealthPackRespawnSeconds = 15.0;
constexpr int MaxActiveHealthPacks = 3;
constexpr std::size_t RuntimeMarkerBotCount = 8;
constexpr double PlayerSpawnSeparationRadius = 350.0;
constexpr int BotMaxHp = 100;
constexpr double BotRespawnSeconds = 8.0;
constexpr double BotInvincibleSeconds = 1.5;
constexpr int BotKillScore = 1;
constexpr int PlayerKillScore = 1;
constexpr int TargetKillScore = 0;
constexpr double WalkSpeed = 600.0;
constexpr double SprintSpeed = 850.0;
constexpr double AdsWalkSpeed = 400.0;
constexpr const char* SafeDemoDifficulty = "easy";
constexpr bool bSafeDemoBotAttacksEnabled = true;
constexpr bool bSafeDemoAutoEndMatchByTimer = false;
constexpr bool bUseHitscanDamage = true;
constexpr bool bProjectileCollisionDamageEnabled = false;
constexpr double FireOriginHeight = 100.0;
constexpr double TargetCenterZ = 80.0;
constexpr double ClientHitClaimCooldownSeconds = 0.18;
constexpr double MaxClientHitClaimBotDistance = 1600.0;
constexpr double MaxClientHitClaimZDelta = 3000.0;

struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct HitscanHitResult
{
    bool hit = false;
    std::string targetType; // "bot", "player", "target"
    std::uint64_t targetId = 0;
    double distance = 0.0;
    int damage = 0;
    bool headshot = false;
    std::string hitGroup; // "head", "body", "fallback_body", "core"
    double hitX = 0.0;
    double hitY = 0.0;
    double hitZ = 0.0;
};

double DeterministicUnit(double seed)
{
    const double value = std::sin(seed) * 43758.5453123;
    return value - std::floor(value);
}

double DeterministicRange(double minValue, double maxValue, double seed)
{
    if (maxValue <= minValue)
    {
        return minValue;
    }

    return minValue + ((maxValue - minValue) * DeterministicUnit(seed));
}

void SetBotCombatState(
    BotState& bot,
    const std::string& state,
    const std::string& reason,
    std::uint64_t targetPlayerId,
    double distanceToTarget,
    bool bWantsToShoot,
    bool bAllowedToShoot,
    int currentShootersForTarget,
    bool bVerboseBotStateLogs
)
{
    const bool bChanged =
        bot.combatState != state
        || bot.fireBlockReason != reason
        || bot.targetPlayerId != targetPlayerId;

    bot.combatState = state;
    bot.fireBlockReason = reason;
    bot.targetPlayerId = targetPlayerId;
    bot.distanceToTarget = distanceToTarget;
    bot.inDetectRange = targetPlayerId != 0;
    bot.inAttackRange = bWantsToShoot;
    bot.hasLineOfFire = true;
    bot.wantsToShoot = bWantsToShoot;
    bot.allowedToShoot = bAllowedToShoot;
    bot.canFire = bAllowedToShoot;
    bot.lastFireReason = reason;
    bot.currentShootersForTarget = currentShootersForTarget;

    if (bChanged && bVerboseBotStateLogs)
    {
        std::ostringstream logMessage;
        logMessage
            << "[BattleGridServer] BotDetect bot=" << bot.botId
            << " target=" << targetPlayerId
            << " dist=" << distanceToTarget
            << " inDetect=" << (bot.inDetectRange ? "true" : "false")
            << " inRange=" << (bot.inAttackRange ? "true" : "false")
            << " canFire=" << (bot.canFire ? "true" : "false")
            << " state=" << state
            << " reason=" << reason;
        Logger::Info(logMessage.str());
    }
}

void ResetBotStuckState(BotState& bot)
{
    bot.stuckTimerSeconds = 0.0;
    bot.lastXForStuck = bot.x;
    bot.lastYForStuck = bot.y;
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
    const double rayDistance = projectedDistance / directionLength;
    if (rayDistance < 0.0 || rayDistance > range)
    {
        return false;
    }

    const double closestX = origin.x + (direction.x * rayDistance);
    const double closestY = origin.y + (direction.y * rayDistance);
    const double missX = centerX - closestX;
    const double missY = centerY - closestY;
    const double missDistanceSquared = (missX * missX) + (missY * missY);
    if (missDistanceSquared > radius * radius)
    {
        return false;
    }

    outDistance = rayDistance;
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

bool IsCloserHit(const HitscanHitResult& candidate, const HitscanHitResult& current)
{
    return candidate.hit && (!current.hit || candidate.distance < current.distance);
}

HitscanHitResult BuildHitResult(
    const std::string& targetType,
    std::uint64_t targetId,
    double distance,
    int damage,
    bool bHeadshot,
    const std::string& hitGroup,
    const Vec3& origin,
    const Vec3& direction
)
{
    const Vec3 hitPoint = BuildHitPoint(origin, direction, distance);

    HitscanHitResult result;
    result.hit = true;
    result.targetType = targetType;
    result.targetId = targetId;
    result.distance = distance;
    result.damage = damage;
    result.headshot = bHeadshot;
    result.hitGroup = hitGroup;
    result.hitX = hitPoint.x;
    result.hitY = hitPoint.y;
    result.hitZ = hitPoint.z;
    return result;
}

std::string GetShotResultTargetName(const std::string& targetType)
{
    if (targetType == "target")
    {
        return "core";
    }
    if (targetType == "player")
    {
        return "player";
    }
    if (targetType == "bot")
    {
        return "bot";
    }
    return "none";
}

void LogShotResult(
    std::uint64_t shooterId,
    const HitscanHitResult& hit,
    bool bVerboseShotResultLogs
)
{
    if (!bVerboseShotResultLogs)
    {
        return;
    }

    std::ostringstream logMessage;
    logMessage
        << "Shot result shooter=" << shooterId
        << " result=" << (hit.hit ? "hit" : "miss")
        << " target=" << GetShotResultTargetName(hit.targetType)
        << " damage=" << hit.damage
        << " headshot=" << (hit.headshot ? "true" : "false");
    Logger::Info(logMessage.str());
}

std::string BuildShotShortMessage(
    const std::string& targetType,
    std::uint64_t targetId,
    int damage,
    bool bHeadshot
)
{
    if (targetType.empty())
    {
        return "SERVER MISS";
    }

    std::string label = "TARGET";
    if (targetType == "bot")
    {
        label = "BOT-" + std::to_string(targetId);
    }
    else if (targetType == "target")
    {
        label = "CORE-" + std::to_string(targetId);
    }
    else if (targetType == "player")
    {
        label = "PLAYER-" + std::to_string(targetId);
    }

    return std::string(bHeadshot ? "SERVER HEADSHOT " : "SERVER HIT ")
        + label
        + " -"
        + std::to_string(damage);
}

nlohmann::json BuildPlayerSnapshotJson(const PlayerState& player)
{
    nlohmann::json playerJson;
    playerJson["player_id"] = player.playerId;
    playerJson["nickname"] = player.nickname;
    playerJson["x"] = player.x;
    playerJson["y"] = player.y;
    playerJson["z"] = player.z;
    playerJson["yaw"] = player.yaw;
    playerJson["hp"] = player.hp;
    playerJson["max_hp"] = player.maxHp;
    playerJson["alive"] = player.alive;
    playerJson["invincible"] = player.invincible;
    playerJson["respawn_timer"] = player.respawnTimerSeconds;
    playerJson["invincible_timer"] = player.invincibleTimerSeconds;
    playerJson["score"] = CalculateKillRaceScore(player);
    playerJson["kills"] = CalculateKillRaceScore(player);
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
    botJson["wander_target_x"] = bot.wanderTargetX;
    botJson["wander_target_y"] = bot.wanderTargetY;
    botJson["current_waypoint_index"] = bot.currentWaypointIndex;
    botJson["stuck_timer"] = bot.stuckTimerSeconds;
    botJson["body_radius"] = bot.bodyRadius;
    botJson["head_radius"] = bot.headRadius;
    botJson["body_hitbox_scale"] = DefaultBotBodyHitboxScale;
    botJson["head_hitbox_scale"] = DefaultBotHeadHitboxScale;
    botJson["effective_body_radius"] = bot.bodyRadius * DefaultBotBodyHitboxScale;
    botJson["effective_head_radius"] = bot.headRadius * DefaultBotHeadHitboxScale;
    botJson["body_height"] = bot.bodyHeight;
    botJson["head_height"] = bot.headHeight;
    return botJson;
}

nlohmann::json BuildBotDebugJson(
    const BotState& bot,
    double detectRange,
    double fallbackAttackRange,
    double botBodyHitboxScale,
    double botHeadHitboxScale
)
{
    nlohmann::json botJson = BuildBotSnapshotJson(bot);
    botJson["body_hitbox_scale"] = botBodyHitboxScale;
    botJson["head_hitbox_scale"] = botHeadHitboxScale;
    botJson["effective_body_radius"] = bot.bodyRadius * botBodyHitboxScale;
    botJson["effective_head_radius"] = bot.headRadius * botHeadHitboxScale;
    botJson["combat_state"] = bot.combatState;
    botJson["fire_block_reason"] = bot.fireBlockReason;
    botJson["last_fire_reason"] = bot.lastFireReason;
    botJson["distance_to_target"] = bot.distanceToTarget;
    const double effectiveAttackRange = bot.attackRange > 0.0
        ? bot.attackRange
        : fallbackAttackRange;
    botJson["in_detect_range"] =
        bot.targetPlayerId != 0 && bot.distanceToTarget <= detectRange;
    botJson["in_attack_range"] =
        bot.targetPlayerId != 0 && bot.distanceToTarget <= effectiveAttackRange;
    botJson["can_fire"] = bot.canFire;
    botJson["weapon_can_fire"] = bot.CanFire();
    botJson["cooldown_remaining"] = bot.fireCooldownSeconds;
    botJson["has_line_of_fire"] = bot.hasLineOfFire;
    botJson["wants_to_shoot"] = bot.wantsToShoot;
    botJson["allowed_to_shoot"] = bot.allowedToShoot;
    botJson["current_shooters_for_target"] = bot.currentShootersForTarget;
    botJson["target_reconsider_timer"] = bot.targetReconsiderTimerSeconds;
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

} // namespace

nlohmann::json GameRoom::BuildArenaPointsJson(
    const std::vector<ArenaPoint>& points,
    const char* idFieldName
) const
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

nlohmann::json GameRoom::BuildArenaBoundsJson() const
{
    nlohmann::json boundsJson;
    boundsJson["min_x"] = arenaBounds.minX;
    boundsJson["max_x"] = arenaBounds.maxX;
    boundsJson["min_y"] = arenaBounds.minY;
    boundsJson["max_y"] = arenaBounds.maxY;
    return boundsJson;
}

nlohmann::json GameRoom::BuildBotAreaBoundsJson() const
{
    nlohmann::json boundsJson;
    boundsJson["min_x"] = botAreaBounds.minX;
    boundsJson["max_x"] = botAreaBounds.maxX;
    boundsJson["min_y"] = botAreaBounds.minY;
    boundsJson["max_y"] = botAreaBounds.maxY;
    return boundsJson;
}

nlohmann::json GameRoom::BuildArenaLayoutJson(
    bool bIncludeSpawnLists,
    bool bIncludeTargetCores
) const
{
    nlohmann::json arenaJson;
    arenaJson["name"] = "BattleGrid PvPvE Kill Race Arena";
    arenaJson["map_profile"] = mapProfile;
    arenaJson["coordinate_mode"] =
        mapProfile == "level_runtime" ? "identity" : "calibrated";
    arenaJson["has_runtime_map_markers"] = bHasRuntimeMapMarkers;
    arenaJson["bounds"] = BuildArenaBoundsJson();
    arenaJson["bot_area_bounds"] = BuildBotAreaBoundsJson();

    if (bIncludeSpawnLists)
    {
        arenaJson["player_spawns"] = BuildArenaPointsJson(playerSpawnPoints, "spawn_id");
        if (bIncludeTargetCores)
        {
            arenaJson["target_cores_debug"] =
                BuildArenaPointsJson(targetCorePositions, "target_id");
        }
        arenaJson["bot_spawns"] = BuildArenaPointsJson(botSpawnPoints, "bot_id");
        arenaJson["bot_waypoints"] = BuildArenaPointsJson(botWanderPoints, "waypoint_id");
        arenaJson["health_pack_spawns"] =
            BuildArenaPointsJson(healthPackProfileSpawnPoints, "spawn_id");
        arenaJson["shared_spawns"] = bHasRuntimeMapMarkers
            ? BuildArenaPointsJson(runtimeSharedSpawnPoints, "spawn_id")
            : BuildArenaPointsJson(playerSpawnPoints, "spawn_id");
        arenaJson["heal_spawns"] = bHasRuntimeMapMarkers
            ? BuildArenaPointsJson(runtimeHealSpawnPoints, "spawn_id")
            : BuildArenaPointsJson(healthPackProfileSpawnPoints, "spawn_id");
    }

    return arenaJson;
}

void GameRoom::ApplyMapProfileUnlocked(const std::string& profileName)
{
    const bool bUsePrototype = profileName == "prototype_arena";
    const bool bUseDemo = profileName == "demo_arena_v1" || profileName.empty();
    if (!bUsePrototype && !bUseDemo)
    {
        Logger::Warn(
            "[BattleGridServer] Unknown map profile '" + profileName
            + "', using demo_arena_v1"
        );
    }

    mapProfile = bUsePrototype ? "prototype_arena" : "demo_arena_v1";
    bHasRuntimeMapMarkers = false;
    runtimeSharedSpawnPoints.clear();
    runtimeHealSpawnPoints.clear();

    targetCorePositions = {
        {1, "CORE-1", 0.0, 0.0, 0.0},
        {2, "CORE-2", 700.0, 500.0, 0.0},
        {3, "CORE-3", 700.0, -500.0, 0.0},
        {4, "CORE-4", -700.0, 500.0, 0.0},
        {5, "CORE-5", -700.0, -500.0, 0.0},
    };

    if (mapProfile == "prototype_arena")
    {
        arenaBounds = {-5000.0, 5000.0, -5000.0, 5000.0};
        botAreaBounds = {-1500.0, 1500.0, -900.0, 900.0};
        playerSpawnPoints = {
            {1, "P1", -1200.0, 0.0, 0.0},
            {2, "P2", 1200.0, 0.0, 0.0},
            {3, "P3", 0.0, 900.0, 0.0},
            {4, "P4", 0.0, -900.0, 0.0},
        };
        botSpawnPoints = {
            {1, "BOT-1", -900.0, 500.0, 0.0},
            {2, "BOT-2", -900.0, -500.0, 0.0},
            {3, "BOT-3", -300.0, 500.0, 0.0},
            {4, "BOT-4", -300.0, -500.0, 0.0},
            {5, "BOT-5", 300.0, 500.0, 0.0},
            {6, "BOT-6", 300.0, -500.0, 0.0},
            {7, "BOT-7", 900.0, 500.0, 0.0},
            {8, "BOT-8", 900.0, -500.0, 0.0},
        };
        botWanderPoints = {
            {1, "WP-1", -1200.0, 0.0, 0.0},
            {2, "WP-2", -900.0, 600.0, 0.0},
            {3, "WP-3", -900.0, -600.0, 0.0},
            {4, "WP-4", -300.0, 700.0, 0.0},
            {5, "WP-5", -300.0, -700.0, 0.0},
            {6, "WP-6", 300.0, 700.0, 0.0},
            {7, "WP-7", 300.0, -700.0, 0.0},
            {8, "WP-8", 900.0, 600.0, 0.0},
            {9, "WP-9", 900.0, -600.0, 0.0},
            {10, "WP-10", 1200.0, 0.0, 0.0},
            {11, "WP-11", 0.0, 0.0, 0.0},
            {12, "WP-12", 0.0, 800.0, 0.0},
            {13, "WP-13", 0.0, -800.0, 0.0},
        };
        healthPackProfileSpawnPoints = {
            {1, "HPACK-SPAWN-1", -1300.0, 700.0, 0.0},
            {2, "HPACK-SPAWN-2", 1300.0, 700.0, 0.0},
            {3, "HPACK-SPAWN-3", 0.0, -1100.0, 0.0},
            {4, "HPACK-SPAWN-4", -1300.0, -700.0, 0.0},
            {5, "HPACK-SPAWN-5", 1300.0, -700.0, 0.0},
            {6, "HPACK-SPAWN-6", 0.0, 1100.0, 0.0},
        };
        initialHealthPackSpawns = {
            {1, 0},
            {2, 2},
            {3, 4},
        };
    }
    else
    {
        arenaBounds = {-2500.0, 2500.0, -1800.0, 1800.0};
        botAreaBounds = {-1800.0, 1800.0, -1200.0, 1200.0};
        playerSpawnPoints = {
            {1, "P1", -1600.0, 0.0, 0.0},
            {2, "P2", 1600.0, 0.0, 0.0},
            {3, "P3", 0.0, 1000.0, 0.0},
            {4, "P4", 0.0, -1000.0, 0.0},
            {5, "P5", 0.0, 0.0, 0.0},
        };
        botSpawnPoints = {
            {1, "BOT-1", -900.0, 500.0, 0.0},
            {2, "BOT-2", -900.0, -500.0, 0.0},
            {3, "BOT-3", -300.0, 700.0, 0.0},
            {4, "BOT-4", -300.0, -700.0, 0.0},
            {5, "BOT-5", 300.0, 700.0, 0.0},
            {6, "BOT-6", 300.0, -700.0, 0.0},
            {7, "BOT-7", 900.0, 500.0, 0.0},
            {8, "BOT-8", 900.0, -500.0, 0.0},
        };
        botWanderPoints = {
            {1, "WP-1", -1200.0, 0.0, 0.0},
            {2, "WP-2", -900.0, 600.0, 0.0},
            {3, "WP-3", -900.0, -600.0, 0.0},
            {4, "WP-4", -300.0, 800.0, 0.0},
            {5, "WP-5", -300.0, -800.0, 0.0},
            {6, "WP-6", 300.0, 800.0, 0.0},
            {7, "WP-7", 300.0, -800.0, 0.0},
            {8, "WP-8", 900.0, 600.0, 0.0},
            {9, "WP-9", 900.0, -600.0, 0.0},
            {10, "WP-10", 1200.0, 0.0, 0.0},
            {11, "WP-11", 0.0, 0.0, 0.0},
        };
        healthPackProfileSpawnPoints = {
            {1, "HPACK-SPAWN-1", -1200.0, 0.0, 0.0},
            {2, "HPACK-SPAWN-2", 1200.0, 0.0, 0.0},
            {3, "HPACK-SPAWN-3", 0.0, -900.0, 0.0},
            {4, "HPACK-SPAWN-4", 0.0, 900.0, 0.0},
        };
        initialHealthPackSpawns = {
            {1, 0},
            {2, 1},
            {3, 2},
        };
    }

    healthPackSpawnPoints.clear();
    targetsInitialized = false;
    botsInitialized = false;
    healthPacksInitialized = false;
}

void GameRoom::ApplyRuntimeMapMarkersUnlocked(
    const std::string& profileName,
    const std::vector<ArenaPoint>& sharedSpawns,
    const std::vector<ArenaPoint>& healSpawns
)
{
    mapProfile = profileName.empty() ? "level_runtime" : profileName;
    bHasRuntimeMapMarkers = true;
    runtimeSharedSpawnPoints = sharedSpawns;
    runtimeHealSpawnPoints = healSpawns;

    if (!runtimeSharedSpawnPoints.empty())
    {
        double minX = runtimeSharedSpawnPoints.front().x;
        double maxX = runtimeSharedSpawnPoints.front().x;
        double minY = runtimeSharedSpawnPoints.front().y;
        double maxY = runtimeSharedSpawnPoints.front().y;

        for (const ArenaPoint& spawnPoint : runtimeSharedSpawnPoints)
        {
            minX = std::min(minX, spawnPoint.x);
            maxX = std::max(maxX, spawnPoint.x);
            minY = std::min(minY, spawnPoint.y);
            maxY = std::max(maxY, spawnPoint.y);

            if (spawnPoint.z < MinAcceptedClientZ)
            {
                std::ostringstream warningMessage;
                warningMessage
                    << "[BattleGridServer] Warning: spawn marker z low label="
                    << spawnPoint.label
                    << " z=" << spawnPoint.z;
                Logger::Warn(warningMessage.str());
            }
        }

        constexpr double ArenaMarkerMargin = 1000.0;
        constexpr double BotAreaMarkerMargin = 500.0;
        arenaBounds = {
            minX - ArenaMarkerMargin,
            maxX + ArenaMarkerMargin,
            minY - ArenaMarkerMargin,
            maxY + ArenaMarkerMargin
        };
        botAreaBounds = {
            minX - BotAreaMarkerMargin,
            maxX + BotAreaMarkerMargin,
            minY - BotAreaMarkerMargin,
            maxY + BotAreaMarkerMargin
        };
    }

    playerSpawnPoints = runtimeSharedSpawnPoints;
    botSpawnPoints = runtimeSharedSpawnPoints;
    healthPackProfileSpawnPoints = runtimeHealSpawnPoints;
    initialHealthPackSpawns.clear();
    for (std::size_t index = 0; index < healthPackProfileSpawnPoints.size(); ++index)
    {
        initialHealthPackSpawns.push_back({
            static_cast<std::uint64_t>(index + 1),
            index
        });
    }

    healthPackSpawnPoints.clear();
    targetsInitialized = false;
    botsInitialized = false;
    healthPacksInitialized = false;

    std::ostringstream logMessage;
    logMessage
        << "[BattleGridServer] Runtime map markers applied shared="
        << runtimeSharedSpawnPoints.size()
        << " heal=" << runtimeHealSpawnPoints.size()
        << " profile=" << mapProfile
        << " coordinate_mode=identity"
        << " arena x=" << arenaBounds.minX << ".." << arenaBounds.maxX
        << " y=" << arenaBounds.minY << ".." << arenaBounds.maxY
        << " bot_area x=" << botAreaBounds.minX << ".." << botAreaBounds.maxX
        << " y=" << botAreaBounds.minY << ".." << botAreaBounds.maxY;
    Logger::Info(logMessage.str());
}

void GameRoom::LogMapProfile() const
{
    std::ostringstream mapLog;
    mapLog
        << "[BattleGridServer] Map profile=" << mapProfile
        << " arena x=" << arenaBounds.minX << ".." << arenaBounds.maxX
        << " y=" << arenaBounds.minY << ".." << arenaBounds.maxY
        << " bot_area x=" << botAreaBounds.minX << ".." << botAreaBounds.maxX
        << " y=" << botAreaBounds.minY << ".." << botAreaBounds.maxY;
    Logger::Info(mapLog.str());
}

GameRoom::GameRoom(std::uint64_t inRoomId)
    : roomId(inRoomId),
      roomName("Default Room"),
      hostPlayerId(0),
      roomFlowState("in_game"),
      maxPlayers(6),
      matchEndingReturnSeconds(5.0),
      matchEndingTimerSeconds(0.0),
      desiredBotCount(RuntimeMarkerBotCount),
      players(),
      matchState(),
      bots(),
      healthPacks(),
      projectiles(),
      targets(),
      healthPackSpawnPoints(),
      mapProfile("demo_arena_v1"),
      arenaBounds(),
      botAreaBounds(),
      playerSpawnPoints(),
      targetCorePositions(),
      botSpawnPoints(),
      botWanderPoints(),
      healthPackProfileSpawnPoints(),
      initialHealthPackSpawns(),
      runtimeSharedSpawnPoints(),
      runtimeHealSpawnPoints(),
      bHasRuntimeMapMarkers(false),
      nextProjectileId(1),
      healthPackRespawnCounter(0),
      recentEvents(),
      nextEventId(1),
      MaxRecentEvents(20),
      serverTimeSeconds(0.0),
      lastBotAttackTickLogTime(-1000.0),
      bBotAttacksEnabled(true),
      botDifficulty("normal"),
      currentDemoPreset("default"),
      bAutoEndMatchByTimer(true),
      bTargetsEnabled(false),
      botDetectRange(1600.0),
      botAttackRange(1300.0),
      botAttackDamage(BotBodyDamage),
      botHeadshotDamage(BotHeadshotDamage),
      botAttackCooldownSeconds(0.5),
      botMoveSpeed(450.0),
      botFireIntervalSeconds(0.5),
      botAimSpreadDegrees(13.0),
      botFireChance(0.65),
      botWanderRadius(500.0),
      botWanderStepMin(160.0),
      botWanderStepMax(500.0),
      botWanderWaitMin(0.4),
      botWanderWaitMax(1.2),
      BotWanderRepathSeconds(1.0),
      bUseClientHitClaimsForBots(true),
      bUseClientHitClaimsForPlayers(true),
      bVerboseBotShotEvents(false),
      bVerboseInputLogs(false),
      bEnableBot2DFallbackHit(true),
      Bot2DFallbackRadiusScale(0.75),
      botBodyHitboxScale(DefaultBotBodyHitboxScale),
      botHeadHitboxScale(DefaultBotHeadHitboxScale),
      playerBodyHitboxScale(DefaultPlayerBodyHitboxScale),
      playerHeadshotHitboxScale(DefaultPlayerHeadshotHitboxScale),
      bVerboseHitscanCandidateLogs(false),
      bVerboseBotStateLogs(false),
      bUseClientFireOriginForHitscan(true),
      ClientFireOriginWarningDistance(300.0),
      MaxAcceptedClientFireOriginDistance(2000.0),
      bUseClientPositionForPlayerMovement(true),
      MaxClientPositionDeltaPerSecond(1400.0),
      MaxClientPositionSnapDistance(3000.0),
      MinAcceptedClientZ(-2000.0),
      MaxAcceptedClientZ(5000.0),
      MaxClientZDeltaPerSecond(2000.0),
      bRejectExtremeClientZ(true),
      MaxBotsTargetingOnePlayer(8),
      MaxBotsShootingOnePlayer(3),
      BotTargetReconsiderSeconds(1.0),
      BotShotRandomDelayMin(0.1),
      BotShotRandomDelayMax(0.35),
      BotRecentDamageGraceSeconds(0.15),
      respawnInvincibleSeconds(2.0),
      targetsInitialized(false),
      botsInitialized(false),
      healthPacksInitialized(false),
      spawnRandom(1337),
      mutex()
{
    ApplyMapProfileUnlocked(mapProfile);
    LogMapProfile();

    std::ostringstream zLog;
    zLog
        << "[BattleGridServer] Player z bounds " << MinPlayerZ << ".." << MaxPlayerZ
        << " accepted_client_z=" << MinAcceptedClientZ << ".." << MaxAcceptedClientZ;
    Logger::Info(zLog.str());

    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();
    AddCombatEvent("match_started", "Match started");
}

std::uint64_t GameRoom::GetRoomId() const
{
    return roomId;
}

std::string GameRoom::GetRoomName() const
{
    std::lock_guard lock(mutex);
    return roomName;
}

std::string GameRoom::GetRoomFlowState() const
{
    std::lock_guard lock(mutex);
    return roomFlowState;
}

std::uint64_t GameRoom::GetHostPlayerId() const
{
    std::lock_guard lock(mutex);
    return hostPlayerId;
}

std::size_t GameRoom::GetMaxPlayers() const
{
    std::lock_guard lock(mutex);
    return maxPlayers;
}

void GameRoom::ConfigureLobbyRoom(
    const std::string& inRoomName,
    std::uint64_t inHostPlayerId,
    std::size_t inMaxPlayers
)
{
    std::lock_guard lock(mutex);
    roomName = inRoomName.empty() ? "BattleGrid room" : inRoomName;
    hostPlayerId = inHostPlayerId;
    maxPlayers = std::max<std::size_t>(1, inMaxPlayers);
    roomFlowState = "waiting";
    matchEndingTimerSeconds = 0.0;
    desiredBotCount = RuntimeMarkerBotCount;
    for (auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);
        player.ready = false;
    }
}

bool GameRoom::AddPlayer(std::uint64_t playerId, const std::string& nickname)
{
    std::lock_guard lock(mutex);
    if (players.find(playerId) == players.end() && players.size() >= maxPlayers)
    {
        return false;
    }

    const auto [iterator, inserted] = players.emplace(
        playerId,
        PlayerState(playerId, roomId, nickname)
    );

    if (!inserted)
    {
        iterator->second.nickname = nickname;
        iterator->second.connected = true;
        iterator->second.ready = false;
        return false;
    }

    if (hostPlayerId == 0)
    {
        hostPlayerId = playerId;
    }

    PlayerState& player = iterator->second;
    player.ready = false;
    player.hp = player.maxHp;
    player.alive = true;
    player.invincible = false;
    player.respawnTimerSeconds = 0.0;
    player.invincibleTimerSeconds = 0.0;
    player.botRecentDamageGraceTimerSeconds = 0.0;
    AssignRespawnPosition(player);

    return true;
}

bool GameRoom::RemovePlayer(std::uint64_t playerId)
{
    std::lock_guard lock(mutex);
    const bool bRemoved = players.erase(playerId) > 0;
    if (bRemoved && hostPlayerId == playerId)
    {
        hostPlayerId = players.empty() ? 0 : players.begin()->first;
        if (hostPlayerId != 0)
        {
            players[hostPlayerId].ready = false;
        }
    }
    return bRemoved;
}

bool GameRoom::HasPlayer(std::uint64_t playerId) const
{
    std::lock_guard lock(mutex);
    return players.find(playerId) != players.end();
}

bool GameRoom::CanJoinRoom() const
{
    std::lock_guard lock(mutex);
    return roomFlowState == "waiting" && players.size() < maxPlayers;
}

bool GameRoom::IsEmpty() const
{
    std::lock_guard lock(mutex);
    return players.empty();
}

bool GameRoom::SetReady(std::uint64_t playerId, bool bReady)
{
    std::lock_guard lock(mutex);
    const auto player = players.find(playerId);
    if (player == players.end())
    {
        return false;
    }

    player->second.ready = playerId == hostPlayerId ? false : bReady;
    return true;
}

bool GameRoom::StartMatchFromLobby(
    std::uint64_t requestingPlayerId,
    std::vector<std::string>& outNotReadyPlayers,
    std::string& outReason
)
{
    std::lock_guard lock(mutex);
    outNotReadyPlayers.clear();
    outReason.clear();

    if (requestingPlayerId != hostPlayerId)
    {
        outReason = "not_host";
        return false;
    }
    if (roomFlowState != "waiting")
    {
        outReason = "not_waiting";
        return false;
    }
    if (players.empty())
    {
        outReason = "not_enough_players";
        return false;
    }

    for (const auto& [playerId, player] : players)
    {
        if (playerId != hostPlayerId && !player.ready)
        {
            outNotReadyPlayers.push_back(player.nickname.empty()
                ? ("P" + std::to_string(playerId))
                : player.nickname);
        }
    }
    if (!outNotReadyPlayers.empty())
    {
        outReason = "not_ready";
        return false;
    }

    const std::size_t humanCount = players.size();
    if (humanCount <= 2)
    {
        desiredBotCount = 8;
    }
    else if (humanCount <= 4)
    {
        desiredBotCount = 6;
    }
    else
    {
        desiredBotCount = 4;
    }

    roomFlowState = "in_game";
    matchEndingTimerSeconds = 0.0;
    bBotAttacksEnabled = true;
    bAutoEndMatchByTimer = true;
    bTargetsEnabled = false;
    ApplyBotDifficultyUnlocked("easy");
    const std::uint64_t matchId = ResetRoomForDemoUnlocked(
        "match_started",
        "Match started",
        false,
        false
    );
    matchState.matchId = matchId;
    matchState.state = "in_progress";
    matchState.matchDurationSeconds = 600.0;
    matchState.timeRemainingSeconds = 600.0;
    matchState.targetScore = 30;
    matchState.gameOver = false;
    matchState.matchEndedEventEmitted = false;

    for (auto& [playerId, player] : players)
    {
        static_cast<void>(playerId);
        player.ready = false;
        player.scoreReachedTimeSeconds = 0.0;
    }

    Logger::Info(
        "[BattleGridServer] Match started room_id=" + std::to_string(roomId)
        + " players=" + std::to_string(players.size())
        + " bots=" + std::to_string(desiredBotCount)
    );
    return true;
}

nlohmann::json GameRoom::BuildRoomListEntryJson() const
{
    std::lock_guard lock(mutex);
    const auto host = players.find(hostPlayerId);
    nlohmann::json json;
    json["room_id"] = roomId;
    json["room_name"] = roomName;
    json["host_player_id"] = hostPlayerId;
    json["host_nickname"] =
        host != players.end() ? host->second.nickname : std::string();
    json["player_count"] = players.size();
    json["max_players"] = maxPlayers;
    json["state"] = roomFlowState;
    json["can_join"] = roomFlowState == "waiting" && players.size() < maxPlayers;
    return json;
}

nlohmann::json GameRoom::BuildRoomStateJson() const
{
    std::lock_guard lock(mutex);
    nlohmann::json json;
    json["type"] = "room_state";
    json["room_id"] = roomId;
    json["room_name"] = roomName;
    json["state"] = roomFlowState;
    json["host_player_id"] = hostPlayerId;
    json["max_players"] = maxPlayers;
    json["players"] = nlohmann::json::array();
    for (const auto& [playerId, player] : players)
    {
        nlohmann::json playerJson;
        playerJson["player_id"] = playerId;
        playerJson["nickname"] = player.nickname;
        playerJson["is_host"] = playerId == hostPlayerId;
        playerJson["ready"] = playerId == hostPlayerId ? false : player.ready;
        playerJson["connected"] = player.connected;
        json["players"].push_back(std::move(playerJson));
    }
    return json;
}

bool GameRoom::UpdateInput(std::uint64_t playerId, const PlayerInput& input)
{
    std::lock_guard lock(mutex);
    const auto player = players.find(playerId);
    if (player == players.end())
    {
        return false;
    }

    PlayerState& playerState = player->second;
    playerState.latestInput = input;
    if (
        (mapProfile == "level_runtime" || bHasRuntimeMapMarkers)
        && input.hasClientWorldPosition
        && std::isfinite(input.clientWorldX)
        && std::isfinite(input.clientWorldY)
    )
    {
        playerState.x = ClampArenaX(input.clientWorldX);
        playerState.y = ClampArenaY(input.clientWorldY);
        if (
            std::isfinite(input.clientWorldZ)
            && (
                !bRejectExtremeClientZ
                || (
                    input.clientWorldZ >= MinAcceptedClientZ
                    && input.clientWorldZ <= MaxAcceptedClientZ
                )
            )
        )
        {
            playerState.z = input.clientWorldZ;
        }
        if (input.hasClientYaw && std::isfinite(input.clientYaw))
        {
            playerState.yaw = input.clientYaw;
        }

        if (bVerboseInputLogs)
        {
            std::ostringstream syncLog;
            syncLog
                << "[BattleGridServer] ClientPositionSync applied player="
                << playerState.playerId
                << " pos=(" << playerState.x << "," << playerState.y << "," << playerState.z << ")"
                << " yaw=" << playerState.yaw
                << " source=input_handler";
            Logger::Info(syncLog.str());
        }
    }
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
    serverTimeSeconds += std::max(0.0, deltaSeconds);

    if (roomFlowState == "waiting")
    {
        return;
    }

    if (roomFlowState == "match_ending")
    {
        matchEndingTimerSeconds = std::max(0.0, matchEndingTimerSeconds - deltaSeconds);
        if (matchEndingTimerSeconds <= 0.0)
        {
            roomFlowState = "waiting";
            matchState.state = "waiting";
            matchState.gameOver = false;
            matchState.timeRemainingSeconds = matchState.matchDurationSeconds;
            projectiles.clear();
            for (auto& [playerId, player] : players)
            {
                static_cast<void>(playerId);
                player.ready = false;
                player.latestInput = PlayerInput();
            }
            Logger::Info(
                "[BattleGrid] Room returned to waiting room_id="
                + std::to_string(roomId)
                + " ready_reset=true"
            );
            AddCombatEvent("room_returned", "Returned to room");
        }
        return;
    }

    if (bTargetsEnabled)
    {
        InitializeDefaultTargets();
    }
    InitializeDefaultBots();
    InitializeDefaultHealthPacks();

    if (matchState.IsGameOver())
    {
        if (roomFlowState == "in_game")
        {
            roomFlowState = "match_ending";
            matchEndingTimerSeconds = matchEndingReturnSeconds;
        }
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

        const PlayerInput& input = player.latestInput;
        double effectiveSpeed = WalkSpeed;
        if (input.sprint)
        {
            effectiveSpeed = SprintSpeed;
        }
        else if (input.ads)
        {
            effectiveSpeed = AdsWalkSpeed;
        }
        player.speed = effectiveSpeed;

        bool bUsedClientPosition = false;
        std::string movementSource = "input";
        const bool bLevelRuntimeClientWorldPosition =
            (mapProfile == "level_runtime" || bHasRuntimeMapMarkers)
            && input.hasClientWorldPosition;
        const bool bLegacyClientPosition =
            bUseClientPositionForPlayerMovement && input.hasClientPosition;
        if (bLevelRuntimeClientWorldPosition || bLegacyClientPosition)
        {
            const double sourceX = bLevelRuntimeClientWorldPosition
                ? input.clientWorldX
                : input.clientX;
            const double sourceY = bLevelRuntimeClientWorldPosition
                ? input.clientWorldY
                : input.clientY;
            const double sourceZ = bLevelRuntimeClientWorldPosition
                ? input.clientWorldZ
                : input.clientZ;
            if (
                std::isfinite(sourceX)
                && std::isfinite(sourceY)
            )
            {
                const double clientX = ClampArenaX(sourceX);
                const double clientY = ClampArenaY(sourceY);
                const double deltaX = clientX - player.x;
                const double deltaY = clientY - player.y;
                const double distance = std::sqrt((deltaX * deltaX) + (deltaY * deltaY));
                const double acceptedStep =
                    (MaxClientPositionDeltaPerSecond * std::max(0.0, deltaSeconds)) + 50.0;
                const bool bAllowLargeDemoSnap = bLevelRuntimeClientWorldPosition;

                if (
                    bAllowLargeDemoSnap
                    || distance <= acceptedStep
                    || distance <= MaxClientPositionSnapDistance
                )
                {
                    double clientZ = player.z;
                    bool bAcceptedClientZ = false;
                    if (!std::isfinite(sourceZ))
                    {
                        if (tickNumber % 30 == 0)
                        {
                            Logger::Warn(
                                "[BattleGridServer] Rejected client Z player="
                                + std::to_string(player.playerId)
                                + " z=nan"
                            );
                        }
                    }
                    else if (
                        bRejectExtremeClientZ
                        && (sourceZ < MinAcceptedClientZ || sourceZ > MaxAcceptedClientZ)
                    )
                    {
                        if (tickNumber % 30 == 0)
                        {
                            std::ostringstream rejectZLog;
                            rejectZLog
                                << "[BattleGridServer] Rejected client Z player="
                                << player.playerId
                                << " z=" << sourceZ;
                            Logger::Warn(rejectZLog.str());
                        }
                    }
                    else
                    {
                        const double deltaZ = std::abs(sourceZ - player.z);
                        const double acceptedZStep =
                            (MaxClientZDeltaPerSecond * std::max(0.0, deltaSeconds)) + 100.0;
                        if (bAllowLargeDemoSnap || deltaZ <= acceptedZStep || distance > acceptedStep)
                        {
                            clientZ = sourceZ;
                            bAcceptedClientZ = true;
                        }
                        else if (tickNumber % 30 == 0)
                        {
                            std::ostringstream rejectZLog;
                            rejectZLog
                                << "[BattleGridServer] Rejected client Z player="
                                << player.playerId
                                << " z=" << sourceZ
                                << " dz=" << deltaZ
                                << " max_step=" << acceptedZStep;
                            Logger::Warn(rejectZLog.str());
                        }
                    }

                    if (bVerboseInputLogs && distance > acceptedStep && tickNumber % 30 == 0)
                    {
                        std::ostringstream snapLog;
                        snapLog
                            << "[BattleGridServer] Accepted demo client position snap player="
                            << player.playerId
                            << " dist=" << distance;
                        Logger::Info(snapLog.str());
                    }

                    player.x = clientX;
                    player.y = clientY;
                    player.z = clientZ;
                    if (input.hasClientYaw && std::isfinite(input.clientYaw))
                    {
                        player.yaw = input.clientYaw;
                    }
                    bUsedClientPosition = true;
                    movementSource = bLevelRuntimeClientWorldPosition
                        ? "client_world_position"
                        : (distance > acceptedStep
                        ? "client_snap"
                        : "client_position");

                    if (
                        bAcceptedClientZ
                        && (sourceZ < MinPlayerZ || sourceZ > MaxPlayerZ)
                        && tickNumber % 30 == 0
                    )
                    {
                        std::ostringstream zClampLog;
                        zClampLog
                            << "[BattleGridServer] Accepted out-of-legacy-range client z player="
                            << player.playerId
                            << " input_z=" << sourceZ
                            << " applied_z=" << clientZ;
                        Logger::Warn(zClampLog.str());
                    }

                    if (bVerboseInputLogs && tickNumber % 30 == 0)
                    {
                        std::ostringstream syncLog;
                        syncLog
                            << "[BattleGridServer] ClientPositionSync applied player="
                            << player.playerId
                            << " pos=(" << player.x << "," << player.y << "," << player.z << ")"
                            << " yaw=" << player.yaw
                            << " source=" << movementSource;
                        Logger::Info(syncLog.str());
                    }
                }
                else if (tickNumber % 30 == 0)
                {
                    std::ostringstream rejectLog;
                    rejectLog
                        << "[BattleGridServer] Rejected client position player="
                        << player.playerId
                        << " dist=" << distance
                        << " max_snap=" << MaxClientPositionSnapDistance;
                    Logger::Warn(rejectLog.str());
                }
            }
            else if (tickNumber % 30 == 0)
            {
                Logger::Warn(
                    "[BattleGridServer] Rejected invalid client/world position player="
                    + std::to_string(player.playerId)
                );
            }
        }

        double moveX = input.moveX;
        double moveY = input.moveY;
        const double length = std::sqrt((moveX * moveX) + (moveY * moveY));
        if (length > 1.0)
        {
            moveX /= length;
            moveY /= length;
        }

        if (!bUsedClientPosition)
        {
            player.x += moveX * effectiveSpeed * deltaSeconds;
            player.y += moveY * effectiveSpeed * deltaSeconds;
            player.x = ClampArenaX(player.x);
            player.y = ClampArenaY(player.y);
        }

        if (
            input.fire
            && input.seq != player.lastProcessedFireSeq
        )
        {
            double projectileDirX = input.shotDirX;
            double projectileDirY = input.shotDirY;
            double projectileDirZ = input.shotDirZ;
            const double shotDirectionLengthSquared =
                (projectileDirX * projectileDirX)
                + (projectileDirY * projectileDirY)
                + (projectileDirZ * projectileDirZ);
            if (shotDirectionLengthSquared <= 0.0001)
            {
                projectileDirX = input.aimX;
                projectileDirY = input.aimY;
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
                ProcessHitscanFire(player, input);
            }
            player.lastProcessedFireSeq = input.seq;
        }

        if (bVerboseInputLogs && tickNumber % 60 == 0)
        {
            std::ostringstream logMessage;
            logMessage
                << "Player position player_id=" << player.playerId
                << " x=" << player.x
                << " y=" << player.y
                << " move_x=" << moveX
                << " move_y=" << moveY
                << " speed=" << effectiveSpeed
                << " source=" << movementSource
                << " sprint=" << (input.sprint ? "true" : "false")
                << " ads=" << (input.ads ? "true" : "false")
                << " has_client_position=" << (input.hasClientPosition ? "true" : "false");
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
    json["map_profile"] = mapProfile;
    json["coordinate_mode"] =
        mapProfile == "level_runtime" ? "identity" : "calibrated";
    json["current_demo_preset"] = currentDemoPreset;
    json["arena"] = BuildArenaLayoutJson(true, bTargetsEnabled);
    json["bot_area_bounds"] = BuildBotAreaBoundsJson();
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
    json["arena_bounds"] = BuildArenaBoundsJson();
    json["bot_area_bounds"] = BuildBotAreaBoundsJson();
    json["player_spawns"] = BuildArenaPointsJson(playerSpawnPoints, "spawn_id");
    json["bot_spawns"] = BuildArenaPointsJson(botSpawnPoints, "bot_id");
    json["health_pack_spawns"] =
        BuildArenaPointsJson(healthPackProfileSpawnPoints, "spawn_id");
    json["map_profile"] = mapProfile;
    json["coordinate_mode"] =
        mapProfile == "level_runtime" ? "identity" : "calibrated";
    json["has_runtime_map_markers"] = bHasRuntimeMapMarkers;
    json["shared_spawn_count"] = runtimeSharedSpawnPoints.size();
    json["heal_spawn_count"] = runtimeHealSpawnPoints.size();
    json["shared_spawns"] = BuildArenaPointsJson(runtimeSharedSpawnPoints, "spawn_id");
    json["heal_spawns"] = BuildArenaPointsJson(runtimeHealSpawnPoints, "spawn_id");
    json["bot_waypoints"] = BuildArenaPointsJson(botWanderPoints, "waypoint_id");
    json["bot_waypoint_count"] = botWanderPoints.size();
    json["match"] = matchState.ToJson();
    json["current_demo_preset"] = currentDemoPreset;
    json["scoreboard"] = BuildScoreboardJson();
    json["events"] = BuildEventsJson();
    json["player_count"] = players.size();
    json["projectile_count"] = projectiles.size();
    json["targets_enabled"] = bTargetsEnabled;
    json["targets_debug_count"] = targets.size();
    json["bot_count"] = bots.size();
    std::size_t activeBotShooters = 0;
    for (const auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);
        if (bot.allowedToShoot || bot.combatState == "shoot")
        {
            ++activeBotShooters;
        }
    }
    json["active_bot_shooters"] = activeBotShooters;
    json["bot_attacks_enabled"] = bBotAttacksEnabled;
    json["bot_difficulty"] = botDifficulty;
    json["bot_attack_damage"] = botAttackDamage;
    json["bot_headshot_damage"] = botHeadshotDamage;
    json["bot_attack_cooldown"] = botAttackCooldownSeconds;
    json["bot_config"] = {
        {"damage", botAttackDamage},
        {"head", botHeadshotDamage},
        {"cooldown", botFireIntervalSeconds},
    };
    json["bot_body_hitbox_scale"] = botBodyHitboxScale;
    json["bot_head_hitbox_scale"] = botHeadHitboxScale;
    json["player_body_hitbox_scale"] = playerBodyHitboxScale;
    json["player_headshot_hitbox_scale"] = playerHeadshotHitboxScale;
    json["head_hitbox_scale_bot"] = botHeadHitboxScale;
    json["head_hitbox_scale_player"] = playerHeadshotHitboxScale;
    json["hitbox_config"] = {
        {"bot_body_scale", botBodyHitboxScale},
        {"bot_head_scale", botHeadHitboxScale},
        {"player_body_scale", playerBodyHitboxScale},
        {"player_head_scale", playerHeadshotHitboxScale},
    };
    const BotState debugBotDefaults;
    const PlayerState debugPlayerDefaults;
    json["bot_hitbox"] = {
        {"body_radius", debugBotDefaults.bodyRadius * botBodyHitboxScale},
        {"head_radius", debugBotDefaults.headRadius * botHeadHitboxScale},
        {"base_body_radius", debugBotDefaults.bodyRadius},
        {"base_head_radius", debugBotDefaults.headRadius},
    };
    json["player_hitbox"] = {
        {"body_radius", debugPlayerDefaults.bodyRadius * playerBodyHitboxScale},
        {"head_radius", debugPlayerDefaults.headRadius * playerHeadshotHitboxScale},
        {"base_body_radius", debugPlayerDefaults.bodyRadius},
        {"base_head_radius", debugPlayerDefaults.headRadius},
    };
    json["bot_detect_range"] = botDetectRange;
    json["bot_attack_range"] = botAttackRange;
    json["bot_move_speed"] = botMoveSpeed;
    json["bot_fire_interval"] = botFireIntervalSeconds;
    json["bot_aim_spread"] = botAimSpreadDegrees;
    json["bot_fire_chance"] = botFireChance;
    json["max_bots_targeting_one_player"] = MaxBotsTargetingOnePlayer;
    json["max_bots_shooting_one_player"] = MaxBotsShootingOnePlayer;
    json["respawn_invincible_seconds"] = respawnInvincibleSeconds;
    json["bot_target_reconsider_seconds"] = BotTargetReconsiderSeconds;
    json["bot_wander_radius"] = botWanderRadius;
    json["bot_wander_step_min"] = botWanderStepMin;
    json["bot_wander_step_max"] = botWanderStepMax;
    json["bot_wander_wait_min"] = botWanderWaitMin;
    json["bot_wander_wait_max"] = botWanderWaitMax;
    json["bot_repath_interval"] = BotWanderRepathSeconds;
    json["use_client_hit_claims_for_bots"] = bUseClientHitClaimsForBots;
    json["use_client_hit_claims_for_players"] = bUseClientHitClaimsForPlayers;
    json["bot_shot_random_delay_min"] = BotShotRandomDelayMin;
    json["bot_shot_random_delay_max"] = BotShotRandomDelayMax;
    json["bot_recent_damage_grace_seconds"] = BotRecentDamageGraceSeconds;
    json["bot_shot_events_verbose"] = bVerboseBotShotEvents;
    json["verbose_input_logs"] = bVerboseInputLogs;
    json["enable_bot_2d_fallback_hit"] = bEnableBot2DFallbackHit;
    json["bot_2d_fallback_radius_scale"] = Bot2DFallbackRadiusScale;
    json["verbose_hitscan_candidate_logs"] = bVerboseHitscanCandidateLogs;
    json["verbose_bot_state_logs"] = bVerboseBotStateLogs;
    json["use_client_fire_origin_for_hitscan"] = bUseClientFireOriginForHitscan;
    json["client_fire_origin_warning_distance"] = ClientFireOriginWarningDistance;
    json["max_accepted_client_fire_origin_distance"] = MaxAcceptedClientFireOriginDistance;
    json["use_client_position_for_player_movement"] = bUseClientPositionForPlayerMovement;
    json["max_client_position_delta_per_second"] = MaxClientPositionDeltaPerSecond;
    json["max_client_position_snap_distance"] = MaxClientPositionSnapDistance;
    json["min_accepted_client_z"] = MinAcceptedClientZ;
    json["max_accepted_client_z"] = MaxAcceptedClientZ;
    json["max_client_z_delta_per_second"] = MaxClientZDeltaPerSecond;
    json["reject_extreme_client_z"] = bRejectExtremeClientZ;
    json["min_player_z"] = MinPlayerZ;
    json["max_player_z"] = MaxPlayerZ;
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
    json["legacy_target_kill_score"] = TargetKillScore;
    json["walk_speed"] = WalkSpeed;
    json["sprint_speed"] = SprintSpeed;
    json["ads_walk_speed"] = AdsWalkSpeed;
    json["health_pack_heal_amount"] = HealthPackHealAmount;
    json["health_pack_pickup_radius"] = HealthPackPickupRadius;
    json["health_pack_respawn_seconds"] = HealthPackRespawnSeconds;
    json["max_active_health_packs"] = bHasRuntimeMapMarkers
        ? initialHealthPackSpawns.size()
        : static_cast<std::size_t>(MaxActiveHealthPacks);
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
        inputJson["has_fire_origin"] = input.hasFireOrigin;
        inputJson["fire_origin_x"] = input.fireOriginX;
        inputJson["fire_origin_y"] = input.fireOriginY;
        inputJson["fire_origin_z"] = input.fireOriginZ;
        inputJson["has_client_position"] = input.hasClientPosition;
        inputJson["client_x"] = input.clientX;
        inputJson["client_y"] = input.clientY;
        inputJson["client_z"] = input.clientZ;
        inputJson["has_client_world_position"] = input.hasClientWorldPosition;
        inputJson["client_world_x"] = input.clientWorldX;
        inputJson["client_world_y"] = input.clientWorldY;
        inputJson["client_world_z"] = input.clientWorldZ;
        inputJson["has_client_yaw"] = input.hasClientYaw;
        inputJson["client_yaw"] = input.clientYaw;

        double effectiveSpeed = WalkSpeed;
        if (input.sprint)
        {
            effectiveSpeed = SprintSpeed;
        }
        else if (input.ads)
        {
            effectiveSpeed = AdsWalkSpeed;
        }

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
        playerJson["yaw"] = player.yaw;
        playerJson["speed"] = player.speed;
        playerJson["effective_speed"] = effectiveSpeed;
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
        playerJson["bot_recent_damage_grace_timer"] =
            player.botRecentDamageGraceTimerSeconds;
        playerJson["latest_input"] = std::move(inputJson);

        json["players"].push_back(std::move(playerJson));
    }

    for (const auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);
        json["bots"].push_back(BuildBotDebugJson(
            bot,
            botDetectRange,
            botAttackRange,
            botBodyHitboxScale,
            botHeadHitboxScale
        ));
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

std::uint64_t GameRoom::ResetRoomForDemoUnlocked(
    const std::string& primaryEventType,
    const std::string& primaryEventMessage,
    bool bAddMatchRestartedEvent,
    bool bAddMatchStartedEvent
)
{
    const std::uint64_t nextMatchId = matchState.matchId + 1;
    matchState.Reset();
    matchState.matchId = nextMatchId;
    serverTimeSeconds = 0.0;
    recentEvents.clear();
    nextEventId = 1;
    if (bHasRuntimeMapMarkers)
    {
        healthPackSpawnPoints.clear();
        targetsInitialized = false;
        botsInitialized = false;
        healthPacksInitialized = false;
    }
    else
    {
        ApplyMapProfileUnlocked(mapProfile);
    }

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
        player.botRecentDamageGraceTimerSeconds = 0.0;
        player.lastProcessedFireSeq = 0;
        AssignRespawnPosition(player);
    }

    if (bAddMatchRestartedEvent)
    {
        AddCombatEvent("match_restarted", "Match restarted");
    }
    if (!primaryEventType.empty())
    {
        AddCombatEvent(primaryEventType, primaryEventMessage);
    }
    if (bAddMatchStartedEvent)
    {
        AddCombatEvent("match_started", "Match started");
    }

    return matchState.matchId;
}

std::uint64_t GameRoom::ResetMatch()
{
    std::lock_guard lock(mutex);
    const std::uint64_t matchId = ResetRoomForDemoUnlocked(
        "",
        "",
        true,
        true
    );
    Logger::Info("Match restarted match_id=" + std::to_string(matchId));
    return matchId;
}

std::uint64_t GameRoom::ApplySafeDemoMode()
{
    std::lock_guard lock(mutex);
    bBotAttacksEnabled = bSafeDemoBotAttacksEnabled;
    bAutoEndMatchByTimer = bSafeDemoAutoEndMatchByTimer;
    bTargetsEnabled = false;
    ApplyBotDifficultyUnlocked(SafeDemoDifficulty);
    currentDemoPreset = "safe_visual";

    const std::uint64_t matchId = ResetRoomForDemoUnlocked(
        "demo_mode_applied",
        "Safe demo mode applied",
        false,
        true
    );
    Logger::Info("Safe demo mode applied.");
    return matchId;
}

bool GameRoom::ApplyDemoPreset(const std::string& presetName)
{
    std::lock_guard lock(mutex);

    std::string eventMessage;
    if (presetName == "safe_visual")
    {
        bBotAttacksEnabled = true;
        bAutoEndMatchByTimer = false;
        bTargetsEnabled = false;
        ApplyBotDifficultyUnlocked("easy");
        eventMessage = "Safe visual demo preset applied";
    }
    else if (presetName == "combat_demo")
    {
        bBotAttacksEnabled = true;
        bAutoEndMatchByTimer = false;
        bTargetsEnabled = false;
        ApplyBotDifficultyUnlocked("easy");
        eventMessage = "Combat demo preset applied";
    }
    else if (presetName == "match_demo")
    {
        bBotAttacksEnabled = true;
        bAutoEndMatchByTimer = true;
        bTargetsEnabled = false;
        ApplyBotDifficultyUnlocked("normal");
        eventMessage = "Match demo preset applied";
    }
    else if (presetName == "debug_visual")
    {
        bBotAttacksEnabled = false;
        bAutoEndMatchByTimer = false;
        bTargetsEnabled = false;
        ApplyBotDifficultyUnlocked("easy");
        eventMessage = "Debug visual preset applied";
    }
    else
    {
        return false;
    }

    currentDemoPreset = presetName;
    const std::uint64_t matchId = ResetRoomForDemoUnlocked(
        "demo_preset_applied",
        eventMessage,
        false,
        true
    );

    Logger::Info(
        "Demo preset applied preset=" + presetName
        + " match_id=" + std::to_string(matchId)
    );
    return true;
}

void GameRoom::ApplyMapProfile(const std::string& profileName)
{
    std::lock_guard lock(mutex);
    ApplyMapProfileUnlocked(profileName);
    currentDemoPreset = "custom";
    const std::uint64_t matchId = ResetRoomForDemoUnlocked(
        "map_profile_applied",
        "Map profile applied: " + mapProfile,
        false,
        true
    );
    LogMapProfile();
    Logger::Info(
        "Map profile applied profile=" + mapProfile
        + " match_id=" + std::to_string(matchId)
    );
}

bool GameRoom::ApplyRuntimeMapMarkers(
    const std::string& profileName,
    const std::vector<ArenaPoint>& sharedSpawns,
    const std::vector<ArenaPoint>& healSpawns
)
{
    if (sharedSpawns.empty() || healSpawns.empty())
    {
        return false;
    }

    const auto IsValidPoint = [](const ArenaPoint& point)
    {
        return std::isfinite(point.x)
            && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (
        !std::all_of(sharedSpawns.begin(), sharedSpawns.end(), IsValidPoint)
        || !std::all_of(healSpawns.begin(), healSpawns.end(), IsValidPoint)
    )
    {
        return false;
    }

    std::lock_guard lock(mutex);
    ApplyRuntimeMapMarkersUnlocked(
        profileName.empty() ? std::string("level_runtime") : profileName,
        sharedSpawns,
        healSpawns
    );
    currentDemoPreset = "custom";
    LogMapProfile();
    return true;
}

void GameRoom::SetBotAttacksEnabled(bool bEnabled)
{
    std::lock_guard lock(mutex);
    bBotAttacksEnabled = bEnabled;
    currentDemoPreset = "custom";
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
    currentDemoPreset = "custom";
    Logger::Info("Bot difficulty set to " + botDifficulty);
}

void GameRoom::SetBotCombatConfig(int bodyDamage, int headDamage, double cooldownSeconds)
{
    std::lock_guard lock(mutex);

    if (bodyDamage > 0)
    {
        botAttackDamage = bodyDamage;
    }
    if (headDamage > 0)
    {
        botHeadshotDamage = headDamage;
    }
    if (cooldownSeconds > 0.0)
    {
        botAttackCooldownSeconds = cooldownSeconds;
        botFireIntervalSeconds = cooldownSeconds;
    }

    for (auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);
        bot.attackCooldownSeconds = botAttackCooldownSeconds;
        bot.attackTimerSeconds = std::min(bot.attackTimerSeconds, botAttackCooldownSeconds);
        bot.fireIntervalSeconds = botFireIntervalSeconds;
        bot.fireCooldownSeconds = std::min(bot.fireCooldownSeconds, bot.fireIntervalSeconds);
    }

    currentDemoPreset = "custom";
    std::ostringstream logMessage;
    logMessage
        << "Bot combat config damage=" << botAttackDamage
        << " head=" << botHeadshotDamage
        << " cooldown=" << botFireIntervalSeconds;
    Logger::Info(logMessage.str());
}

void GameRoom::SetHitboxConfig(
    double botBodyScale,
    double botHeadScale,
    double playerBodyScale,
    double playerHeadScale
)
{
    std::lock_guard lock(mutex);

    auto clampScale = [](double value)
    {
        return std::clamp(value, MinHitboxScale, MaxHitboxScale);
    };

    if (botBodyScale > 0.0)
    {
        botBodyHitboxScale = clampScale(botBodyScale);
    }
    if (botHeadScale > 0.0)
    {
        botHeadHitboxScale = clampScale(botHeadScale);
    }
    if (playerBodyScale > 0.0)
    {
        playerBodyHitboxScale = clampScale(playerBodyScale);
    }
    if (playerHeadScale > 0.0)
    {
        playerHeadshotHitboxScale = clampScale(playerHeadScale);
    }

    currentDemoPreset = "custom";
    std::ostringstream logMessage;
    logMessage
        << "Hitbox config bot_body=" << botBodyHitboxScale
        << " bot_head=" << botHeadHitboxScale
        << " player_body=" << playerBodyHitboxScale
        << " player_head=" << playerHeadshotHitboxScale;
    Logger::Info(logMessage.str());
}

void GameRoom::ApplyBotDifficultyUnlocked(const std::string& difficulty)
{
    botDifficulty = difficulty;
    if (difficulty == "easy")
    {
        botAttackDamage = BotBodyDamage;
        botHeadshotDamage = BotHeadshotDamage;
        botAttackCooldownSeconds = 0.5;
        botDetectRange = 1000.0;
        botAttackRange = 950.0;
        botMoveSpeed = 260.0;
        botFireIntervalSeconds = 0.5;
        botAimSpreadDegrees = 12.0;
        botFireChance = 0.55;
        botWanderRadius = 350.0;
        botWanderStepMin = 120.0;
        botWanderStepMax = 350.0;
        botWanderWaitMin = 0.6;
        botWanderWaitMax = 1.8;
        BotWanderRepathSeconds = 0.8;
        MaxBotsTargetingOnePlayer = 4;
        MaxBotsShootingOnePlayer = 2;
        respawnInvincibleSeconds = 2.5;
    }
    else if (difficulty == "hard")
    {
        botAttackDamage = BotBodyDamage;
        botHeadshotDamage = BotHeadshotDamage;
        botAttackCooldownSeconds = 0.5;
        botDetectRange = 1900.0;
        botAttackRange = 1600.0;
        botMoveSpeed = 550.0;
        botFireIntervalSeconds = 0.5;
        botAimSpreadDegrees = 8.0;
        botFireChance = 0.85;
        botWanderRadius = 700.0;
        botWanderStepMin = 200.0;
        botWanderStepMax = 700.0;
        botWanderWaitMin = 0.25;
        botWanderWaitMax = 0.8;
        BotWanderRepathSeconds = 0.6;
        MaxBotsTargetingOnePlayer = 8;
        MaxBotsShootingOnePlayer = 4;
        respawnInvincibleSeconds = 1.5;
    }
    else
    {
        botDifficulty = "normal";
        botAttackDamage = BotBodyDamage;
        botHeadshotDamage = BotHeadshotDamage;
        botAttackCooldownSeconds = 0.5;
        botDetectRange = 1600.0;
        botAttackRange = 1300.0;
        botMoveSpeed = 450.0;
        botFireIntervalSeconds = 0.5;
        botAimSpreadDegrees = 13.0;
        botFireChance = 0.65;
        botWanderRadius = 500.0;
        botWanderStepMin = 160.0;
        botWanderStepMax = 500.0;
        botWanderWaitMin = 0.4;
        botWanderWaitMax = 1.2;
        BotWanderRepathSeconds = 1.0;
        MaxBotsTargetingOnePlayer = 8;
        MaxBotsShootingOnePlayer = 3;
        respawnInvincibleSeconds = 2.0;
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
        bot.targetReconsiderTimerSeconds = 0.0;
        bot.decisionTimerSeconds = std::min(bot.decisionTimerSeconds, BotWanderRepathSeconds);
        bot.wanderWaitTimerSeconds = std::min(bot.wanderWaitTimerSeconds, botWanderWaitMax);
        if (bot.targetPlayerId == 0)
        {
            AssignNearbyBotWanderTarget(bot);
        }
    }
}

void GameRoom::SetAutoEndMatchByTimer(bool bEnabled)
{
    std::lock_guard lock(mutex);
    bAutoEndMatchByTimer = bEnabled;
    currentDemoPreset = "custom";
    Logger::Info(
        std::string("Auto end match by timer=")
        + (bAutoEndMatchByTimer ? "true" : "false")
    );
}

bool GameRoom::IsInsideBotArea(double x, double y) const
{
    return x >= botAreaBounds.minX
        && x <= botAreaBounds.maxX
        && y >= botAreaBounds.minY
        && y <= botAreaBounds.maxY;
}

bool GameRoom::IsInsideArena(double x, double y) const
{
    return x >= arenaBounds.minX
        && x <= arenaBounds.maxX
        && y >= arenaBounds.minY
        && y <= arenaBounds.maxY;
}

double GameRoom::ClampArenaX(double x) const
{
    return std::clamp(x, arenaBounds.minX, arenaBounds.maxX);
}

double GameRoom::ClampArenaY(double y) const
{
    return std::clamp(y, arenaBounds.minY, arenaBounds.maxY);
}

double GameRoom::ClampBotX(double x) const
{
    return std::clamp(x, botAreaBounds.minX, botAreaBounds.maxX);
}

double GameRoom::ClampBotY(double y) const
{
    return std::clamp(y, botAreaBounds.minY, botAreaBounds.maxY);
}

std::pair<double, double> GameRoom::ClampToBotArea(double x, double y) const
{
    return {ClampBotX(x), ClampBotY(y)};
}

std::size_t GameRoom::SelectRandomIndex(std::size_t count) const
{
    if (count <= 1)
    {
        return 0;
    }

    std::uniform_int_distribution<std::size_t> distribution(0, count - 1);
    return distribution(spawnRandom);
}

bool GameRoom::IsSpawnTooCloseToAlivePlayer(
    const ArenaPoint& spawnPoint,
    std::uint64_t ignoredPlayerId
) const
{
    const double minimumDistanceSquared =
        PlayerSpawnSeparationRadius * PlayerSpawnSeparationRadius;
    for (const auto& [playerId, player] : players)
    {
        if (
            playerId == ignoredPlayerId
            || !player.connected
            || !player.alive
        )
        {
            continue;
        }

        const double deltaX = player.x - spawnPoint.x;
        const double deltaY = player.y - spawnPoint.y;
        if ((deltaX * deltaX) + (deltaY * deltaY) < minimumDistanceSquared)
        {
            return true;
        }
    }

    return false;
}

ArenaPoint GameRoom::ChoosePlayerSpawnPoint(const PlayerState& player) const
{
    if (playerSpawnPoints.empty())
    {
        return ArenaPoint{0, "P-FALLBACK", 0.0, 0.0, 0.0};
    }

    const std::size_t startIndex = SelectRandomIndex(playerSpawnPoints.size());
    for (std::size_t offset = 0; offset < playerSpawnPoints.size(); ++offset)
    {
        const std::size_t candidateIndex = (startIndex + offset) % playerSpawnPoints.size();
        const ArenaPoint& candidate = playerSpawnPoints[candidateIndex];
        if (!IsSpawnTooCloseToAlivePlayer(candidate, player.playerId))
        {
            return candidate;
        }
    }

    return playerSpawnPoints[startIndex];
}

ArenaPoint GameRoom::ChooseBotSpawnPoint(std::uint64_t botId) const
{
    if (botSpawnPoints.empty())
    {
        return ArenaPoint{botId, "BOT-SPAWN-FALLBACK", 0.0, 0.0, 0.0};
    }

    return botSpawnPoints[SelectRandomIndex(botSpawnPoints.size())];
}

std::size_t GameRoom::GetInitialBotWaypointIndex(const ArenaPoint& spawn) const
{
    if (botWanderPoints.empty())
    {
        return 0;
    }

    return static_cast<std::size_t>((spawn.id - 1) % botWanderPoints.size());
}

std::size_t GameRoom::GetNextBotWaypointIndex(const BotState& bot) const
{
    if (botWanderPoints.empty())
    {
        return 0;
    }

    return static_cast<std::size_t>(
        (bot.currentWaypointIndex + 1 + (bot.botId % 3)) % botWanderPoints.size()
    );
}

void GameRoom::AssignBotWaypoint(BotState& bot, std::size_t waypointIndex) const
{
    if (botWanderPoints.empty())
    {
        bot.wanderTargetX = ClampBotX(bot.x);
        bot.wanderTargetY = ClampBotY(bot.y);
        bot.currentWaypointIndex = 0;
        return;
    }

    const std::size_t normalizedIndex = waypointIndex % botWanderPoints.size();
    const ArenaPoint& waypoint = botWanderPoints[normalizedIndex];
    bot.currentWaypointIndex = static_cast<std::uint64_t>(normalizedIndex);
    bot.wanderTargetX = ClampBotX(waypoint.x);
    bot.wanderTargetY = ClampBotY(waypoint.y);
    bot.decisionTimerSeconds = 2.0 + static_cast<double>(bot.botId % 3);
}

void GameRoom::AssignNextBotWaypoint(BotState& bot) const
{
    AssignBotWaypoint(bot, GetNextBotWaypointIndex(bot));
}

void GameRoom::AssignNearbyBotWanderTarget(BotState& bot) const
{
    const double seed =
        (serverTimeSeconds * 29.0)
        + (static_cast<double>(bot.botId) * 113.0)
        + (static_cast<double>(bot.currentWaypointIndex) * 17.0)
        + bot.x
        + (bot.y * 0.37);
    const double angle = DeterministicRange(0.0, TwoPi, seed);
    const double step = DeterministicRange(botWanderStepMin, botWanderStepMax, seed + 19.0);

    double targetX = bot.x + (std::cos(angle) * step);
    double targetY = bot.y + (std::sin(angle) * step);

    const double homeDeltaX = targetX - bot.homeX;
    const double homeDeltaY = targetY - bot.homeY;
    const double homeDistance = std::sqrt((homeDeltaX * homeDeltaX) + (homeDeltaY * homeDeltaY));
    if (botWanderRadius > 0.0 && homeDistance > botWanderRadius && homeDistance > 0.0001)
    {
        const double scale = botWanderRadius / homeDistance;
        targetX = bot.homeX + (homeDeltaX * scale);
        targetY = bot.homeY + (homeDeltaY * scale);
    }

    bot.wanderTargetX = ClampBotX(targetX);
    bot.wanderTargetY = ClampBotY(targetY);
    bot.decisionTimerSeconds = BotWanderRepathSeconds;
    bot.wanderWaitTimerSeconds = 0.0;
    bot.currentWaypointIndex += 1;
}

void GameRoom::AssignRespawnPosition(PlayerState& player) const
{
    if (playerSpawnPoints.empty())
    {
        player.x = 0.0;
        player.y = 0.0;
        player.z = 0.0;
        return;
    }

    const ArenaPoint spawnPoint = ChoosePlayerSpawnPoint(player);

    player.x = spawnPoint.x;
    player.y = spawnPoint.y;
    player.z = spawnPoint.z;

    std::ostringstream logMessage;
    logMessage
        << "[BattleGridServer] Player " << player.playerId
        << " spawned at " << spawnPoint.label
        << " pos=(" << player.x << "," << player.y << "," << player.z << ")";
    Logger::Info(logMessage.str());
}

void GameRoom::InitializeDefaultTargets() const
{
    if (targetsInitialized && !targets.empty())
    {
        return;
    }

    targets.clear();

    for (const ArenaPoint& corePosition : targetCorePositions)
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

    const std::size_t configuredBotCount = desiredBotCount > 0
        ? desiredBotCount
        : (bHasRuntimeMapMarkers ? RuntimeMarkerBotCount : botSpawnPoints.size());
    std::vector<std::size_t> spawnOrder(botSpawnPoints.size());
    std::iota(spawnOrder.begin(), spawnOrder.end(), 0);
    std::shuffle(spawnOrder.begin(), spawnOrder.end(), spawnRandom);

    for (std::size_t botIndex = 0; botIndex < configuredBotCount; ++botIndex)
    {
        const ArenaPoint spawn = botSpawnPoints.empty()
            ? ArenaPoint{static_cast<std::uint64_t>(botIndex + 1), "BOT-SPAWN-FALLBACK", 0.0, 0.0, 0.0}
            : botSpawnPoints[spawnOrder[botIndex % spawnOrder.size()]];

        BotState bot;
        bot.botId = static_cast<std::uint64_t>(botIndex + 1);
        bot.name = "BOT-" + std::to_string(bot.botId);
        bot.x = spawn.x;
        bot.y = spawn.y;
        bot.z = spawn.z;
        bot.homeX = spawn.x;
        bot.homeY = spawn.y;
        bot.yaw = 0.0;
        bot.hp = BotMaxHp;
        bot.maxHp = BotMaxHp;
        bot.alive = true;
        bot.invincible = false;
        bot.speed = botMoveSpeed;
        AssignBotWaypoint(bot, GetInitialBotWaypointIndex(spawn));
        AssignNearbyBotWanderTarget(bot);
        bot.wanderWaitTimerSeconds = DeterministicRange(
            botWanderWaitMin,
            botWanderWaitMax,
            static_cast<double>(bot.botId) * 31.0
        );
        ResetBotStuckState(bot);
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
        bot.targetReconsiderTimerSeconds = 0.0;
        bots.emplace(bot.botId, std::move(bot));

        Logger::Info(
            "[BattleGridServer] BOT-" + std::to_string(botIndex + 1)
            + " spawned at " + spawn.label
            + " pos=(" + std::to_string(bot.x)
            + "," + std::to_string(bot.y)
            + "," + std::to_string(bot.z) + ")"
        );
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

    healthPackSpawnPoints.reserve(healthPackProfileSpawnPoints.size());
    for (const ArenaPoint& spawnPoint : healthPackProfileSpawnPoints)
    {
        healthPackSpawnPoints.push_back(spawnPoint);
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

    for (const HealthPackInitialSpawn& spawn : initialHealthPackSpawns)
    {
        if (healthPackSpawnPoints.empty())
        {
            break;
        }

        const ArenaPoint& spawnPoint =
            healthPackSpawnPoints[spawn.spawnPointIndex % healthPackSpawnPoints.size()];

        HealthPackState healthPack;
        healthPack.healthPackId = spawn.healthPackId;
        healthPack.x = spawnPoint.x;
        healthPack.y = spawnPoint.y;
        healthPack.z = spawnPoint.z;
        healthPack.active = true;
        healthPack.healAmount = HealthPackHealAmount;
        healthPack.pickupRadius = HealthPackPickupRadius;
        healthPack.respawnTimerSeconds = 0.0;
        healthPack.respawnDelaySeconds = HealthPackRespawnSeconds;
        healthPacks.emplace(healthPack.healthPackId, healthPack);

        Logger::Info(
            "[BattleGridServer] HPACK-" + std::to_string(healthPack.healthPackId)
            + " spawned at " + spawnPoint.label
            + " pos=(" + std::to_string(healthPack.x)
            + "," + std::to_string(healthPack.y)
            + "," + std::to_string(healthPack.z) + ")"
        );
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

        player.botRecentDamageGraceTimerSeconds = std::max(
            0.0,
            player.botRecentDamageGraceTimerSeconds - deltaSeconds
        );

        if (!player.alive)
        {
            player.botRecentDamageGraceTimerSeconds = 0.0;
            player.respawnTimerSeconds -= deltaSeconds;
            if (player.respawnTimerSeconds > 0.0)
            {
                continue;
            }

            player.alive = true;
            player.invincible = true;
            player.hp = player.maxHp;
            player.respawnTimerSeconds = 0.0;
            player.invincibleTimerSeconds = respawnInvincibleSeconds;
            player.botRecentDamageGraceTimerSeconds = 0.0;
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

            const ArenaPoint spawnPoint = ChooseBotSpawnPoint(bot.botId);
            bot.Respawn(spawnPoint.x, spawnPoint.y, spawnPoint.z);
            bot.homeX = spawnPoint.x;
            bot.homeY = spawnPoint.y;
            bot.invincibleTimerSeconds = BotInvincibleSeconds;
            bot.speed = botMoveSpeed;
            bot.attackRange = botAttackRange;
            bot.fireIntervalSeconds = botFireIntervalSeconds;
            bot.aimSpreadDegrees = botAimSpreadDegrees;
            AssignBotWaypoint(bot, GetInitialBotWaypointIndex(spawnPoint));
            AssignNearbyBotWanderTarget(bot);
            bot.wanderWaitTimerSeconds = DeterministicRange(
                botWanderWaitMin,
                botWanderWaitMax,
                static_cast<double>(bot.botId) * 37.0
            );
            ResetBotStuckState(bot);

            Logger::Info(
                "[BattleGridServer] BOT-" + std::to_string(bot.botId)
                + " spawned at " + spawnPoint.label
                + " pos=(" + std::to_string(bot.x)
                + "," + std::to_string(bot.y)
                + "," + std::to_string(bot.z) + ")"
            );
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
        || targetPlayer.botRecentDamageGraceTimerSeconds > 0.0
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
    bot.fireCooldownSeconds = bot.fireIntervalSeconds + DeterministicRange(
        BotShotRandomDelayMin,
        BotShotRandomDelayMax,
        (serverTimeSeconds * 17.0)
            + (static_cast<double>(bot.botId) * 53.0)
            + (static_cast<double>(bot.ammo) * 11.0)
    );

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
            targetPlayer.headRadius * playerHeadshotHitboxScale,
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
                targetPlayer.bodyRadius * playerBodyHitboxScale,
                HitscanRange,
                hitDistance
            )
            )
        {
            bHit = true;
            damage = botAttackDamage;
        }
    }

    const double visualMissDistance = std::min(
        HitscanRange,
        std::max(botAttackRange, 1200.0)
    );
    const Vec3 visualEnd = bHit
        ? BuildHitPoint(origin, direction, hitDistance)
        : BuildHitPoint(origin, direction, visualMissDistance);

    CombatEvent botFiredEvent;
    botFiredEvent.type = "bot_fired";
    botFiredEvent.message = bot.name
        + " fired at "
        + targetPlayer.nickname
        + (bHit ? (bHeadshot ? " and headshot" : " and hit") : " and missed");
    botFiredEvent.shortMessage = bHit
        ? (bot.name + (bHeadshot ? " HEADSHOT" : " HIT"))
        : (bot.name + " fired");
    botFiredEvent.botId = bot.botId;
    botFiredEvent.shotId = nextEventId;
    botFiredEvent.targetPlayerId = targetPlayer.playerId;
    botFiredEvent.hit = bHit;
    botFiredEvent.headshot = bHeadshot;
    botFiredEvent.killerIsBot = true;
    botFiredEvent.victimIsPlayer = bHit;
    botFiredEvent.damage = damage;
    botFiredEvent.hitGroup = bHit ? (bHeadshot ? "head" : "body") : "miss";
    botFiredEvent.startX = origin.x;
    botFiredEvent.startY = origin.y;
    botFiredEvent.startZ = origin.z;
    botFiredEvent.endX = visualEnd.x;
    botFiredEvent.endY = visualEnd.y;
    botFiredEvent.endZ = visualEnd.z;
    botFiredEvent.impactX = visualEnd.x;
    botFiredEvent.impactY = visualEnd.y;
    botFiredEvent.impactZ = visualEnd.z;
    botFiredEvent.hitX = visualEnd.x;
    botFiredEvent.hitY = visualEnd.y;
    botFiredEvent.hitZ = visualEnd.z;
    AddCombatEvent(botFiredEvent);

    if (bVerboseBotShotEvents || bVerboseBotStateLogs)
    {
        std::ostringstream fireLog;
        fireLog
            << "[BattleGridServer] BOT_FIRE bot=" << bot.botId
            << " target=" << targetPlayer.playerId
            << " hit=" << (bHit ? "true" : "false")
            << " damage=" << damage;
        Logger::Info(fireLog.str());
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
    targetPlayer.botRecentDamageGraceTimerSeconds = BotRecentDamageGraceSeconds;

    const Vec3 hitPoint = visualEnd;
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
    hitEvent.shotId = botFiredEvent.shotId;
    hitEvent.targetPlayerId = targetPlayer.playerId;
    hitEvent.hit = true;
    hitEvent.headshot = bHeadshot;
    hitEvent.killerIsBot = true;
    hitEvent.victimIsPlayer = true;
    hitEvent.damage = damage;
    hitEvent.hitGroup = bHeadshot ? "head" : "body";
    hitEvent.hitX = hitPoint.x;
    hitEvent.hitY = hitPoint.y;
    hitEvent.hitZ = hitPoint.z;
    hitEvent.startX = origin.x;
    hitEvent.startY = origin.y;
    hitEvent.startZ = origin.z;
    hitEvent.endX = hitPoint.x;
    hitEvent.endY = hitPoint.y;
    hitEvent.endZ = hitPoint.z;
    hitEvent.impactX = hitPoint.x;
    hitEvent.impactY = hitPoint.y;
    hitEvent.impactZ = hitPoint.z;
    AddCombatEvent(hitEvent);

    if (bVerboseInputLogs || bHeadshot)
    {
        std::ostringstream hitLogMessage;
        hitLogMessage
            << "Bot shot hit bot_id=" << bot.botId
            << " player_id=" << targetPlayer.playerId
            << " damage=" << damage
            << " headshot=" << (bHeadshot ? "true" : "false")
            << " hp=" << targetPlayer.hp
            << "/" << targetPlayer.maxHp;
        Logger::Info(hitLogMessage.str());
    }

    if (targetPlayer.hp > 0 || !targetPlayer.alive)
    {
        return;
    }

    targetPlayer.alive = false;
    targetPlayer.invincible = false;
    targetPlayer.deaths += 1;
    targetPlayer.respawnTimerSeconds = PlayerRespawnSeconds;
    targetPlayer.invincibleTimerSeconds = 0.0;
    targetPlayer.botRecentDamageGraceTimerSeconds = 0.0;
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
    std::unordered_map<std::uint64_t, int> targetingCounts;
    std::unordered_map<std::uint64_t, int> shootingCounts;

    std::vector<std::uint64_t> botIds;
    botIds.reserve(bots.size());
    for (const auto& [botId, bot] : bots)
    {
        if (bot.IsAlive())
        {
            botIds.push_back(botId);
        }
    }
    std::sort(botIds.begin(), botIds.end());

    const int maxTargeting = std::max(0, MaxBotsTargetingOnePlayer);
    const int maxShooting = std::max(0, MaxBotsShootingOnePlayer);
    int activeShootersThisTick = 0;

    auto isValidBotTarget = [](const PlayerState& player)
    {
        return player.connected && player.alive && !player.invincible;
    };

    auto getNoTargetReason = [this](const BotState& bot)
    {
        bool bHasConnectedPlayer = false;
        bool bHasAlivePlayer = false;
        bool bHasNonInvinciblePlayer = false;
        double nearestDistanceSquared = std::numeric_limits<double>::max();

        for (const auto& [playerId, player] : players)
        {
            static_cast<void>(playerId);
            if (!player.connected)
            {
                continue;
            }

            bHasConnectedPlayer = true;
            if (!player.alive)
            {
                continue;
            }

            bHasAlivePlayer = true;
            if (player.invincible)
            {
                continue;
            }

            bHasNonInvinciblePlayer = true;
            const double deltaX = player.x - bot.x;
            const double deltaY = player.y - bot.y;
            nearestDistanceSquared = std::min(
                nearestDistanceSquared,
                (deltaX * deltaX) + (deltaY * deltaY)
            );
        }

        if (!bHasConnectedPlayer)
        {
            return std::string("no_target");
        }
        if (!bHasAlivePlayer)
        {
            return std::string("target_dead");
        }
        if (!bHasNonInvinciblePlayer)
        {
            return std::string("target_invincible");
        }
        if (nearestDistanceSquared > (botDetectRange * botDetectRange))
        {
            return std::string("out_of_detect_range");
        }
        return std::string("no_target");
    };

    for (std::uint64_t botId : botIds)
    {
        auto botIterator = bots.find(botId);
        if (botIterator == bots.end())
        {
            continue;
        }

        BotState& bot = botIterator->second;
        if (!bot.IsAlive())
        {
            continue;
        }

        const bool bWasReloading = bot.reloading;
        bot.TickWeapon(deltaSeconds);
        if (!bWasReloading && bot.reloading)
        {
            CombatEvent reloadEvent;
            reloadEvent.type = "bot_reload_started";
            reloadEvent.message = bot.name + " started reload";
            reloadEvent.shortMessage = bot.name + " reload";
            reloadEvent.botId = bot.botId;
            reloadEvent.startX = bot.x;
            reloadEvent.startY = bot.y;
            reloadEvent.startZ = bot.z + FireOriginHeight;
            AddCombatEvent(reloadEvent);
        }
        bot.decisionTimerSeconds -= deltaSeconds;
        bot.targetReconsiderTimerSeconds = std::max(
            0.0,
            bot.targetReconsiderTimerSeconds - deltaSeconds
        );

        PlayerState* targetPlayer = nullptr;
        const double detectRangeSquared = botDetectRange * botDetectRange;
        if (bot.targetPlayerId != 0 && bot.targetReconsiderTimerSeconds > 0.0)
        {
            auto playerIterator = players.find(bot.targetPlayerId);
            if (playerIterator != players.end() && isValidBotTarget(playerIterator->second))
            {
                PlayerState& player = playerIterator->second;
                const double deltaX = player.x - bot.x;
                const double deltaY = player.y - bot.y;
                const double distanceSquared = (deltaX * deltaX) + (deltaY * deltaY);
                if (distanceSquared <= detectRangeSquared)
                {
                    targetPlayer = &player;
                }
            }
        }

        if (targetPlayer == nullptr)
        {
            double bestDistanceSquared = detectRangeSquared;
            for (auto& [playerId, player] : players)
            {
                static_cast<void>(playerId);

                if (!isValidBotTarget(player))
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
        }

        double moveX = 0.0;
        double moveY = 0.0;
        bool bTriedToMove = false;

        if (targetPlayer)
        {
            const bool bTargetingAllowed =
                maxTargeting <= 0
                || targetingCounts[targetPlayer->playerId] < maxTargeting;
            if (bot.targetPlayerId != targetPlayer->playerId)
            {
                bot.targetReconsiderTimerSeconds = BotTargetReconsiderSeconds;
            }
            else if (bot.targetReconsiderTimerSeconds <= 0.0)
            {
                bot.targetReconsiderTimerSeconds = BotTargetReconsiderSeconds;
            }

            const double aimX = targetPlayer->x - bot.x;
            const double aimY = targetPlayer->y - bot.y;
            const double distance = std::sqrt((aimX * aimX) + (aimY * aimY));
            const double effectiveAttackRange = bot.attackRange > 0.0
                ? bot.attackRange
                : botAttackRange;
            const bool bInAttackRange = distance <= effectiveAttackRange;
            const bool bWantsToShoot = bInAttackRange;

            if (distance > 0.0001)
            {
                bot.yaw = std::atan2(aimY, aimX) * 180.0 / 3.14159265358979323846;
            }

            if (!bTargetingAllowed)
            {
                SetBotCombatState(
                    bot,
                    "suppressed",
                    "shooting_cap",
                    targetPlayer->playerId,
                    distance,
                    bWantsToShoot,
                    false,
                    shootingCounts[targetPlayer->playerId],
                    bVerboseBotStateLogs
                );
                bot.x = ClampBotX(bot.x);
                bot.y = ClampBotY(bot.y);
                ResetBotStuckState(bot);
                continue;
            }

            ++targetingCounts[targetPlayer->playerId];

            if (bInAttackRange)
            {
                // Bot attacks disabled means bots stop shooting; players can
                // still damage bots through server hitscan.
                bool bAllowedToShoot = false;
                std::string combatState = "aim";
                std::string fireBlockReason;
                int currentShootersForTarget = shootingCounts[targetPlayer->playerId];

                if (!bBotAttacksEnabled)
                {
                    fireBlockReason = "bot_attacks_disabled";
                }
                else if (targetPlayer->invincible)
                {
                    fireBlockReason = "target_invincible";
                }
                else if (!targetPlayer->alive)
                {
                    fireBlockReason = "target_dead";
                }
                else if (bot.reloading)
                {
                    combatState = "reload";
                    fireBlockReason = "reloading";
                }
                else if (bot.ammo <= 0)
                {
                    combatState = "reload";
                    fireBlockReason = "no_ammo";
                }
                else if (bot.fireCooldownSeconds > 0.0)
                {
                    fireBlockReason = "cooldown";
                }
                else if (
                    maxShooting > 0
                    && shootingCounts[targetPlayer->playerId] >= maxShooting
                )
                {
                    combatState = "suppressed";
                    fireBlockReason = "shooting_cap";
                }
                else if (targetPlayer->botRecentDamageGraceTimerSeconds > 0.0)
                {
                    combatState = "suppressed";
                    fireBlockReason = "shooting_cap";
                }
                else if (
                    DeterministicUnit(
                        (serverTimeSeconds * 23.0)
                        + (static_cast<double>(bot.botId) * 79.0)
                        + (static_cast<double>(bot.ammo) * 5.0)
                    ) > botFireChance
                )
                {
                    fireBlockReason = "fire_chance";
                    bot.fireCooldownSeconds = bot.fireIntervalSeconds;
                }
                else
                {
                    bAllowedToShoot = true;
                    ++shootingCounts[targetPlayer->playerId];
                    ++activeShootersThisTick;
                    currentShootersForTarget = shootingCounts[targetPlayer->playerId];
                    combatState = "shoot";
                    ProcessBotShot(bot, *targetPlayer);
                }

                SetBotCombatState(
                    bot,
                    combatState,
                    fireBlockReason,
                    targetPlayer->playerId,
                    distance,
                    bWantsToShoot,
                    bAllowedToShoot,
                    currentShootersForTarget,
                    bVerboseBotStateLogs
                );

                if (distance <= bot.preferredCombatRange)
                {
                    bot.x = ClampBotX(bot.x);
                    bot.y = ClampBotY(bot.y);
                    ResetBotStuckState(bot);
                    continue;
                }
            }
            else
            {
                SetBotCombatState(
                    bot,
                    "chase",
                    "out_of_attack_range",
                    targetPlayer->playerId,
                    distance,
                    false,
                    false,
                    shootingCounts[targetPlayer->playerId],
                    bVerboseBotStateLogs
                );
            }

            const std::pair<double, double> clampedChaseTarget =
                ClampToBotArea(targetPlayer->x, targetPlayer->y);
            moveX = clampedChaseTarget.first - bot.x;
            moveY = clampedChaseTarget.second - bot.y;
            const double moveDistance = std::sqrt((moveX * moveX) + (moveY * moveY));
            if (moveDistance > 0.0001)
            {
                moveX /= moveDistance;
                moveY /= moveDistance;
                bTriedToMove = true;
            }
        }
        else
        {
            bot.targetReconsiderTimerSeconds = 0.0;
            SetBotCombatState(
                bot,
                "wander",
                getNoTargetReason(bot),
                0,
                0.0,
                false,
                false,
                0,
                bVerboseBotStateLogs
            );
            const double deltaX = bot.wanderTargetX - bot.x;
            const double deltaY = bot.wanderTargetY - bot.y;
            const double distance = std::sqrt((deltaX * deltaX) + (deltaY * deltaY));

            if (distance < 80.0)
            {
                if (bot.wanderWaitTimerSeconds <= 0.0)
                {
                    bot.wanderWaitTimerSeconds = DeterministicRange(
                        botWanderWaitMin,
                        botWanderWaitMax,
                        (serverTimeSeconds * 41.0)
                            + (static_cast<double>(bot.botId) * 43.0)
                            + static_cast<double>(bot.currentWaypointIndex)
                    );
                }

                bot.wanderWaitTimerSeconds = std::max(
                    0.0,
                    bot.wanderWaitTimerSeconds - deltaSeconds
                );
                if (bot.wanderWaitTimerSeconds <= 0.0)
                {
                    AssignNearbyBotWanderTarget(bot);
                }
            }
            else if (bot.decisionTimerSeconds <= 0.0)
            {
                AssignNearbyBotWanderTarget(bot);
            }

            moveX = bot.wanderTargetX - bot.x;
            moveY = bot.wanderTargetY - bot.y;
            const double moveLength = std::sqrt((moveX * moveX) + (moveY * moveY));
            if (moveLength > 80.0)
            {
                moveX /= moveLength;
                moveY /= moveLength;
                bot.yaw = std::atan2(moveY, moveX) * 180.0 / 3.14159265358979323846;
                bTriedToMove = true;
            }
        }

        if (bTriedToMove)
        {
            bot.x = ClampBotX(bot.x + (moveX * bot.speed * deltaSeconds));
            bot.y = ClampBotY(bot.y + (moveY * bot.speed * deltaSeconds));

            const double baselineDeltaX = bot.x - bot.lastXForStuck;
            const double baselineDeltaY = bot.y - bot.lastYForStuck;
            const double baselineDistance =
                std::sqrt((baselineDeltaX * baselineDeltaX) + (baselineDeltaY * baselineDeltaY));
            if (baselineDistance < BotStuckMovementThreshold)
            {
                bot.stuckTimerSeconds += deltaSeconds;
                if (bot.stuckTimerSeconds >= BotStuckTimeoutSeconds)
                {
                    AssignNearbyBotWanderTarget(bot);
                    ResetBotStuckState(bot);
                    Logger::Info(
                        "[BattleGridServer] Bot stuck; selected nearby wander target bot_id="
                        + std::to_string(bot.botId)
                        + " wander_index="
                        + std::to_string(bot.currentWaypointIndex)
                    );
                }
            }
            else
            {
                ResetBotStuckState(bot);
            }
        }
        else
        {
            bot.x = ClampBotX(bot.x);
            bot.y = ClampBotY(bot.y);
            ResetBotStuckState(bot);
        }
    }

    if (
        bVerboseBotStateLogs
        && serverTimeSeconds - lastBotAttackTickLogTime >= 1.0
    )
    {
        lastBotAttackTickLogTime = serverTimeSeconds;
        std::ostringstream tickLog;
        tickLog
            << "[BattleGridServer] BotAttackTick bots=" << botIds.size()
            << " activeShooters=" << activeShootersThisTick;
        Logger::Info(tickLog.str());
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
        const ArenaPoint spawnPoint =
            ChooseHealthPackSpawnPoint(healthPackId);
        healthPack.Respawn(spawnPoint.x, spawnPoint.y, spawnPoint.z);

        std::ostringstream logMessage;
        logMessage
            << "[BattleGridServer] HPACK-" << healthPack.healthPackId
            << " spawned at " << spawnPoint.label
            << " pos=(" << healthPack.x << "," << healthPack.y << "," << healthPack.z << ")";
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

            const int previousHp = player.hp;
            player.hp = std::min(player.maxHp, player.hp + healthPack.healAmount);
            healthPack.Deactivate();

            std::ostringstream logMessage;
            logMessage
                << "Player picked health pack player_id=" << player.playerId
                << " health_pack_id=" << healthPack.healthPackId
                << " heal_amount=" << healthPack.healAmount
                << " hp_before=" << previousHp
                << " hp=" << player.hp
                << "/" << player.maxHp;
            Logger::Info(logMessage.str());

            CombatEvent event;
            event.type = "health_pack_picked";
            event.message =
                player.nickname
                + " picked up HPACK-"
                + std::to_string(healthPack.healthPackId)
                + " +"
                + std::to_string(healthPack.healAmount);
            event.shortMessage = "HEALED +" + std::to_string(healthPack.healAmount);
            event.actorPlayerId = player.playerId;
            event.healthPackId = healthPack.healthPackId;
            event.healAmount = healthPack.healAmount;
            AddCombatEvent(event);
            break;
        }
    }
}

ArenaPoint GameRoom::ChooseHealthPackSpawnPoint(
    std::uint64_t healthPackId
) const
{
    InitializeHealthPackSpawnPoints();
    if (healthPackSpawnPoints.empty())
    {
        return ArenaPoint{0, "HPACK-SPAWN-FALLBACK", 0.0, 0.0, 0.0};
    }

    static_cast<void>(healthPackId);
    const std::size_t spawnIndex = SelectRandomIndex(healthPackSpawnPoints.size());
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

    const bool bNoisyShotEvent =
        storedEvent.type == "shot_miss"
        || storedEvent.type == "bot_fired"
        || storedEvent.type == "bot_reload_started"
        || storedEvent.type == "bot_shot_miss"
        || storedEvent.type == "shot_hit_bot"
        || storedEvent.type == "shot_hit_player"
        || storedEvent.type == "bot_shot_hit_player";
    if (bVerboseInputLogs || !bNoisyShotEvent)
    {
        Logger::Info("Event: " + storedEvent.message);
    }
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
    if (matchState.matchEndedEventEmitted)
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

    const std::uint64_t selectedWinnerPlayerId = DetermineWinnerPlayerId();
    const auto winner = players.find(selectedWinnerPlayerId);
    const int winnerScore = winner != players.end()
        ? CalculateKillRaceScore(winner->second)
        : 0;
    const bool bHasRealWinner = selectedWinnerPlayerId != 0 && winnerScore > 0;
    const std::uint64_t winnerPlayerId = bHasRealWinner ? selectedWinnerPlayerId : 0;
    const std::string winnerNickname = bHasRealWinner && winner != players.end()
        ? winner->second.nickname
        : std::string();
    const int winnerKills = bHasRealWinner ? winnerScore : 0;

    matchState.EndMatch(
        winnerPlayerId,
        winnerNickname,
        bHasRealWinner ? winnerScore : 0,
        winnerKills,
        !bHasRealWinner
    );

    std::ostringstream logMessage;
    logMessage
        << "[BattleGrid] Match ended result="
        << (bHasRealWinner ? "winner" : "draw")
        << " winner=" << winnerPlayerId
        << " nickname=" << winnerNickname
        << " score=" << (bHasRealWinner ? winnerScore : 0)
        << " reason="
        << (matchState.timeRemainingSeconds <= 0.0 ? "time_limit" : "score_limit");
    Logger::Info(logMessage.str());

    CombatEvent event;
    event.type = "match_ended";
    event.message = !bHasRealWinner
        ? "Match ended. Draw"
        : "Match ended. Winner: "
            + (winnerNickname.empty() ? std::string("P") + std::to_string(winnerPlayerId) : winnerNickname);
    event.actorPlayerId = winnerPlayerId;
    AddCombatEvent(event);
    matchState.matchEndedEventEmitted = true;
    roomFlowState = "match_ending";
    matchEndingTimerSeconds = matchEndingReturnSeconds;
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

        const bool bPlayerReachedScoreEarlier =
            player.scoreReachedTimeSeconds > 0.0
            && (
                bestPlayer->scoreReachedTimeSeconds <= 0.0
                || player.scoreReachedTimeSeconds < bestPlayer->scoreReachedTimeSeconds
            );
        const bool bIsBetter =
            playerScore > bestScore
            || (
                playerScore == bestScore
                && player.deaths < bestPlayer->deaths
            )
            || (
                playerScore == bestScore
                && player.deaths == bestPlayer->deaths
                && bPlayerReachedScoreEarlier
            )
            || (
                playerScore == bestScore
                && player.deaths == bestPlayer->deaths
                && player.scoreReachedTimeSeconds == bestPlayer->scoreReachedTimeSeconds
                && player.playerId < bestPlayer->playerId
            );

        if (bIsBetter)
        {
            bestPlayer = &player;
            bestPlayerId = playerId;
        }
    }

    return bestPlayer && CalculateKillRaceScore(*bestPlayer) > 0
        ? bestPlayerId
        : 0;
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
            if (lhs->deaths != rhs->deaths)
            {
                return lhs->deaths < rhs->deaths;
            }
            const double lhsReachedTime =
                lhs->scoreReachedTimeSeconds > 0.0
                    ? lhs->scoreReachedTimeSeconds
                    : std::numeric_limits<double>::max();
            const double rhsReachedTime =
                rhs->scoreReachedTimeSeconds > 0.0
                    ? rhs->scoreReachedTimeSeconds
                    : std::numeric_limits<double>::max();
            if (lhsReachedTime != rhsReachedTime)
            {
                return lhsReachedTime < rhsReachedTime;
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
        entry["score_reached_time"] = player->scoreReachedTimeSeconds;
        entry["hp"] = player->hp;
        entry["alive"] = player->alive;
        scoreboard.push_back(std::move(entry));
    }

    return scoreboard;
}

void GameRoom::RefreshPlayerScore(PlayerState& player)
{
    const int previousScore = player.score;
    player.score = CalculateKillRaceScore(player);
    if (player.score <= 0)
    {
        player.scoreReachedTimeSeconds = 0.0;
        return;
    }
    if (player.score != previousScore)
    {
        player.scoreReachedTimeSeconds = serverTimeSeconds;
    }
}

void GameRoom::ApplyPlayerDamageToBot(
    PlayerState& shooter,
    BotState& hitBot,
    int damage,
    bool bHeadshot,
    const std::string& hitGroup,
    double hitX,
    double hitY,
    double hitZ,
    bool bVerboseHitLog
)
{
    if (!hitBot.CanBeDamaged())
    {
        return;
    }

    hitBot.ApplyDamage(damage);

    if (bVerboseHitLog || bHeadshot)
    {
        std::ostringstream hitLogMessage;
        hitLogMessage
            << "Player bot hit shooter=" << shooter.playerId
            << " bot=" << hitBot.botId
            << " group=" << hitGroup
            << " damage=" << damage
            << " hp=" << hitBot.hp
            << "/" << hitBot.maxHp;
        Logger::Info(hitLogMessage.str());
    }

    CombatEvent shotEvent;
    shotEvent.type = "shot_hit_bot";
    shotEvent.message = shooter.nickname
        + (bHeadshot ? " headshot " : " hit ")
        + hitBot.name
        + " for "
        + std::to_string(damage);
    shotEvent.shortMessage = BuildShotShortMessage(
        "bot",
        hitBot.botId,
        damage,
        bHeadshot
    );
    shotEvent.actorPlayerId = shooter.playerId;
    shotEvent.botId = hitBot.botId;
    shotEvent.headshot = bHeadshot;
    shotEvent.damage = damage;
    shotEvent.hitGroup = hitGroup;
    shotEvent.hitX = hitX;
    shotEvent.hitY = hitY;
    shotEvent.hitZ = hitZ;
    AddCombatEvent(shotEvent);

    if (!hitBot.IsAlive())
    {
        hitBot.respawnTimerSeconds = BotRespawnSeconds;
        shooter.kills += 1;
        shooter.botKills += 1;
        RefreshPlayerScore(shooter);

        std::ostringstream killLogMessage;
        killLogMessage
            << "Bot killed bot_id=" << hitBot.botId
            << " shooter=" << shooter.playerId
            << " score=" << shooter.score;
        Logger::Info(killLogMessage.str());

        CombatEvent event;
        event.type = "bot_killed";
        event.message = shooter.nickname
            + (bHeadshot ? " headshot " : " killed ")
            + hitBot.name;
        event.actorPlayerId = shooter.playerId;
        event.botId = hitBot.botId;
        event.headshot = bHeadshot;
        event.victimIsBot = true;
        event.killerIsPlayer = true;
        AddCombatEvent(event);
    }
}

bool GameRoom::ApplyClientBotHitClaim(
    std::uint64_t playerId,
    std::uint64_t shotId,
    std::uint64_t botId,
    int requestedDamage,
    bool bHeadshot,
    double hitX,
    double hitY,
    double hitZ,
    std::string& outReason
)
{
    std::lock_guard lock(mutex);

    if (!bUseClientHitClaimsForBots)
    {
        outReason = "client hit claims disabled";
        return false;
    }

    const auto playerIt = players.find(playerId);
    if (playerIt == players.end())
    {
        outReason = "player not found";
        return false;
    }

    PlayerState& shooter = playerIt->second;
    if (!shooter.connected || !shooter.alive)
    {
        outReason = "player not alive";
        return false;
    }

    if (matchState.IsGameOver())
    {
        outReason = "match over";
        return false;
    }

    if (shotId == 0 || shotId <= shooter.lastProcessedClientHitClaimShotId)
    {
        outReason = "duplicate shot_id";
        return false;
    }

    const double cooldownRemaining =
        (shooter.lastClientHitClaimTimeSeconds + ClientHitClaimCooldownSeconds)
        - serverTimeSeconds;
    if (cooldownRemaining > 0.0)
    {
        outReason = "cooldown";
        return false;
    }

    InitializeDefaultBots();
    const auto botIt = bots.find(botId);
    if (botIt == bots.end())
    {
        outReason = "bot not found";
        return false;
    }

    BotState& hitBot = botIt->second;
    if (!hitBot.CanBeDamaged())
    {
        outReason = "bot not damageable";
        return false;
    }

    if (!std::isfinite(hitX) || !std::isfinite(hitY) || !std::isfinite(hitZ))
    {
        outReason = "invalid hit position";
        return false;
    }

    const double deltaX = hitX - hitBot.x;
    const double deltaY = hitY - hitBot.y;
    const double distance2d = std::sqrt((deltaX * deltaX) + (deltaY * deltaY));
    const double zDelta = std::abs(hitZ - hitBot.z);
    if (distance2d > MaxClientHitClaimBotDistance || zDelta > MaxClientHitClaimZDelta)
    {
        outReason = "hit position too far from bot";
        if (bVerboseInputLogs)
        {
            std::ostringstream rejectLog;
            rejectLog
                << "[BattleGridServer] Rejected client_hit_claim player=" << playerId
                << " bot=" << botId
                << " dist2d=" << distance2d
                << " z_delta=" << zDelta;
            Logger::Info(rejectLog.str());
        }
        return false;
    }

    const int allowedDamage = bHeadshot ? HeadshotDamage : BodyDamage;
    const int damage = std::clamp(requestedDamage, 0, allowedDamage);
    if (damage <= 0)
    {
        outReason = "invalid damage";
        return false;
    }

    shooter.lastProcessedClientHitClaimShotId = shotId;
    shooter.lastClientHitClaimTimeSeconds = serverTimeSeconds;
    ApplyPlayerDamageToBot(
        shooter,
        hitBot,
        damage,
        bHeadshot,
        bHeadshot ? "client_head" : "client_body",
        hitX,
        hitY,
        hitZ,
        bVerboseInputLogs || bHeadshot
    );

    std::ostringstream acceptedLog;
    acceptedLog
        << "[BattleGridServer] client_hit_claim accepted player=" << playerId
        << " bot=" << botId
        << " damage=" << damage
        << " headshot=" << (bHeadshot ? "true" : "false")
        << " shot_id=" << shotId;
    Logger::Info(acceptedLog.str());

    outReason = "accepted";
    return true;
}

bool GameRoom::ApplyClientPlayerHitClaim(
    std::uint64_t playerId,
    std::uint64_t shotId,
    std::uint64_t targetPlayerId,
    int requestedDamage,
    bool bHeadshot,
    double hitX,
    double hitY,
    double hitZ,
    std::string& outReason
)
{
    std::lock_guard lock(mutex);

    if (!bUseClientHitClaimsForPlayers)
    {
        outReason = "client player hit claims disabled";
        return false;
    }

    if (roomFlowState != "in_game")
    {
        outReason = "match not in progress";
        return false;
    }

    const auto shooterIt = players.find(playerId);
    if (shooterIt == players.end())
    {
        outReason = "player not found";
        return false;
    }

    PlayerState& shooter = shooterIt->second;
    if (!shooter.connected || !shooter.alive)
    {
        outReason = "player not alive";
        return false;
    }

    if (matchState.IsGameOver())
    {
        outReason = "match over";
        return false;
    }

    if (targetPlayerId == 0 || targetPlayerId == playerId)
    {
        outReason = "invalid target player";
        return false;
    }

    const auto targetIt = players.find(targetPlayerId);
    if (targetIt == players.end())
    {
        outReason = "target player not found";
        return false;
    }

    PlayerState& victimPlayer = targetIt->second;
    if (!victimPlayer.connected || !victimPlayer.alive || victimPlayer.invincible)
    {
        outReason = "target player not damageable";
        return false;
    }

    if (shotId == 0 || shotId <= shooter.lastProcessedClientHitClaimShotId)
    {
        outReason = "duplicate shot_id";
        return false;
    }

    const double cooldownRemaining =
        (shooter.lastClientHitClaimTimeSeconds + ClientHitClaimCooldownSeconds)
        - serverTimeSeconds;
    if (cooldownRemaining > 0.0)
    {
        outReason = "cooldown";
        return false;
    }

    if (!std::isfinite(hitX) || !std::isfinite(hitY) || !std::isfinite(hitZ))
    {
        outReason = "invalid hit position";
        return false;
    }

    const double headDx = hitX - victimPlayer.x;
    const double headDy = hitY - victimPlayer.y;
    const double headDz = hitZ - (victimPlayer.z + victimPlayer.headHeight);
    const double headDistance =
        std::sqrt((headDx * headDx) + (headDy * headDy) + (headDz * headDz));
    const double bodyDx = hitX - victimPlayer.x;
    const double bodyDy = hitY - victimPlayer.y;
    const double bodyDz = hitZ - (victimPlayer.z + victimPlayer.bodyHeight);
    const double bodyDistance =
        std::sqrt((bodyDx * bodyDx) + (bodyDy * bodyDy) + (bodyDz * bodyDz));
    const double effectiveHeadRadius = victimPlayer.headRadius * playerHeadshotHitboxScale;
    const double effectiveBodyRadius = victimPlayer.bodyRadius * playerBodyHitboxScale;
    const bool bHeadHit = headDistance <= effectiveHeadRadius;
    const bool bBodyHit = bodyDistance <= effectiveBodyRadius;

    if ((bHeadshot && !bHeadHit) || (!bHeadshot && !bBodyHit && !bHeadHit))
    {
        outReason = "hit position too far from player";
        if (bVerboseInputLogs)
        {
            std::ostringstream rejectLog;
            rejectLog
                << "[BattleGridServer] Rejected client_hit_claim player=" << playerId
                << " target_player=" << targetPlayerId
                << " head_distance=" << headDistance
                << " body_distance=" << bodyDistance
                << " head_radius=" << effectiveHeadRadius
                << " body_radius=" << effectiveBodyRadius;
            Logger::Info(rejectLog.str());
        }
        return false;
    }

    const int allowedDamage = bHeadshot ? HeadshotDamage : BodyDamage;
    const int damage = std::clamp(requestedDamage, 0, allowedDamage);
    if (damage <= 0)
    {
        outReason = "invalid damage";
        return false;
    }

    shooter.lastProcessedClientHitClaimShotId = shotId;
    shooter.lastClientHitClaimTimeSeconds = serverTimeSeconds;
    victimPlayer.hp = std::max(0, victimPlayer.hp - damage);

    if (bVerboseInputLogs || bHeadshot)
    {
        std::ostringstream hitLogMessage;
        hitLogMessage
            << "Client claim player hit shooter=" << shooter.playerId
            << " victim=" << victimPlayer.playerId
            << " damage=" << damage
            << " headshot=" << (bHeadshot ? "true" : "false")
            << " hp=" << victimPlayer.hp
            << "/" << victimPlayer.maxHp;
        Logger::Info(hitLogMessage.str());
    }

    CombatEvent shotEvent;
    shotEvent.type = "shot_hit_player";
    shotEvent.message = shooter.nickname
        + (bHeadshot ? " headshot " : " hit ")
        + victimPlayer.nickname
        + " for "
        + std::to_string(damage);
    shotEvent.shortMessage = BuildShotShortMessage(
        "player",
        victimPlayer.playerId,
        damage,
        bHeadshot
    );
    shotEvent.actorPlayerId = shooter.playerId;
    shotEvent.targetPlayerId = victimPlayer.playerId;
    shotEvent.headshot = bHeadshot;
    shotEvent.damage = damage;
    shotEvent.hitGroup = bHeadshot ? "client_head" : "client_body";
    shotEvent.hitX = hitX;
    shotEvent.hitY = hitY;
    shotEvent.hitZ = hitZ;
    AddCombatEvent(shotEvent);

    if (victimPlayer.hp <= 0)
    {
        victimPlayer.alive = false;
        victimPlayer.invincible = false;
        victimPlayer.deaths += 1;
        victimPlayer.respawnTimerSeconds = PlayerRespawnSeconds;
        victimPlayer.invincibleTimerSeconds = 0.0;
        victimPlayer.botRecentDamageGraceTimerSeconds = 0.0;
        victimPlayer.latestInput.fire = false;
        victimPlayer.latestInput.moveX = 0.0;
        victimPlayer.latestInput.moveY = 0.0;

        shooter.kills += 1;
        shooter.playerKills += 1;
        RefreshPlayerScore(shooter);

        std::ostringstream killLogMessage;
        killLogMessage
            << "Player killed by client claim killer=" << shooter.playerId
            << " victim=" << victimPlayer.playerId
            << " headshot=" << (bHeadshot ? "true" : "false")
            << " killer_score=" << shooter.score;
        Logger::Info(killLogMessage.str());

        CombatEvent event;
        event.type = "player_killed";
        event.message = shooter.nickname
            + (bHeadshot ? " headshot " : " killed ")
            + victimPlayer.nickname;
        event.actorPlayerId = shooter.playerId;
        event.targetPlayerId = victimPlayer.playerId;
        event.headshot = bHeadshot;
        event.victimIsPlayer = true;
        event.killerIsPlayer = true;
        AddCombatEvent(event);
        CheckMatchEndCondition();
    }

    std::ostringstream acceptedLog;
    acceptedLog
        << "[BattleGridServer] client_hit_claim accepted player=" << playerId
        << " target_player=" << targetPlayerId
        << " damage=" << damage
        << " headshot=" << (bHeadshot ? "true" : "false")
        << " shot_id=" << shotId;
    Logger::Info(acceptedLog.str());

    outReason = "accepted";
    return true;
}

void GameRoom::ProcessHitscanFire(PlayerState& shooter, const PlayerInput& input)
{
    if (!shooter.alive)
    {
        return;
    }

    if (matchState.IsGameOver())
    {
        CheckMatchEndCondition();
        return;
    }

    const Vec3 serverOrigin{shooter.x, shooter.y, shooter.z + FireOriginHeight};
    Vec3 origin = serverOrigin;
    std::string originSource = "server";
    if (bUseClientFireOriginForHitscan && input.hasFireOrigin)
    {
        const double deltaX = input.fireOriginX - shooter.x;
        const double deltaY = input.fireOriginY - shooter.y;
        const double originDistance = std::sqrt((deltaX * deltaX) + (deltaY * deltaY));
        const bool bFireOriginZValid =
            std::isfinite(input.fireOriginZ)
            && (!bRejectExtremeClientZ
                || (input.fireOriginZ >= MinAcceptedClientZ && input.fireOriginZ <= MaxAcceptedClientZ));
        if (originDistance <= MaxAcceptedClientFireOriginDistance && bFireOriginZValid)
        {
            origin = Vec3{input.fireOriginX, input.fireOriginY, input.fireOriginZ};
            originSource = "client";
            if (originDistance > ClientFireOriginWarningDistance)
            {
                std::ostringstream warningMessage;
                warningMessage
                    << "Client fire origin far from server position player=" << shooter.playerId
                    << " dist=" << originDistance;
                Logger::Warn(warningMessage.str());
            }
        }
        else
        {
            std::ostringstream rejectMessage;
            if (!bFireOriginZValid)
            {
                rejectMessage
                    << "[BattleGridServer] Rejected client fire origin z player="
                    << shooter.playerId
                    << " z=" << input.fireOriginZ;
            }
            else
            {
                rejectMessage
                    << "Rejected client fire origin player=" << shooter.playerId
                    << " dist=" << originDistance;
            }
            Logger::Warn(rejectMessage.str());
        }
    }

    const Vec3 direction = BuildShotDirection(input);

    if (bVerboseInputLogs)
    {
        std::ostringstream fireLogMessage;
        fireLogMessage
            << "Fire origin player=" << shooter.playerId
            << " source=" << originSource
            << " seq=" << input.seq
            << " origin=(" << origin.x << "," << origin.y << "," << origin.z << ")"
            << " dir=(" << direction.x << "," << direction.y << "," << direction.z << ")"
            << " ads=" << (input.ads ? "true" : "false")
            << " spread=" << input.spreadDegrees;
        Logger::Info(fireLogMessage.str());
    }

    HitscanHitResult selectedHit;

    auto considerHit = [this, &selectedHit](const HitscanHitResult& candidate)
    {
        if (!candidate.hit)
        {
            return;
        }

        if (bVerboseHitscanCandidateLogs && candidate.targetType == "bot")
        {
            std::ostringstream candidateLogMessage;
            candidateLogMessage
                << "Hitscan candidate bot=" << candidate.targetId
                << " group=" << candidate.hitGroup
                << " distance=" << candidate.distance;
            Logger::Info(candidateLogMessage.str());
        }

        if (IsCloserHit(candidate, selectedHit))
        {
            selectedHit = candidate;
        }
    };

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
            )
            {
                considerHit(BuildHitResult(
                    "target",
                    targetId,
                    hitDistance,
                    BodyDamage,
                    false,
                    "core",
                    origin,
                    direction
                ));
            }
        }
    }

    if (!bUseClientHitClaimsForBots)
    {
        for (const auto& [botId, bot] : bots)
        {
            // Bot attack toggles only stop bots from damaging players. Bots remain
            // valid player hitscan targets for movement/combat demos when client
            // hit claims are disabled.
            if (!bot.CanBeDamaged())
            {
                continue;
            }

            double hitDistance = 0.0;
            HitscanHitResult botHit;
            const Vec3 headCenter{bot.x, bot.y, bot.z + bot.headHeight};
            if (
                RaySphereIntersection(
                    origin,
                    direction,
                    headCenter,
                    bot.headRadius * botHeadHitboxScale,
                    HitscanRange,
                    hitDistance
                )
            )
            {
                botHit = BuildHitResult(
                    "bot",
                    botId,
                    hitDistance,
                    HeadshotDamage,
                    true,
                    "head",
                    origin,
                    direction
                );
            }
            else
            {
                const Vec3 bodyCenter{bot.x, bot.y, bot.z + bot.bodyHeight};
                if (
                    RaySphereIntersection(
                        origin,
                        direction,
                        bodyCenter,
                        bot.bodyRadius * botBodyHitboxScale,
                        HitscanRange,
                        hitDistance
                    )
                )
                {
                    botHit = BuildHitResult(
                        "bot",
                        botId,
                        hitDistance,
                        BodyDamage,
                        false,
                        "body",
                        origin,
                        direction
                    );
                }
            }

            double fallbackDistance = 0.0;
            if (
                !botHit.hit
                && bEnableBot2DFallbackHit
                && RayCircleIntersection2D(
                    origin,
                    direction,
                    bot.x,
                    bot.y,
                    std::max(0.0, bot.bodyRadius * botBodyHitboxScale * Bot2DFallbackRadiusScale),
                    HitscanRange,
                    fallbackDistance
                )
            )
            {
                botHit = BuildHitResult(
                    "bot",
                    botId,
                    fallbackDistance,
                    BodyDamage,
                    false,
                    "fallback_body",
                    origin,
                    direction
                );
            }

            considerHit(botHit);
        }
    }

    if (!bUseClientHitClaimsForPlayers)
    {
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
            HitscanHitResult playerHit;
            const Vec3 headCenter{victim.x, victim.y, victim.z + victim.headHeight};
            if (
                RaySphereIntersection(
                    origin,
                    direction,
                    headCenter,
                    victim.headRadius * playerHeadshotHitboxScale,
                    HitscanRange,
                    hitDistance
                )
            )
            {
                playerHit = BuildHitResult(
                    "player",
                    victimId,
                    hitDistance,
                    HeadshotDamage,
                    true,
                    "head",
                    origin,
                    direction
                );
            }
            else
            {
                const Vec3 bodyCenter{victim.x, victim.y, victim.z + victim.bodyHeight};
                if (
                    RaySphereIntersection(
                        origin,
                        direction,
                        bodyCenter,
                        victim.bodyRadius * playerBodyHitboxScale,
                        HitscanRange,
                        hitDistance
                    )
                )
                {
                    playerHit = BuildHitResult(
                        "player",
                        victimId,
                        hitDistance,
                        BodyDamage,
                        false,
                        "body",
                        origin,
                        direction
                    );
                }
            }

            considerHit(playerHit);
        }
    }

    if (bVerboseInputLogs && selectedHit.hit)
    {
        std::ostringstream selectedLogMessage;
        selectedLogMessage
            << "Hitscan selected shooter=" << shooter.playerId
            << " target=" << selectedHit.targetType << ":" << selectedHit.targetId
            << " group=" << selectedHit.hitGroup
            << " distance=" << selectedHit.distance
            << " damage=" << selectedHit.damage;
        Logger::Info(selectedLogMessage.str());
    }
    else if (bVerboseInputLogs)
    {
        Logger::Info(
            "Hitscan selected shooter=" + std::to_string(shooter.playerId)
            + " target=none miss"
        );
    }

    if (bTargetsEnabled && selectedHit.targetType == "target")
    {
        const auto target = targets.find(selectedHit.targetId);
        if (target == targets.end())
        {
            return;
        }

        target->second.ApplyDamage(selectedHit.damage);

        std::ostringstream hitLogMessage;
        hitLogMessage
            << "Hitscan target hit shooter=" << shooter.playerId
            << " target=" << selectedHit.targetId
            << " damage=" << selectedHit.damage
            << " hp=" << target->second.hp
            << "/" << target->second.maxHp;
        Logger::Info(hitLogMessage.str());

        CombatEvent shotEvent;
        shotEvent.type = "shot_hit_target";
        shotEvent.message = shooter.nickname
            + " hit CORE-"
            + std::to_string(selectedHit.targetId)
            + " for "
            + std::to_string(selectedHit.damage);
        shotEvent.shortMessage = BuildShotShortMessage(
            selectedHit.targetType,
            selectedHit.targetId,
            selectedHit.damage,
            selectedHit.headshot
        );
        shotEvent.actorPlayerId = shooter.playerId;
        shotEvent.targetId = selectedHit.targetId;
        shotEvent.damage = selectedHit.damage;
        shotEvent.hitGroup = selectedHit.hitGroup;
        shotEvent.hitX = selectedHit.hitX;
        shotEvent.hitY = selectedHit.hitY;
        shotEvent.hitZ = selectedHit.hitZ;
        AddCombatEvent(shotEvent);
        LogShotResult(shooter.playerId, selectedHit, bVerboseInputLogs);

        if (!target->second.IsAlive())
        {
            shooter.targetKills += 1;
            RefreshPlayerScore(shooter);

            std::ostringstream destroyLogMessage;
            destroyLogMessage
                << "Server target destroyed target_id=" << selectedHit.targetId
                << " shooter=" << shooter.playerId
                << " score=" << shooter.score;
            Logger::Info(destroyLogMessage.str());

            CombatEvent event;
            event.type = "target_destroyed";
            event.message = shooter.nickname + " destroyed CORE-" + std::to_string(selectedHit.targetId);
            event.actorPlayerId = shooter.playerId;
            event.targetId = selectedHit.targetId;
            AddCombatEvent(event);
        }

        return;
    }

    if (selectedHit.targetType == "bot")
    {
        const auto bot = bots.find(selectedHit.targetId);
        if (bot == bots.end())
        {
            return;
        }

        BotState& hitBot = bot->second;
        if (selectedHit.hitGroup == "fallback_body")
        {
            std::ostringstream fallbackLogMessage;
            fallbackLogMessage
                << "Hitscan bot body hit by 2D fallback shooter=" << shooter.playerId
                << " bot=" << hitBot.botId
                << " damage=" << selectedHit.damage
                << " hp=" << hitBot.hp
                << "/" << hitBot.maxHp
                << " radius_scale=" << Bot2DFallbackRadiusScale;
            if (bVerboseInputLogs || bVerboseHitscanCandidateLogs)
            {
                Logger::Info(fallbackLogMessage.str());
            }
        }

        ApplyPlayerDamageToBot(
            shooter,
            hitBot,
            selectedHit.damage,
            selectedHit.headshot,
            selectedHit.hitGroup,
            selectedHit.hitX,
            selectedHit.hitY,
            selectedHit.hitZ,
            bVerboseInputLogs || selectedHit.headshot
        );
        LogShotResult(shooter.playerId, selectedHit, bVerboseInputLogs);

        return;
    }

    if (selectedHit.targetType == "player")
    {
        const auto victim = players.find(selectedHit.targetId);
        if (victim == players.end())
        {
            return;
        }

        PlayerState& victimPlayer = victim->second;
        victimPlayer.hp = std::max(0, victimPlayer.hp - selectedHit.damage);

        if (bVerboseInputLogs || selectedHit.headshot)
        {
            std::ostringstream hitLogMessage;
            hitLogMessage
                << "Hitscan player hit shooter=" << shooter.playerId
                << " victim=" << victimPlayer.playerId
                << " damage=" << selectedHit.damage
                << " headshot=" << (selectedHit.headshot ? "true" : "false")
                << " hp=" << victimPlayer.hp
                << "/" << victimPlayer.maxHp;
            Logger::Info(hitLogMessage.str());
        }

        CombatEvent shotEvent;
        shotEvent.type = "shot_hit_player";
        shotEvent.message = shooter.nickname
            + (selectedHit.headshot ? " headshot " : " hit ")
            + victimPlayer.nickname
            + " for "
            + std::to_string(selectedHit.damage);
        shotEvent.shortMessage = BuildShotShortMessage(
            selectedHit.targetType,
            victimPlayer.playerId,
            selectedHit.damage,
            selectedHit.headshot
        );
        shotEvent.actorPlayerId = shooter.playerId;
        shotEvent.targetPlayerId = victimPlayer.playerId;
        shotEvent.headshot = selectedHit.headshot;
        shotEvent.damage = selectedHit.damage;
        shotEvent.hitGroup = selectedHit.hitGroup;
        shotEvent.hitX = selectedHit.hitX;
        shotEvent.hitY = selectedHit.hitY;
        shotEvent.hitZ = selectedHit.hitZ;
        AddCombatEvent(shotEvent);
        LogShotResult(shooter.playerId, selectedHit, bVerboseInputLogs);

        if (victimPlayer.hp <= 0)
        {
            victimPlayer.alive = false;
            victimPlayer.invincible = false;
            victimPlayer.deaths += 1;
            victimPlayer.respawnTimerSeconds = PlayerRespawnSeconds;
            victimPlayer.invincibleTimerSeconds = 0.0;
            victimPlayer.botRecentDamageGraceTimerSeconds = 0.0;
            victimPlayer.latestInput.fire = false;
            victimPlayer.latestInput.moveX = 0.0;
            victimPlayer.latestInput.moveY = 0.0;

            shooter.kills += 1;
            shooter.playerKills += 1;
            RefreshPlayerScore(shooter);

            std::ostringstream killLogMessage;
            killLogMessage
                << "Player killed killer=" << shooter.playerId
                << " victim=" << victimPlayer.playerId
                << " headshot=" << (selectedHit.headshot ? "true" : "false")
                << " killer_score=" << shooter.score;
            Logger::Info(killLogMessage.str());

            CombatEvent event;
            event.type = "player_killed";
            event.message = shooter.nickname
                + (selectedHit.headshot ? " headshot " : " killed ")
                + victimPlayer.nickname;
            event.actorPlayerId = shooter.playerId;
            event.targetPlayerId = victimPlayer.playerId;
            event.headshot = selectedHit.headshot;
            event.victimIsPlayer = true;
            event.killerIsPlayer = true;
            AddCombatEvent(event);
        }

        return;
    }

    if (
        bUseClientHitClaimsForBots
        && serverTimeSeconds - shooter.lastClientHitClaimTimeSeconds
            <= ClientHitClaimCooldownSeconds
    )
    {
        if (bVerboseInputLogs)
        {
            Logger::Info(
                "Suppressing hitscan miss after accepted client bot hit claim shooter="
                + std::to_string(shooter.playerId)
                + " seq=" + std::to_string(input.seq)
            );
        }
        return;
    }

    if (bVerboseInputLogs)
    {
        Logger::Info(
            "Hitscan missed shooter=" + std::to_string(shooter.playerId)
            + " seq=" + std::to_string(input.seq)
            + " dir=(" + std::to_string(direction.x)
            + "," + std::to_string(direction.y)
            + "," + std::to_string(direction.z)
            + ")"
        );
    }

    CombatEvent missEvent;
    missEvent.type = "shot_miss";
    missEvent.message = shooter.nickname + " missed";
    missEvent.shortMessage = BuildShotShortMessage(
        "",
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
    LogShotResult(shooter.playerId, selectedHit, bVerboseInputLogs);
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
                    RefreshPlayerScore(owner->second);
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
