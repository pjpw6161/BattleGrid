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
  "seq": 1
}
```

The server currently parses and logs input, then acknowledges the sequence. It does not simulate movement yet.

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

## Current Limitations

- `room_id` is fixed to `1`.
- Player IDs are process-local and reset when the server restarts.
- No authentication.
- No room management.
- No game loop integration.
- No server-side player movement simulation.
- No server-side combat messages.
