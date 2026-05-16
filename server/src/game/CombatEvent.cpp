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
    json["shot_id"] = shotId;
    json["target_id"] = targetId;
    json["health_pack_id"] = healthPackId;
    json["hit"] = hit;
    json["headshot"] = headshot;
    json["victim_is_player"] = victimIsPlayer;
    json["victim_is_bot"] = victimIsBot;
    json["killer_is_bot"] = killerIsBot;
    json["killer_is_player"] = killerIsPlayer;
    json["damage"] = damage;
    json["heal_amount"] = healAmount;
    json["hit_group"] = hitGroup;
    json["hit_x"] = hitX;
    json["hit_y"] = hitY;
    json["hit_z"] = hitZ;
    json["start_x"] = startX;
    json["start_y"] = startY;
    json["start_z"] = startZ;
    json["end_x"] = endX;
    json["end_y"] = endY;
    json["end_z"] = endZ;
    json["impact_x"] = impactX;
    json["impact_y"] = impactY;
    json["impact_z"] = impactZ;
    return json;
}
}
