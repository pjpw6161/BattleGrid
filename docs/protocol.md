# BattleGrid WebSocket JSON Protocol

BattleGrid currently uses UTF-8 JSON text frames over WebSocket for local client/server testing.

## Transport

- Default endpoint: `ws://127.0.0.1:7777`
- Transport: WebSocket
- Payload: compact JSON text
- Binary protocol: not implemented yet

## Client To Server Messages

### `ping`

```json
{ "type": "ping" }
```

Expected response:

```json
{ "type": "pong" }
```

### `join`

```json
{ "type": "join", "nickname": "player1" }
```

Expected response:

```json
{
  "type": "join_ok",
  "player_id": 1,
  "room_id": 1,
  "nickname": "player1"
}
```

Rules:

- If nickname is missing, empty, or not a string, the server uses `anonymous`.
- The first join on a WebSocket session creates `PlayerState` in Room 1.
- Repeated join messages on the same session return the existing identity.

### `input`

```json
{
  "type": "input",
  "seq": 1,
  "player_id": 1,
  "move_x": 1.0,
  "move_y": 0.0,
  "aim_x": 1.0,
  "aim_y": 0.0,
  "shot_dir_x": 1.0,
  "shot_dir_y": 0.0,
  "shot_dir_z": 0.0,
  "fire": false,
  "reload": false,
  "ads": false,
  "sprint": false,
  "jump": false,
  "ammo": 30,
  "spread_deg": 0.8
}
```

Fields:

- `seq`: client input sequence number.
- `player_id`: ID assigned by `join_ok`.
- `move_x`: server logical X movement axis.
- `move_y`: server logical Y movement axis.
- `aim_x`: server logical X aim direction.
- `aim_y`: server logical Y aim direction.
- `shot_dir_x`: server logical X shot direction after local spread is applied.
- `shot_dir_y`: server logical Y shot direction after local spread is applied.
- `shot_dir_z`: server vertical shot direction after local spread is applied.
- `fire`: true when a fire input occurred on this packet.
- `reload`: true when a reload input occurred on this packet.
- `ads`: true while the client is aiming down sights.
- `sprint`: true while the client is sprinting.
- `jump`: true while the client is jumping or falling.
- `ammo`: client-side magazine ammo count after local weapon processing.
- `spread_deg`: client-side spread value used for this shot/input.

`aim_x` / `aim_y` represent the intentional camera aim direction. `shot_dir_x` / `shot_dir_y` / `shot_dir_z` represent the actual shot direction after spread. If `shot_dir_x` and `shot_dir_y` are missing, the server falls back to `aim_x` and `aim_y` with `shot_dir_z = 0`.

Expected response:

```json
{
  "type": "input_ack",
  "seq": 1,
  "player_id": 1
}
```

Rules:

- The session must be joined.
- Message `player_id` must match the joined session.
- Accepted input replaces the player's latest stored `PlayerInput`.
- Fire input spawns one projectile during the server tick when the sequence has not already been processed.
- Server projectiles use `shot_dir_x` / `shot_dir_y` when present, otherwise `aim_x` / `aim_y`.

### `debug_room`

```json
{ "type": "debug_room" }
```

Expected response:

```json
{
  "type": "room_state",
  "room_id": 1,
  "player_count": 1,
  "projectile_count": 0,
  "targets_enabled": false,
  "targets_debug_count": 0,
  "bot_count": 8,
  "bot_attacks_enabled": true,
  "bot_difficulty": "normal",
  "bot_attack_damage": 20,
  "bot_attack_cooldown": 1.0,
  "bot_detect_range": 1500.0,
  "bot_attack_range": 900.0,
  "bot_move_speed": 500.0,
  "auto_end_match_by_timer": true,
  "health_pack_count": 3,
  "active_health_pack_count": 3,
  "match": {},
  "scoreboard": [],
  "events": [],
  "players": [],
  "bots": [],
  "health_packs": [],
  "projectiles": [],
  "targets_debug": []
}
```

`debug_room` is intended for browser testing and inspection.

Primary PvPvE kill race snapshots use `players`, `bots`, `health_packs`, `projectiles`, `match`, `scoreboard`, and `events`. `targets` is legacy/debug-only and is empty by default while `targets_enabled=false`.

### `debug_restart_match`

```json
{ "type": "debug_restart_match" }
```

Expected response:

```json
{
  "type": "match_restarted",
  "match_id": 2
}
```

Rules:

- This is a test/debug message, not a production rematch flow.
- The server resets match timer, winner, scores, player combat counters, projectiles, legacy targets, bots, and health packs.
- Existing joined players remain joined and are reset to full HP.
- Current bot difficulty, bot attack enablement, and timer auto-end settings are preserved.

### `debug_apply_demo_mode`

```json
{ "type": "debug_apply_demo_mode" }
```

Expected response:

```json
{ "type": "debug_ok", "message": "safe demo mode applied" }
```

Rules:

- This is a test/debug message, not a production admin command.
- The server resets the match, clears projectiles, resets players, bots, legacy targets, and health packs, and clears stale game-over state.
- Existing joined players remain joined, respawn at server spawn points, and return to full HP.
- The command applies stable demo settings:
  - bot difficulty `easy`
  - bot attacks disabled
  - timer-based match ending disabled
- A recent combat event is added with message `Safe demo mode applied`.

### `debug_set_bot_attacks`

```json
{ "type": "debug_set_bot_attacks", "enabled": false }
```

Expected response:

```json
{ "type": "debug_ok", "message": "bot attacks disabled" }
```

When disabled, bots can still move, wander, chase, respawn, and appear in snapshots, but they do not damage players.

### `debug_set_bot_difficulty`

```json
{ "type": "debug_set_bot_difficulty", "difficulty": "easy" }
```

Supported values:

- `easy`
- `normal`
- `hard`

Expected response:

```json
{ "type": "debug_ok", "message": "bot difficulty set to easy" }
```

Invalid values return:

```json
{ "type": "error", "message": "invalid bot difficulty" }
```

### `debug_set_match_timer`

```json
{ "type": "debug_set_match_timer", "enabled": false }
```

Expected response:

```json
{ "type": "debug_ok", "message": "auto end match by timer disabled" }
```

When disabled, the match timer can reach zero without forcing `game_over`. The kill-goal win condition still applies.

## Server To Client Messages

### `pong`

```json
{ "type": "pong" }
```

### `join_ok`

```json
{
  "type": "join_ok",
  "player_id": 1,
  "room_id": 1,
  "nickname": "player1"
}
```

### `input_ack`

```json
{
  "type": "input_ack",
  "seq": 1,
  "player_id": 1
}
```

### `room_state`

```json
{
  "type": "room_state",
  "room_id": 1,
  "player_count": 1,
  "projectile_count": 1,
  "targets_enabled": false,
  "targets_debug_count": 0,
  "bot_count": 8,
  "bot_attacks_enabled": true,
  "bot_difficulty": "normal",
  "bot_attack_damage": 20,
  "bot_attack_cooldown": 1.0,
  "bot_detect_range": 1500.0,
  "bot_attack_range": 900.0,
  "bot_move_speed": 500.0,
  "auto_end_match_by_timer": true,
  "health_pack_count": 3,
  "active_health_pack_count": 3,
  "match": {
    "state": "in_progress",
    "time_left": 287.5,
    "duration": 300.0,
    "target_score": 20,
    "game_over": false,
    "winner_player_id": 0,
    "winner_nickname": "",
    "match_id": 1
  },
  "scoreboard": [
    {
      "player_id": 1,
      "nickname": "player1",
      "score": 4,
      "kills": 4,
      "deaths": 1,
      "bot_kills": 3,
      "player_kills": 1,
      "target_kills": 0,
      "hp": 80,
      "alive": true
    }
  ],
  "events": [
    {
      "event_id": 1,
      "type": "bot_killed",
      "message": "player1 killed BOT-3",
      "short_message": "",
      "time": 123.4,
      "actor_player_id": 1,
      "target_player_id": 0,
      "bot_id": 3,
      "target_id": 0,
      "health_pack_id": 0,
      "headshot": false,
      "damage": 0,
      "hit_group": "",
      "hit_x": 0.0,
      "hit_y": 0.0,
      "hit_z": 0.0
    }
  ],
  "players": [
    {
      "player_id": 1,
      "room_id": 1,
      "nickname": "player1",
      "connected": true,
      "x": 100.0,
      "y": 0.0,
      "speed": 600.0,
      "hp": 100,
      "score": 0,
      "latest_input": {
        "seq": 10,
        "move_x": 1.0,
        "move_y": 0.0,
        "aim_x": 1.0,
        "aim_y": 0.0,
        "shot_dir_x": 1.0,
        "shot_dir_y": 0.0,
        "shot_dir_z": 0.0,
        "fire": false,
        "reload": false,
        "ads": false,
        "sprint": false,
        "jump": false,
        "ammo": 30,
        "spread_deg": 0.8
      }
    }
  ],
  "bots": [
    {
      "bot_id": 1,
      "name": "BOT-1",
      "x": 300.0,
      "y": 300.0,
      "z": 0.0,
      "yaw": -135.0,
      "hp": 100,
      "max_hp": 100,
      "alive": true,
      "invincible": false,
      "target_player_id": 1
    }
  ],
  "health_packs": [
    {
      "health_pack_id": 1,
      "x": -600.0,
      "y": 0.0,
      "z": 0.0,
      "active": true,
      "heal_amount": 35,
      "respawn_timer": 0.0
    }
  ],
  "projectiles": [
    {
      "projectile_id": 1,
      "owner_player_id": 1,
      "x": 150.0,
      "y": 0.0,
      "dir_x": 1.0,
      "dir_y": 0.0
    }
  ],
  "targets": [
    {
      "target_id": 1,
      "x": 600.0,
      "y": 0.0,
      "hp": 100,
      "max_hp": 100,
      "alive": true
    }
  ]
}
```

### `match_restarted`

```json
{
  "type": "match_restarted",
  "match_id": 2
}
```

This is sent in response to `debug_restart_match`.

### `debug_ok`

```json
{
  "type": "debug_ok",
  "message": "bot difficulty set to easy"
}
```

This is sent for successful debug/demo control messages such as `debug_apply_demo_mode`, `debug_set_bot_attacks`, `debug_set_bot_difficulty`, and `debug_set_match_timer`.

### `snapshot`

Snapshots are broadcast to joined sessions at the configured tick rate.

```json
{
  "type": "snapshot",
  "tick": 30,
  "room_id": 1,
  "match": {
    "state": "in_progress",
    "time_left": 287.5,
    "duration": 300.0,
    "target_score": 20,
    "game_over": false,
    "winner_player_id": 0,
    "winner_nickname": "",
    "match_id": 1
  },
  "scoreboard": [
    {
      "player_id": 1,
      "nickname": "player1",
      "score": 4,
      "kills": 4,
      "deaths": 1,
      "bot_kills": 3,
      "player_kills": 1,
      "target_kills": 0,
      "hp": 80,
      "alive": true
    }
  ],
  "events": [
    {
      "event_id": 1,
      "type": "bot_killed",
      "message": "player1 killed BOT-3",
      "short_message": "",
      "time": 123.4,
      "actor_player_id": 1,
      "target_player_id": 0,
      "bot_id": 3,
      "target_id": 0,
      "health_pack_id": 0,
      "headshot": false,
      "damage": 0,
      "hit_group": "",
      "hit_x": 0.0,
      "hit_y": 0.0,
      "hit_z": 0.0
    }
  ],
  "players": [
    {
      "player_id": 1,
      "nickname": "player1",
      "x": 100.0,
      "y": 0.0,
      "z": 0.0,
      "hp": 100,
      "max_hp": 100,
      "alive": true,
      "invincible": false,
      "respawn_timer": 0.0,
      "invincible_timer": 0.0,
      "score": 0,
      "kills": 0,
      "deaths": 0,
      "player_kills": 0,
      "target_kills": 0,
      "bot_kills": 0,
      "last_seq": 10
    }
  ],
  "bots": [
    {
      "bot_id": 1,
      "name": "BOT-1",
      "x": 300.0,
      "y": 300.0,
      "z": 0.0,
      "yaw": -135.0,
      "hp": 100,
      "max_hp": 100,
      "alive": true,
      "invincible": false,
      "target_player_id": 1
    }
  ],
  "health_packs": [
    {
      "health_pack_id": 1,
      "x": -600.0,
      "y": 0.0,
      "z": 0.0,
      "active": true,
      "heal_amount": 35,
      "respawn_timer": 0.0
    }
  ],
  "projectiles": [
    {
      "projectile_id": 1,
      "owner_player_id": 1,
      "x": 150.0,
      "y": 0.0,
      "dir_x": 1.0,
      "dir_y": 0.0
    }
  ],
  "targets_debug": []
}
```

Match fields:

- `state`: current match state, currently `in_progress` or `game_over`.
- `time_left`: seconds remaining in the match.
- `duration`: configured match duration in seconds.
- `target_score`: kill goal needed for immediate win.
- `game_over`: true after score or timer win condition is reached.
- `winner_player_id`: server player ID of the winner, or `0` while in progress.
- `winner_nickname`: winner display name, or empty while in progress.
- `match_id`: process-local match counter.

Scoreboard fields:

- `player_id`, `nickname`: player identity.
- `score`: authoritative kill race score, currently `bot_kills + player_kills`.
- `kills`: player plus bot kills.
- `deaths`: player deaths.
- `bot_kills`: bot kills worth +1 score.
- `player_kills`: player kills worth +1 score.
- `target_kills`: legacy/debug counter; not part of primary ranking while targets are disabled.
- `hp`: current server player HP.
- `alive`: whether the player is alive.

The `scoreboard` array is the source for the PvPvE player ranking. It contains connected players only; bots do not appear as ranking entries. The Unreal HUD displays the top 5 entries by `score`, then `player_kills`, `bot_kills`, fewer `deaths`, and lower `player_id`.

Combat event fields:

- `event_id`: monotonically increasing room-local event ID for deduplication.
- `type`: event type.
- `message`: human-readable short kill-feed message.
- `short_message`: compact HUD/browser display text for shot results, such as `SERVER HIT BOT-3 -20`.
- `time`: server room time in seconds.
- `actor_player_id`: player that caused the event, if any.
- `target_player_id`: player affected by the event, if any.
- `bot_id`: bot involved in the event, if any.
- `target_id`: server target involved in the event, if any.
- `health_pack_id`: health pack involved in the event, if any.
- `headshot`: true for headshot kill events.
- `victim_is_player`: true when the event victim is a player.
- `victim_is_bot`: true when the event victim is a bot.
- `killer_is_bot`: true when the event actor is a bot.
- `killer_is_player`: true when the event actor is a player.
- `damage`: damage amount for shot result events, or `0` when not applicable.
- `hit_group`: shot result hit group such as `head`, `body`, `legacy_target`, or `miss`.
- `hit_x`, `hit_y`, `hit_z`: approximate server-space hit position for shot result/debug display.

The Unreal kill log filters combat events to `bot_killed`, `player_killed`, and `bot_killed_player`. Own kills are displayed green, player death events are displayed red, and other bot kills are displayed neutral white/gray.

Current event types:

- `shot_hit_bot`
- `shot_hit_player`
- `shot_hit_target` legacy/debug only while targets are disabled
- `shot_miss`
- `bot_shot_hit_player`
- `bot_shot_miss` optional/verbose; suppressed by default to avoid miss-event spam
- `target_destroyed`
- `bot_killed`
- `player_killed`
- `bot_killed_player`
- `player_respawned`
- `health_pack_picked`
- `match_ended`
- `match_restarted`

Shot result events are emitted once per processed player `fire=true` input sequence while the match is not `game_over`. They are intended for browser/Unreal hit confirmation and do not replace the scoreboard or kill events. A bot headshot can produce both a `shot_hit_bot` event and, if HP reaches zero, a later `bot_killed` event in the same snapshot event feed.

Bot shooter events are emitted when bot hitscan fire damages or kills a player. Bot miss events are intentionally quiet by default because several bots can fire at once.

Example shot result event:

```json
{
  "event_id": 42,
  "type": "shot_hit_bot",
  "message": "player1 hit BOT-3 for 20",
  "short_message": "SERVER HIT BOT-3 -20",
  "time": 124.2,
  "actor_player_id": 1,
  "target_player_id": 0,
  "bot_id": 3,
  "target_id": 0,
  "health_pack_id": 0,
  "headshot": false,
  "damage": 20,
  "hit_group": "body",
  "hit_x": 300.0,
  "hit_y": 300.0,
  "hit_z": 90.0
}
```

Player fields:

- `player_id`: server player ID.
- `nickname`: player nickname from join.
- `x`, `y`, `z`: server logical position.
- `hp`: server player HP placeholder.
- `max_hp`: server player maximum HP.
- `alive`: whether the player can move and be damaged.
- `invincible`: true during the short post-respawn protection window.
- `respawn_timer`: seconds remaining until respawn when dead.
- `invincible_timer`: seconds remaining in post-respawn invincibility.
- `score`: server kill race score.
- `kills`, `deaths`, `player_kills`, `target_kills`, `bot_kills`: server combat counters.
- `last_seq`: latest input sequence stored for the player.

Bot fields:

- `bot_id`: fixed server bot ID.
- `name`: display name such as `BOT-1`.
- `x`, `y`, `z`: server bot position.
- `yaw`: server bot facing yaw in degrees.
- `hp`, `max_hp`: bot health.
- `alive`: whether the bot can move, attack, and be damaged.
- `invincible`: true during the short post-respawn protection window.
- `target_player_id`: current player target, or `0`.

Health pack fields:

- `health_pack_id`: fixed server health pack entity ID.
- `x`, `y`, `z`: server health pack position.
- `active`: whether the pack can currently be picked up.
- `heal_amount`: amount of HP restored on pickup.
- `respawn_timer`: seconds remaining until it respawns when inactive.

Projectile fields:

- `projectile_id`: server projectile ID.
- `owner_player_id`: player that fired the projectile, or `0` for non-player tracers.
- `owner_type`: `"player"` or `"bot"`.
- `owner_bot_id`: bot that fired the projectile when `owner_type` is `"bot"`.
- `x`, `y`: server logical 2D projectile position.
- `dir_x`, `dir_y`: normalized server logical movement direction.
- `visual_only`: true for tracer projectiles that do not apply collision damage.

Target fields:

- `target_id`: fixed server target ID.
- `x`, `y`: server logical 2D target position.
- `hp`, `max_hp`: server target health.
- `alive`: whether the target can still be damaged.

### `error`

Invalid JSON:

```json
{ "type": "error", "message": "invalid json" }
```

Unknown message type:

```json
{ "type": "error", "message": "unknown message type" }
```

Input before join:

```json
{ "type": "error", "message": "not joined" }
```

Mismatched player ID:

```json
{ "type": "error", "message": "player_id mismatch" }
```

## Current Server Simulation

- Room ID is fixed to `1`.
- Player IDs are process-local and reset when the server restarts.
- Movement integrates latest input at a fixed speed and clamps to the arena.
- Fire input creates server projectiles.
- Server projectiles use the spread-adjusted `shot_dir_x` / `shot_dir_y` fields when available and remain as tracer visualization.
- Server hitscan damage is applied immediately when a new fire input sequence is processed.
- Legacy target hitscan is disabled by default with the old target objective.
- Hitscan player damage checks head sphere first for 40 damage, then body sphere for 20 damage.
- Hitscan bot damage checks head sphere first for 40 damage, then body sphere for 20 damage.
- Server bot kills award +1 kill and increment `bot_kills`.
- Server player kills award +1 kill and increment `player_kills`.
- Legacy target kills do not affect primary ranking while targets are disabled.
- Dead players respawn after 8 seconds and are invincible for 1.5 seconds.
- Dead bots respawn after 8 seconds and are invincible for 1.5 seconds.
- Bots use simple server AI: move toward the nearest alive non-invincible player inside detect range, otherwise wander.
- Bot attacks are server hitscan gun shots with ammo, reload, low accuracy, and visual tracer projectiles.
- Bot body damage is 10 and bot headshot damage is 20.
- Debug bot difficulty tunes bot shooter AI:
  - `easy`: body/head 10/20, fire interval 0.75s, spread 18 degrees, detect range 1000, attack range 1200, speed 400.
  - `normal`: body/head 10/20, fire interval 0.5s, spread 12 degrees, detect range 1500, attack range 1400, speed 500.
  - `hard`: body/head 10/20, fire interval 0.35s, spread 7 degrees, detect range 1800, attack range 1600, speed 600.
- Three server health packs spawn from predefined points.
- Alive players below max HP pick up active health packs by overlapping their pickup radius.
- Health packs heal +35 HP, do not overheal above max HP, disappear on pickup, and respawn after 15 seconds.
- Bots ignore health packs for now.
- Match duration is 300 seconds and kill goal is 20.
- The match ends when any connected player reaches the kill goal or when the timer reaches zero.
- `debug_set_match_timer` can disable timer-based `game_over` for development while keeping kill-goal wins.
- Winner tie breakers are highest score, higher player kills, higher bot kills, fewer deaths, then lower player ID.
- After game over, snapshots continue, but new combat score changes are stopped.

## Current Limitations

- No binary protocol.
- No authentication.
- No real multiple-room support.
- Bot behavior is simple hitscan shooter AI with no pathfinding, animations, or advanced target selection.
- Health packs are server-side snapshot entities only; there are no local pickup effects, sounds, or imported models yet.
- No lag compensation or advanced hit validation yet.
- Legacy targets/cores are disabled by default and kept only as debug compatibility data.
- No authoritative synchronization with Unreal-placed local targets.
- `debug_restart_match` is a browser/testing utility, not a production match flow.
- No persistence or database.
