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
- `fire`: true when a fire input occurred on this packet.
- `reload`: true when a reload input occurred on this packet.
- `ads`: true while the client is aiming down sights.
- `sprint`: true while the client is sprinting.
- `jump`: true while the client is jumping or falling.
- `ammo`: client-side magazine ammo count after local weapon processing.
- `spread_deg`: client-side spread value used for this shot/input.

`aim_x` / `aim_y` represent the intentional camera aim direction. `shot_dir_x` / `shot_dir_y` represent the actual shot direction after spread. If `shot_dir_x` and `shot_dir_y` are missing, the server falls back to `aim_x` and `aim_y`.

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
  "players": [],
  "projectiles": [],
  "targets": []
}
```

`debug_room` is intended for browser testing and inspection.

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

### `snapshot`

Snapshots are broadcast to joined sessions at the configured tick rate.

```json
{
  "type": "snapshot",
  "tick": 30,
  "room_id": 1,
  "players": [
    {
      "player_id": 1,
      "nickname": "player1",
      "x": 100.0,
      "y": 0.0,
      "hp": 100,
      "score": 0,
      "last_seq": 10
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

Player fields:

- `player_id`: server player ID.
- `nickname`: player nickname from join.
- `x`, `y`: server logical 2D position.
- `hp`: server player HP placeholder.
- `score`: server score from destroyed server targets.
- `last_seq`: latest input sequence stored for the player.

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
- Server projectiles use the spread-adjusted `shot_dir_x` / `shot_dir_y` fields when available.
- Server projectiles move, expire, and collide with fixed server targets.
- Server target HP decreases on projectile hit.
- The projectile owner gains server score when a server target dies.

## Current Limitations

- No binary protocol.
- No authentication.
- No real multiple-room support.
- No server-side player damage.
- No authoritative server hitscan combat yet.
- No target respawn.
- No authoritative synchronization with Unreal-placed local targets.
- No persistence or database.
