#include "game/CombatEvent.h"

namespace battlegrid
{
nlohmann::json CombatEvent::ToJson() const
{
    nlohmann::json json;
    json["event_id"] = eventId;
    json["type"] = type;
    json["message"] = message;
    json["short_message"] = shortMessage;
    json["time"] = serverTimeSeconds;
    json["actor_player_id"] = actorPlayerId;
    json["target_player_id"] = targetPlayerId;
    json["bot_id"] = botId;
    json["target_id"] = targetId;
    json["health_pack_id"] = healthPackId;
    json["headshot"] = headshot;
    json["damage"] = damage;
    json["hit_group"] = hitGroup;
    json["hit_x"] = hitX;
    json["hit_y"] = hitY;
    json["hit_z"] = hitZ;
    return json;
}
}
