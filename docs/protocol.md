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
  "target_count": 5,
  "bot_count": 8,
  "health_pack_count": 3,
  "active_health_pack_count": 3,
  "match": {},
  "scoreboard": [],
  "players": [],
  "bots": [],
  "health_packs": [],
  "projectiles": [],
  "targets": []
}
```

`debug_room` is intended for browser testing and inspection.

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
- The server resets match timer, winner, scores, player combat counters, projectiles, targets, bots, and health packs.
- Existing joined players remain joined and are reset to full HP.

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
  "target_count": 5,
  "bot_count": 8,
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
      "kills": 3,
      "deaths": 1,
      "bot_kills": 2,
      "player_kills": 1,
      "target_kills": 0,
      "hp": 80,
      "alive": true
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
      "kills": 3,
      "deaths": 1,
      "bot_kills": 2,
      "player_kills": 1,
      "target_kills": 0,
      "hp": 80,
      "alive": true
    }
  ],
  "players": [
    {
      "player_id": 1,
      "nickname": "player1",
      "x": 100.0,
      "y": 0.0,
      "hp": 100,
      "max_hp": 100,
      "alive": true,
      "invincible": false,
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
  "targets": [
    {
      "target_id": 1,
      "x": 600.0,
      "y": 0.0,
      "hp": 80,
      "max_hp": 100,
      "alive": true
    }
  ]
}
```

Match fields:

- `state`: current match state, currently `in_progress` or `game_over`.
- `time_left`: seconds remaining in the match.
- `duration`: configured match duration in seconds.
- `target_score`: score needed for immediate win.
- `game_over`: true after score or timer win condition is reached.
- `winner_player_id`: server player ID of the winner, or `0` while in progress.
- `winner_nickname`: winner display name, or empty while in progress.
- `match_id`: process-local match counter.

Scoreboard fields:

- `player_id`, `nickname`: player identity.
- `score`: authoritative server match score.
- `kills`: player plus bot kills.
- `deaths`: player deaths.
- `bot_kills`: bot kills worth +1 score.
- `player_kills`: player kills worth +2 score.
- `target_kills`: destroyed server targets worth +1 score.
- `hp`: current server player HP.
- `alive`: whether the player is alive.

Player fields:

- `player_id`: server player ID.
- `nickname`: player nickname from join.
- `x`, `y`: server logical 2D position.
- `hp`: server player HP placeholder.
- `max_hp`: server player maximum HP.
- `alive`: whether the player can move and be damaged.
- `invincible`: true during the short post-respawn protection window.
- `score`: server score from destroyed server targets.
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
- `owner_player_id`: player that fired the projectile.
- `x`, `y`: server logical 2D projectile position.
- `dir_x`, `dir_y`: normalized server logical movement direction.

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
- Hitscan target damage uses 20 damage and fixed server target spheres.
- Hitscan player damage checks head sphere first for 40 damage, then body sphere for 20 damage.
- Hitscan bot damage checks head sphere first for 40 damage, then body sphere for 20 damage.
- Server target kills award +1 score; server player kills award +2 score.
- Server bot kills award +1 score and increment `bot_kills`.
- Dead players respawn after 8 seconds and are invincible for 1.5 seconds.
- Dead bots respawn after 8 seconds and are invincible for 1.5 seconds.
- Bots use simple server AI: move toward the nearest alive non-invincible player inside detect range, otherwise wander.
- Bot attack v1 applies direct body damage when in range; bot projectiles are not implemented yet.
- Three server health packs spawn from predefined points.
- Alive players below max HP pick up active health packs by overlapping their pickup radius.
- Health packs heal +35 HP, do not overheal above max HP, disappear on pickup, and respawn after 15 seconds.
- Bots ignore health packs for now.
- Match duration is 300 seconds and target score is 20.
- The match ends when any connected player reaches target score or when the timer reaches zero.
- Winner tie breakers are highest score, higher player kills, higher bot kills, fewer deaths, then lower player ID.
- After game over, snapshots continue, but new combat score changes are stopped.

## Current Limitations

- No binary protocol.
- No authentication.
- No real multiple-room support.
- Bot behavior is simple direct-damage AI with no pathfinding, animations, or projectile visualization.
- Health packs are server-side snapshot entities only; there are no local pickup effects, sounds, or imported models yet.
- No lag compensation or advanced hit validation yet.
- No target respawn.
- No authoritative synchronization with Unreal-placed local targets.
- `debug_restart_match` is a browser/testing utility, not a production match flow.
- No persistence or database.
