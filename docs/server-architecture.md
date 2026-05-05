# BattleGrid Server Architecture

## Current Scope

The BattleGrid server is a C++20 CMake application that accepts WebSocket JSON clients and simulates a small fixed arena room. It is not production-ready networking. It is a portfolio server used to demonstrate sessions, protocol dispatch, fixed-rate tick, authoritative state, and snapshot broadcast.

## Build Units

- `Logger`: simple console logging with a `[BattleGridServer]` prefix.
- `ServerConfig`: parses `--host`, `--port`, `--tick-rate`, and `--help`.
- `GameServer`: owns startup/shutdown and starts the WebSocket server.
- `WebSocketServer`: owns Boost.Asio `io_context`, TCP acceptor, active sessions, RoomManager, player ID allocation, and the fixed-rate timer.
- `Session`: owns one WebSocket stream, join state, session player identity, and async send queue.
- `MessageDispatcher`: handles parsed JSON messages and mutates session/room state.
- `RoomManager`: owns the fixed default Room 1.
- `GameRoom`: thread-safe state container and simulation for Room 1.
- `PlayerState`: player identity, nickname, connected flag, latest input, position, speed, HP, score, and processed fire sequence.
- `PlayerInput`: latest input packet fields.
- `ProjectileState`: projectile ID, owner, position, direction, speed, age, lifetime, damage, radius, and active flag.
- `TargetState`: fixed server target ID, position, HP, max HP, collision radius, and alive flag.

## Startup Flow

1. `main` parses `ServerConfig`.
2. `GameServer::Run` logs startup settings.
3. `WebSocketServer` binds and listens on the configured host/port.
4. `RoomManager` creates fixed Room 1.
5. `GameRoom` initializes fixed server targets.
6. The accept loop and tick timer start.
7. `io_context.run()` keeps the process alive.

## Session Flow

1. A TCP connection is accepted.
2. `Session` performs the WebSocket handshake.
3. The client sends JSON messages.
4. `Session` parses each text frame through `MessageDispatcher`.
5. Dispatcher returns a JSON response.
6. `Session` writes responses through its send queue.

When a joined session closes, the player is removed from Room 1 and the disconnect is logged.

## Room Model

The server currently has one fixed room:

```text
room_id = 1
```

Default targets:

1. `(600, 0)`
2. `(900, 300)`
3. `(900, -300)`
4. `(1200, 0)`
5. `(1500, 400)`

Each target starts with:

- `hp = 100`
- `maxHp = 100`
- `radius = 80`
- `alive = true`

## Join And Input

On `join`:

1. The server allocates a process-local player ID.
2. The player is added to Room 1.
3. Session state stores `playerId`, `roomId`, nickname, and joined flag.
4. The server returns `join_ok`.

On `input`:

1. Dispatcher verifies the session is joined.
2. Dispatcher verifies `player_id` matches the session player ID.
3. Input fields are safely read with defaults.
4. The latest input is stored in `PlayerState`.
5. The server returns `input_ack`.

## Game Tick

`WebSocketServer` uses a Boost.Asio `steady_timer` at:

```text
1.0 / tickRate
```

Each tick:

1. Increments `tickNumber`.
2. Calls `RoomManager::TickAll`.
3. `GameRoom::Tick` updates connected players from latest input.
4. New fire input spawns server projectiles.
5. Active projectiles move.
6. Projectile-target collision is checked.
7. Inactive projectiles are removed.
8. The default room snapshot is built.
9. Snapshot JSON is broadcast to joined active sessions.

Movement is intentionally simple:

- Normalize move vector if length is greater than 1.
- Apply `speed * deltaSeconds`.
- Clamp x/y to `-2000..2000`.

## Projectile And Target Simulation

Server projectiles spawn from the owning player's current position, offset slightly forward along the aim direction.

Projectiles:

- Move at fixed speed.
- Track age.
- Expire after lifetime.
- Deactivate outside the projectile arena.
- Collide with alive targets using radius overlap.

On target hit:

1. Target takes projectile damage.
2. Projectile becomes inactive.
3. If target HP reaches 0, target becomes dead.
4. Projectile owner gains 1 server score.

The server does not damage players yet.

## Snapshot Broadcast

Snapshots include:

- players
- projectiles
- targets

Only joined sessions receive snapshots. `WebSocketServer` keeps weak pointers to active sessions, removes expired sessions, and broadcasts the same compact JSON string to each joined session.

## Session Send Queue

`Session::SendText` posts onto the WebSocket executor and pushes messages into a `std::deque`. Only one `async_write` is active at a time. When a write completes, the next queued message is written.

This avoids overlapping writes when protocol responses and server snapshot broadcasts happen close together.

## Debug Room

`debug_room` returns Room 1 state:

- player count
- projectile count
- target count
- players and latest input
- active projectiles
- targets and HP

This is for browser testing.

## Current Limitations

- Fixed single room.
- Process-local player IDs.
- No authentication.
- No database.
- No real matchmaking.
- No binary protocol.
- No server-side player damage.
- No target respawn.
- No deployment automation yet.
