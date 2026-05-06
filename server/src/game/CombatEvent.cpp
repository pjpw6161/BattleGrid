#include "game/CombatEvent.h"

namespace battlegrid
{
nlohmann::json CombatEvent::ToJson() const
{
    nlohmann::json json;
    json["event_id"] = eventId;
    json["type"] = type;
    json["message"] = message;
    json["time"] = serverTimeSeconds;
    json["actor_player_id"] = actorPlayerId;
    json["target_player_id"] = targetPlayerId;
    json["bot_id"] = botId;
    json["target_id"] = targetId;
    json["health_pack_id"] = healthPackId;
    json["headshot"] = headshot;
    return json;
}
}
