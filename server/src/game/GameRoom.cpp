#include "game/GameRoom.h"

#include "core/Logger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <utility>

namespace battlegrid
{
namespace
{
constexpr double ArenaMin = -2000.0;
constexpr double ArenaMax = 2000.0;
constexpr double ProjectileArenaMin = -2500.0;
constexpr double ProjectileArenaMax = 2500.0;
constexpr double ProjectileSpawnForwardOffset = 50.0;
constexpr int BodyDamage = 20;
constexpr int HeadshotDamage = 40;
constexpr double HitscanRange = 3000.0;
constexpr bool bUseHitscanDamage = true;
constexpr bool bProjectileCollisionDamageEnabled = false;
constexpr double FireOriginHeight = 100.0;
constexpr double TargetCenterZ = 80.0;
constexpr double RespawnDelaySeconds = 8.0;
constexpr double RespawnInvincibilitySeconds = 1.5;
constexpr double BotDetectRange = 1500.0;
constexpr double BotAttackRange = 900.0;
constexpr double BotAttackDamage = 20.0;

struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

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
    int damage = 0;
};

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

void AssignRespawnPosition(PlayerState& player)
{
    const int spawnIndex = static_cast<int>((player.playerId - 1) % 4);
    const Vec3 spawnPoints[] = {
        {-600.0, -600.0, 0.0},
        {600.0, -600.0, 0.0},
        {-600.0, 600.0, 0.0},
        {600.0, 600.0, 0.0},
    };

    player.x = spawnPoints[spawnIndex].x;
    player.y = spawnPoints[spawnIndex].y;
    player.z = spawnPoints[spawnIndex].z;
}

nlohmann::json BuildPlayerSnapshotJson(const PlayerState& player)
{
    nlohmann::json playerJson;
    playerJson["player_id"] = player.playerId;
    playerJson["nickname"] = player.nickname;
    playerJson["x"] = player.x;
    playerJson["y"] = player.y;
    playerJson["hp"] = player.hp;
    playerJson["max_hp"] = player.maxHp;
    playerJson["alive"] = player.alive;
    playerJson["invincible"] = player.invincible;
    playerJson["score"] = player.score;
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
    return botJson;
}

nlohmann::json BuildProjectileSnapshotJson(const ProjectileState& projectile)
{
    nlohmann::json projectileJson;
    projectileJson["projectile_id"] = projectile.projectileId;
    projectileJson["owner_player_id"] = projectile.ownerPlayerId;
    projectileJson["x"] = projectile.x;
    projectileJson["y"] = projectile.y;
    projectileJson["dir_x"] = projectile.dirX;
    projectileJson["dir_y"] = projectile.dirY;
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
}

GameRoom::GameRoom(std::uint64_t inRoomId)
    : roomId(inRoomId),
      players(),
      bots(),
      projectiles(),
      targets(),
      nextProjectileId(1),
      targetsInitialized(false),
      botsInitialized(false),
      mutex()
{
    InitializeDefaultTargets();
    InitializeDefaultBots();
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
    }

    return inserted;
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
    InitializeDefaultTargets();
    InitializeDefaultBots();
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
        player.x = std::clamp(player.x, ArenaMin, ArenaMax);
        player.y = std::clamp(player.y, ArenaMin, ArenaMax);

        if (
            player.latestInput.fire
            && player.latestInput.seq != player.lastProcessedFireSeq
        )
        {
            double projectileDirX = player.latestInput.shotDirX;
            double projectileDirY = player.latestInput.shotDirY;
            const double shotDirectionLengthSquared =
                (projectileDirX * projectileDirX) + (projectileDirY * projectileDirY);
            if (shotDirectionLengthSquared <= 0.0001)
            {
                projectileDirX = player.latestInput.aimX;
                projectileDirY = player.latestInput.aimY;
            }

            SpawnProjectile(
                player.playerId,
                projectileDirX,
                projectileDirY
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
    if (bProjectileCollisionDamageEnabled || !bUseHitscanDamage)
    {
        UpdateProjectileTargetCollisions();
    }
    RemoveInactiveProjectiles();
    UpdateBots(deltaSeconds);
}

nlohmann::json GameRoom::BuildSnapshotJson(std::uint64_t tickNumber) const
{
    std::lock_guard lock(mutex);
    InitializeDefaultTargets();
    InitializeDefaultBots();

    nlohmann::json json;
    json["type"] = "snapshot";
    json["tick"] = tickNumber;
    json["room_id"] = roomId;
    json["players"] = nlohmann::json::array();
    json["bots"] = nlohmann::json::array();
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

    for (const auto& [targetId, target] : targets)
    {
        static_cast<void>(targetId);
        json["targets"].push_back(BuildTargetSnapshotJson(target));
    }

    return json;
}

nlohmann::json GameRoom::ToDebugJson() const
{
    std::lock_guard lock(mutex);
    InitializeDefaultTargets();
    InitializeDefaultBots();

    nlohmann::json json;
    json["room_id"] = roomId;
    json["player_count"] = players.size();
    json["projectile_count"] = projectiles.size();
    json["target_count"] = targets.size();
    json["bot_count"] = bots.size();
    json["players"] = nlohmann::json::array();
    json["bots"] = nlohmann::json::array();
    json["projectiles"] = nlohmann::json::array();
    json["targets"] = nlohmann::json::array();

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
        playerJson["score"] = player.score;
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
        json["targets"].push_back(BuildTargetSnapshotJson(target));
    }

    return json;
}

void GameRoom::InitializeDefaultTargets() const
{
    if (targetsInitialized && !targets.empty())
    {
        return;
    }

    targets.clear();

    struct TargetSpawn
    {
        std::uint64_t id;
        double x;
        double y;
    };

    const TargetSpawn targetSpawns[] = {
        {1, 600.0, 0.0},
        {2, 900.0, 300.0},
        {3, 900.0, -300.0},
        {4, 1200.0, 0.0},
        {5, 1500.0, 400.0},
    };

    for (const TargetSpawn& spawn : targetSpawns)
    {
        TargetState target;
        target.targetId = spawn.id;
        target.x = spawn.x;
        target.y = spawn.y;
        target.hp = 100;
        target.maxHp = 100;
        target.radius = 80.0;
        target.alive = true;
        targets.emplace(target.targetId, target);
    }

    targetsInitialized = true;
    Logger::Info("Initialized default server targets count=" + std::to_string(targets.size()));
}

void GameRoom::InitializeDefaultBots() const
{
    if (botsInitialized && !bots.empty())
    {
        return;
    }

    bots.clear();

    struct BotSpawn
    {
        std::uint64_t id;
        double x;
        double y;
    };

    const BotSpawn botSpawns[] = {
        {1, 300.0, 300.0},
        {2, 300.0, -300.0},
        {3, 700.0, 500.0},
        {4, 700.0, -500.0},
        {5, 1100.0, 500.0},
        {6, 1100.0, -500.0},
        {7, 1500.0, 200.0},
        {8, 1500.0, -200.0},
    };

    for (const BotSpawn& spawn : botSpawns)
    {
        BotState bot;
        bot.botId = spawn.id;
        bot.name = "BOT-" + std::to_string(spawn.id);
        bot.x = spawn.x;
        bot.y = spawn.y;
        bot.z = 0.0;
        bot.yaw = 0.0;
        bot.hp = 100;
        bot.maxHp = 100;
        bot.alive = true;
        bot.invincible = false;
        bot.speed = 500.0;
        bot.wanderTargetX = spawn.x;
        bot.wanderTargetY = spawn.y;
        bot.decisionTimerSeconds = 0.0;
        bot.attackCooldownSeconds = 1.0;
        bot.attackTimerSeconds = 0.0;
        bots.emplace(bot.botId, std::move(bot));
    }

    botsInitialized = true;
    Logger::Info("Initialized default bots count=" + std::to_string(bots.size()));
}

void GameRoom::SpawnProjectile(
    std::uint64_t ownerPlayerId,
    double dirX,
    double dirY
)
{
    const auto owner = players.find(ownerPlayerId);
    if (owner == players.end() || !owner->second.connected)
    {
        return;
    }

    ProjectileState projectile;
    projectile.projectileId = nextProjectileId++;
    projectile.ownerPlayerId = ownerPlayerId;
    projectile.dirX = dirX;
    projectile.dirY = dirY;
    projectile.NormalizeDirection();
    projectile.x = owner->second.x + (projectile.dirX * ProjectileSpawnForwardOffset);
    projectile.y = owner->second.y + (projectile.dirY * ProjectileSpawnForwardOffset);

    projectiles.emplace(projectile.projectileId, projectile);

    std::ostringstream logMessage;
    logMessage
        << "Projectile spawned id=" << projectile.projectileId
        << " owner=" << projectile.ownerPlayerId
        << " x=" << projectile.x
        << " y=" << projectile.y
        << " dir_x=" << projectile.dirX
        << " dir_y=" << projectile.dirY;
    Logger::Info(logMessage.str());
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
            player.invincibleTimerSeconds = RespawnInvincibilitySeconds;
            player.latestInput.moveX = 0.0;
            player.latestInput.moveY = 0.0;
            player.latestInput.fire = false;
            AssignRespawnPosition(player);

            Logger::Info("Player respawned player_id=" + std::to_string(player.playerId));
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

            const double respawnX = std::clamp(
                300.0 + (static_cast<double>((bot.botId - 1) % 4) * 400.0),
                ArenaMin,
                ArenaMax
            );
            const double respawnY = ((bot.botId % 2) == 0) ? -300.0 : 300.0;
            bot.Respawn(respawnX, respawnY);

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

void GameRoom::UpdateBotAI(double deltaSeconds)
{
    for (auto& [botId, bot] : bots)
    {
        static_cast<void>(botId);

        if (!bot.IsAlive())
        {
            continue;
        }

        bot.attackTimerSeconds = std::max(0.0, bot.attackTimerSeconds - deltaSeconds);
        bot.decisionTimerSeconds -= deltaSeconds;

        PlayerState* targetPlayer = nullptr;
        double bestDistanceSquared = BotDetectRange * BotDetectRange;

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

            if (distance <= BotAttackRange)
            {
                if (bot.attackTimerSeconds <= 0.0)
                {
                    targetPlayer->hp = std::max(
                        0,
                        targetPlayer->hp - static_cast<int>(BotAttackDamage)
                    );
                    bot.attackTimerSeconds = bot.attackCooldownSeconds;

                    std::ostringstream attackLogMessage;
                    attackLogMessage
                        << "Bot attacked player bot_id=" << bot.botId
                        << " player_id=" << targetPlayer->playerId
                        << " damage=" << static_cast<int>(BotAttackDamage)
                        << " hp=" << targetPlayer->hp
                        << "/" << targetPlayer->maxHp;
                    Logger::Info(attackLogMessage.str());

                    if (targetPlayer->hp <= 0 && targetPlayer->alive)
                    {
                        targetPlayer->alive = false;
                        targetPlayer->invincible = false;
                        targetPlayer->deaths += 1;
                        targetPlayer->respawnTimerSeconds = RespawnDelaySeconds;
                        targetPlayer->invincibleTimerSeconds = 0.0;
                        targetPlayer->latestInput.fire = false;
                        targetPlayer->latestInput.moveX = 0.0;
                        targetPlayer->latestInput.moveY = 0.0;

                        std::ostringstream killLogMessage;
                        killLogMessage
                            << "Bot killed player bot_id=" << bot.botId
                            << " player_id=" << targetPlayer->playerId;
                        Logger::Info(killLogMessage.str());
                    }
                }

                continue;
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
                bot.wanderTargetX = std::clamp(
                    std::fmod((botFactor * 733.0) + (bot.x * 0.37) + 2000.0, 4000.0) - 2000.0,
                    ArenaMin,
                    ArenaMax
                );
                bot.wanderTargetY = std::clamp(
                    std::fmod((botFactor * 419.0) + (bot.y * 0.53) + 2000.0, 4000.0) - 2000.0,
                    ArenaMin,
                    ArenaMax
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

        bot.x = std::clamp(bot.x + (moveX * bot.speed * deltaSeconds), ArenaMin, ArenaMax);
        bot.y = std::clamp(bot.y + (moveY * bot.speed * deltaSeconds), ArenaMin, ArenaMax);
    }
}

void GameRoom::ProcessHitscanFire(PlayerState& shooter, const PlayerInput& input)
{
    if (!shooter.alive)
    {
        return;
    }

    const Vec3 origin{shooter.x, shooter.y, shooter.z + FireOriginHeight};
    const Vec3 direction = BuildShotDirection(input);

    HitscanHit bestHit;

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
            bestHit.damage = BodyDamage;
        }
    }

    for (const auto& [botId, bot] : bots)
    {
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

        if (!target->second.IsAlive())
        {
            shooter.score += 1;
            shooter.targetKills += 1;

            std::ostringstream destroyLogMessage;
            destroyLogMessage
                << "Server target destroyed target_id=" << bestHit.targetId
                << " shooter=" << shooter.playerId
                << " score=" << shooter.score;
            Logger::Info(destroyLogMessage.str());
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

        if (!hitBot.IsAlive())
        {
            hitBot.respawnTimerSeconds = RespawnDelaySeconds;
            shooter.score += 1;
            shooter.kills += 1;
            shooter.botKills += 1;

            std::ostringstream killLogMessage;
            killLogMessage
                << "Bot killed bot_id=" << hitBot.botId
                << " shooter=" << shooter.playerId
                << " score=" << shooter.score;
            Logger::Info(killLogMessage.str());
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

        if (victimPlayer.hp <= 0)
        {
            victimPlayer.alive = false;
            victimPlayer.invincible = false;
            victimPlayer.deaths += 1;
            victimPlayer.respawnTimerSeconds = RespawnDelaySeconds;
            victimPlayer.invincibleTimerSeconds = 0.0;
            victimPlayer.latestInput.fire = false;
            victimPlayer.latestInput.moveX = 0.0;
            victimPlayer.latestInput.moveY = 0.0;

            shooter.score += 2;
            shooter.kills += 1;
            shooter.playerKills += 1;

            std::ostringstream killLogMessage;
            killLogMessage
                << "Player killed killer=" << shooter.playerId
                << " victim=" << victimPlayer.playerId
                << " headshot=" << (bestHit.headshot ? "true" : "false")
                << " killer_score=" << shooter.score;
            Logger::Info(killLogMessage.str());
        }
    }
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
        projectile.ageSeconds += deltaSeconds;

        if (
            projectile.ageSeconds > projectile.maxLifetimeSeconds
            || projectile.x < ProjectileArenaMin
            || projectile.x > ProjectileArenaMax
            || projectile.y < ProjectileArenaMin
            || projectile.y > ProjectileArenaMax
        )
        {
            projectile.active = false;
        }
    }
}

void GameRoom::UpdateProjectileTargetCollisions()
{
    for (auto& [projectileId, projectile] : projectiles)
    {
        if (!projectile.active)
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
                    owner->second.score += 1;
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
