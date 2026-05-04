# BattleGrid WebSocket Protocol

BattleGrid currently uses a small JSON-over-WebSocket protocol for local server testing.

## Transport

- WebSocket endpoint: `ws://127.0.0.1:7777`
- Messages are UTF-8 JSON text frames.
- Binary messages are not part of the protocol yet.

## Ping

Client:

```json
{ "type": "ping" }
```

Server:

```json
{ "type": "pong" }
```

## Join

Client:

```json
{ "type": "join", "nickname": "player1" }
```

Server:

```json
{
  "type": "join_ok",
  "player_id": 1,
  "room_id": 1,
  "nickname": "player1"
}
```

If `nickname` is missing, not a string, or empty, the server uses `anonymous`.
On the first `join` for a WebSocket session, the server allocates a `player_id` and creates a `PlayerState` in Room 1. Repeated `join` messages on the same session return the existing player state identity.

## Input

Client:

```json
{
  "type": "input",
  "seq": 1,
  "player_id": 1,
  "move_x": 1.0,
  "move_y": 0.0,
  "aim_x": 0.7,
  "aim_y": 0.2,
  "fire": false
}
```

Fields:

- `seq`: client input sequence number.
- `player_id`: player ID assigned by `join_ok`.
- `move_x`: local right movement axis.
- `move_y`: local forward movement axis.
- `aim_x`: normalized aim direction X.
- `aim_y`: normalized aim direction Y.
- `fire`: whether a fire input occurred on this input packet.

Server:

```json
{
  "type": "input_ack",
  "seq": 1,
  "player_id": 1
}
```

The server requires the session to be joined before accepting input. `player_id` must match the joined session. Accepted input updates the player's latest `PlayerInput` in Room 1, then acknowledges the sequence. The fixed-rate server tick uses the latest movement axes to calculate authoritative `x` / `y` positions.

## Snapshot

Server broadcast:

```json
{
  "type": "snapshot",
  "tick": 30,
  "room_id": 1,
  "players": [
    {
      "player_id": 1,
      "nickname": "player1",
      "x": 600.0,
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
      "x": 650.0,
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

Snapshots are sent to joined WebSocket sessions at the configured server tick rate. Current movement simulation is intentionally simple: latest input is treated as a continuous movement vector, normalized if its length is greater than 1, multiplied by player speed, and clamped to a `-2000..2000` arena on both axes.

`projectiles` contains server-authoritative projectile visuals spawned from accepted `input` messages where `fire` is `true`. Projectile fields:

- `projectile_id`: server-local projectile ID.
- `owner_player_id`: player that fired the projectile.
- `x`, `y`: server 2D arena position.
- `dir_x`, `dir_y`: normalized projectile movement direction.

Server projectiles currently move, expire after a short lifetime, leave the snapshot when inactive, and apply damage only to fixed server targets.

`targets` contains fixed server-authoritative targets for the current room. Target fields:

- `target_id`: fixed target ID.
- `x`, `y`: server 2D arena position.
- `hp`, `max_hp`: target health.
- `alive`: whether the target can still be damaged.

Server projectiles collide with alive server targets. On hit, the projectile becomes inactive and the target loses projectile damage. When target HP reaches `0`, `alive` becomes `false` and the projectile owner gains `1` server score. Server targets are fixed test targets and are not synchronized with Unreal-placed local targets yet.

## Debug Room

Client:

```json
{ "type": "debug_room" }
```

Server:

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
      "x": 600.0,
      "y": 0.0,
      "speed": 600.0,
      "hp": 100,
      "score": 0,
      "latest_input": {
        "seq": 1,
        "move_x": 1.0,
        "move_y": 0.0,
        "aim_x": 0.7,
        "aim_y": 0.2,
        "fire": false
      }
    }
  ],
  "projectiles": [
    {
      "projectile_id": 1,
      "owner_player_id": 1,
      "x": 650.0,
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

`debug_room` is for local browser testing only. It returns the current fixed room state and each player's latest stored input.

## Error

Invalid JSON:

```json
{
  "type": "error",
  "message": "invalid json"
}
```

Unknown or missing message type:

```json
{
  "type": "error",
  "message": "unknown message type"
}
```

Input before `join`:

```json
{
  "type": "error",
  "message": "not joined"
}
```

Input with a mismatched `player_id`:

```json
{
  "type": "error",
  "message": "player_id mismatch"
}
```

## Current Limitations

- `room_id` is fixed to `1`.
- Player IDs are process-local and reset when the server restarts.
- No authentication.
- No multiple-room management.
- Movement simulation only supports simple 2D position integration from latest input.
- Server target simulation uses fixed room targets and does not synchronize with Unreal-placed target actors yet.
- Server projectile simulation covers spawn, movement, lifetime, target collision, target HP, and server score.
- Unreal renders server player, projectile, and target snapshots as separate ghost actors; local gameplay remains offline.
- No server-side player damage, target respawn, or combat messages.
