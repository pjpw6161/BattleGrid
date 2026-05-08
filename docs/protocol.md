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
  "spread_deg": 0.8,
  "has_fire_origin": true,
  "fire_origin_x": -1200.0,
  "fire_origin_y": 0.0,
  "fire_origin_z": 100.0,
  "has_client_position": true,
  "client_x": -1198.0,
  "client_y": 4.0,
  "client_z": 0.0
}
```

Fields:

- `seq`: client input sequence number.
- `player_id`: ID assigned by `join_ok`.
- `move_x`: normalized camera-relative movement direction converted to server logical X.
- `move_y`: normalized camera-relative movement direction converted to server logical Y.
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
- `has_fire_origin`: optional. True when the client is providing a fire origin in server coordinate space for the current fire input.
- `fire_origin_x`, `fire_origin_y`, `fire_origin_z`: optional client-provided ray origin in server coordinates. Used for prototype/demo hitscan alignment when accepted by server sanity checks.
- `has_client_position`: optional. True when the Unreal client is providing its current pawn location in server coordinate space for prototype/demo movement synchronization.
- `client_x`, `client_y`, `client_z`: optional client pawn position. Used by the demo server to keep `PlayerState` / `SERVER ECHO` aligned with the actual Unreal pawn when accepted by sanity checks. X/Y use server arena coordinates; `client_z` preserves the Unreal world height in Unreal units for prototype vertical alignment.

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
- Fire input spawns one visual-only tracer during the server tick when the sequence has not already been processed.
- In demo/prototype mode, if `has_client_position=true`, the server can use `client_x/y/z` as the player's current position after sanity checks. This keeps `SERVER ECHO` aligned with the Unreal pawn and avoids drift caused by missing server-side Unreal collision. X/Y are server arena coordinates; Z is currently an Unreal-height value, not a terrain-simulated server physics value.
- If client position is missing or rejected, movement falls back to integrating the latest normalized move vector with an effective speed of 600 walk, 850 sprint, or 400 ADS walk.
- If both sprint and ADS are true, sprint speed wins. Dead players do not move.
- Server hitscan and tracer direction use the full `shot_dir_x` / `shot_dir_y` / `shot_dir_z` vector when present, otherwise the server falls back to `aim_x` / `aim_y` with `shot_dir_z = 0`.
- In demo/prototype mode, the server may accept `fire_origin_*` as the ray origin for hitscan if `has_fire_origin=true` and the origin is within `max_accepted_client_fire_origin_distance` of the authoritative player position. If missing or rejected, the server falls back to the authoritative player position.

### `debug_room`

```json
{ "type": "debug_room" }
```

Expected response:

```json
{
  "type": "room_state",
  "room_id": 1,
  "arena": {
    "bounds": {
      "min_x": -5000.0,
      "max_x": 5000.0,
      "min_y": -5000.0,
      "max_y": 5000.0
    }
  },
  "arena_bounds": {
    "min_x": -5000.0,
    "max_x": 5000.0,
    "min_y": -5000.0,
    "max_y": 5000.0
  },
  "bot_area_bounds": {
    "min_x": -1500.0,
    "max_x": 1500.0,
    "min_y": -900.0,
    "max_y": 900.0
  },
  "bot_waypoints": [
    { "waypoint_id": 1, "label": "WP-1", "x": -1200.0, "y": 0.0, "z": 0.0 }
  ],
  "player_count": 1,
  "projectile_count": 0,
  "targets_enabled": false,
  "targets_debug_count": 0,
  "bot_count": 8,
  "bot_attacks_enabled": true,
  "bot_difficulty": "normal",
  "bot_attack_damage": 10,
  "bot_headshot_damage": 20,
  "bot_attack_cooldown": 0.75,
  "bot_detect_range": 1600.0,
  "bot_attack_range": 1300.0,
  "bot_move_speed": 450.0,
  "bot_fire_interval": 0.75,
  "bot_aim_spread": 13.0,
  "max_bots_targeting_one_player": 8,
  "max_bots_shooting_one_player": 3,
  "respawn_invincible_seconds": 2.0,
  "bot_target_reconsider_seconds": 1.0,
  "bot_shot_random_delay_min": 0.1,
  "bot_shot_random_delay_max": 0.35,
  "bot_recent_damage_grace_seconds": 0.15,
  "enable_bot_2d_fallback_hit": true,
  "bot_2d_fallback_radius_scale": 0.75,
  "verbose_hitscan_candidate_logs": false,
  "use_client_fire_origin_for_hitscan": true,
  "client_fire_origin_warning_distance": 300.0,
  "max_accepted_client_fire_origin_distance": 2000.0,
  "use_client_position_for_player_movement": true,
  "max_client_position_delta_per_second": 1400.0,
  "max_client_position_snap_distance": 3000.0,
  "walk_speed": 600.0,
  "sprint_speed": 850.0,
  "ads_walk_speed": 400.0,
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

`arena.bounds` exposes the server-authoritative clamp rectangle. Unreal can draw this rectangle for debugging because correction can pull the local pawn back when the server position reaches those bounds.

`bot_area_bounds` and `bot_waypoints` expose the smaller prototype bot navigation area. Browser and Unreal debug views can use these fields to verify bots are spawning and wandering inside the current visible demo map. The full server response contains all 13 bot waypoints.

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

Current pressure values:

| Difficulty | Body / Head | Fire Interval | Spread | Detect | Attack Range | Speed | Max Targeting | Max Shooting | Respawn Invincible |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `easy` | 10 / 20 | 1.0s | 18 deg | 1400 | 1100 | 380 | 8 | 2 | 2.5s |
| `normal` | 10 / 20 | 0.75s | 13 deg | 1600 | 1300 | 450 | 8 | 3 | 2.0s |
| `hard` | 10 / 20 | 0.5s | 8 deg | 1900 | 1600 | 550 | 8 | 4 | 1.5s |

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
  "arena": {
    "bounds": {
      "min_x": -5000.0,
      "max_x": 5000.0,
      "min_y": -5000.0,
      "max_y": 5000.0
    }
  },
  "player_count": 1,
  "projectile_count": 1,
  "targets_enabled": false,
  "targets_debug_count": 0,
  "bot_count": 8,
  "bot_attacks_enabled": true,
  "bot_difficulty": "normal",
  "bot_attack_damage": 10,
  "bot_headshot_damage": 20,
  "bot_attack_cooldown": 0.75,
  "bot_detect_range": 1600.0,
  "bot_attack_range": 1300.0,
  "bot_move_speed": 450.0,
  "bot_fire_interval": 0.75,
  "bot_aim_spread": 13.0,
  "max_bots_targeting_one_player": 8,
  "max_bots_shooting_one_player": 3,
  "respawn_invincible_seconds": 2.0,
  "enable_bot_2d_fallback_hit": true,
  "bot_2d_fallback_radius_scale": 0.75,
  "verbose_hitscan_candidate_logs": false,
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
        "spread_deg": 0.8,
        "has_client_position": true,
        "client_x": 100.0,
        "client_y": 0.0,
        "client_z": 0.0
      }
    }
  ],
  "bots": [
    {
      "bot_id": 1,
      "name": "BOT-1",
      "x": -900.0,
      "y": 500.0,
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
      "owner_type": "player",
      "owner_player_id": 1,
      "owner_bot_id": 0,
      "visual_only": true,
      "x": 150.0,
      "y": 0.0,
      "z": 100.0,
      "dir_x": 1.0,
      "dir_y": 0.0,
      "dir_z": 0.0,
      "start_x": 150.0,
      "start_y": 0.0,
      "start_z": 100.0,
      "end_x": 1950.0,
      "end_y": 0.0,
      "end_z": 100.0
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
  "arena": {
    "bounds": {
      "min_x": -5000.0,
      "max_x": 5000.0,
      "min_y": -5000.0,
      "max_y": 5000.0
    }
  },
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
      "x": -900.0,
      "y": 500.0,
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
      "owner_type": "player",
      "owner_player_id": 1,
      "owner_bot_id": 0,
      "visual_only": true,
      "x": 150.0,
      "y": 0.0,
      "z": 100.0,
      "dir_x": 1.0,
      "dir_y": 0.0,
      "dir_z": 0.0,
      "start_x": 150.0,
      "start_y": 0.0,
      "start_z": 100.0,
      "end_x": 1950.0,
      "end_y": 0.0,
      "end_z": 100.0
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
- `hit_group`: shot result hit group such as `head`, `body`, `fallback_body`, `core`, or `miss`.
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
- `x`, `y`, `z`: server player position. X/Y are logical arena coordinates; Z currently mirrors Unreal world height in demo client-position sync.
- `speed` / `effective_speed`: current server movement speed after sprint/ADS rules when present in debug data.
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
- `wander_target_x`, `wander_target_y`: current demo waypoint target used when the bot is not chasing a player.
- `current_waypoint_index`: zero-based index into the fixed demo waypoint list.
- `stuck_timer`: seconds accumulated by the simple stuck detector.
- `body_radius`, `head_radius`: server hit sphere radii for player shots against bots.
- `body_height`, `head_height`: server hit sphere center heights above the bot base position.

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
- `x`, `y`, `z`: current server visual tracer position.
- `dir_x`, `dir_y`, `dir_z`: normalized server logical shot/tracer direction.
- `visual_only`: true for tracer projectiles that do not apply collision damage.
- `start_x`, `start_y`, `start_z`: visual tracer start point, usually near the server shooter muzzle/origin.
- `end_x`, `end_y`, `end_z`: visual tracer end point.

Projectile/tracer records are visual-only for the current PvPvE shooter path. Server damage is applied by hitscan at fire time, not by projectile collision.

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
- In Unreal demo mode, player movement can use accepted `client_x/y/z` pawn positions so `SERVER ECHO` follows the Unreal character and respects Unreal map collision. `client_z` / snapshot player `z` carry the local pawn world height so jump/ramp/platform visuals can stay aligned.
- Browser clients or clients without `has_client_position` fall back to normalized input integration and clamp to the server arena bounds, currently `x=-5000..5000`, `y=-5000..5000` for the demo map.
- Player effective movement speed is 600 walk, 850 sprint, or 400 ADS walk. Sprint overrides ADS if both flags arrive.
- Fire input creates visual-only server tracer records.
- Server tracers use the spread-adjusted `shot_dir_x` / `shot_dir_y` / `shot_dir_z` fields when available.
- Server hitscan damage is applied immediately when a new fire input sequence is processed.
- Player hitscan evaluates all valid bot/player candidates and applies damage to the closest valid hit along the ray.
- Legacy target hitscan is disabled by default with the old target objective.
- Hitscan player damage checks head sphere first for 40 damage, then body sphere for 20 damage.
- Hitscan bot damage checks head sphere first for 40 damage, then body sphere for 20 damage, then optional `fallback_body`.
- Bot 2D fallback is enabled by default for forgiving tests, but uses `bot_2d_fallback_radius_scale = 0.75` and participates in closest-hit selection by ray distance.
- Server bot kills award +1 kill and increment `bot_kills`.
- Server player kills award +1 kill and increment `player_kills`.
- Legacy target kills do not affect primary ranking while targets are disabled.
- Dead players respawn after 8 seconds and receive difficulty-tuned invincibility.
- Dead bots respawn after 8 seconds and are invincible for 1.5 seconds.
- Bots use simple server AI: detect nearby alive non-invincible players, face/chase/aim/shoot based on range and pressure caps, otherwise wander through fixed waypoints inside the smaller demo bot area bounds `x=-1500..1500`, `y=-900..900`.
- Bot attacks are server hitscan gun shots with ammo, reload, low accuracy, and visual tracer projectiles.
- Bot body damage is 10 and bot headshot damage is 20.
- Debug bot difficulty tunes bot shooter AI:
  - `easy`: body/head 10/20, fire interval 1.0s, spread 18 degrees, detect range 1400, attack range 1100, speed 380, max shooting 2.
  - `normal`: body/head 10/20, fire interval 0.75s, spread 13 degrees, detect range 1600, attack range 1300, speed 450, max shooting 3.
  - `hard`: body/head 10/20, fire interval 0.5s, spread 8 degrees, detect range 1900, attack range 1600, speed 550, max shooting 4.
- `debug_room.bots[*]` includes `combat_state`, `fire_block_reason`, `distance_to_target`, `wants_to_shoot`, and `allowed_to_shoot` for bot combat diagnosis.
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
- Demo movement can accept client pawn position for Unreal collision alignment; this is not production movement validation or anti-cheat.
- Bot behavior is simple hitscan shooter AI with no pathfinding, animations, or advanced target selection.
- Health packs are server-side snapshot entities only; there are no local pickup effects, sounds, or imported models yet.
- No lag compensation or advanced hit validation yet.
- Legacy targets/cores are disabled by default and kept only as debug compatibility data.
- No authoritative synchronization with Unreal-placed local targets.
- `debug_restart_match` is a browser/testing utility, not a production match flow.
- No persistence or database.
