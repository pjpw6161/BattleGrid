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
- `MatchState`: current match timer, target score, game-over flag, winner, and match ID.
- `PlayerState`: player identity, nickname, connected flag, latest input, position, speed, HP, score, and processed fire sequence.
- `PlayerInput`: latest input packet fields.
- `BotState`: server-controlled PvE combatant state, movement target, HP, respawn, invincibility, and simple attack timers.
- `HealthPackState`: server pickup state, position, active flag, heal amount, pickup radius, and respawn timer.
- `ProjectileState`: projectile ID, owner, position, direction, speed, age, lifetime, damage, radius, and active flag.
- `TargetState`: fixed server target ID, position, HP, max HP, collision radius, and alive flag.

## Startup Flow

1. `main` parses `ServerConfig`.
2. `GameServer::Run` logs startup settings.
3. `WebSocketServer` binds and listens on the configured host/port.
4. `RoomManager` creates fixed Room 1.
5. `GameRoom` initializes fixed server targets, fixed server bots, and server health packs.
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

Default bots:

1. `(300, 300)`
2. `(300, -300)`
3. `(700, 500)`
4. `(700, -500)`
5. `(1100, 500)`
6. `(1100, -500)`
7. `(1500, 200)`
8. `(1500, -200)`

Each bot starts with:

- `hp = 100`
- `maxHp = 100`
- `speed = 500`
- `alive = true`
- `invincible = false`

Health pack spawn points:

1. `(-600, 0)`
2. `(-300, 500)`
3. `(-300, -500)`
4. `(300, 700)`
5. `(300, -700)`
6. `(800, 700)`
7. `(800, -700)`
8. `(1300, 0)`

The room starts with three active health packs at spawn points 1, 3, and 5.

Each health pack starts with:

- `healAmount = 35`
- `pickupRadius = 90`
- `respawnDelaySeconds = 15`
- `active = true`

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
6. Projectile-target collision is checked only when hitscan damage is disabled.
7. Inactive projectiles are removed.
8. Bot respawns and simple bot AI are updated.
9. Health pack respawns and player pickups are processed.
10. Match time is decremented and the win condition is checked.
11. The default room snapshot is built.
12. Snapshot JSON is broadcast to joined active sessions.

If the match is already over, `GameRoom::Tick` stops new combat and score changes. Snapshots continue so clients can display the final scoreboard.

Movement is intentionally simple:

- Normalize move vector if length is greater than 1.
- Apply `speed * deltaSeconds`.
- Clamp x/y to `-2000..2000`.

## Projectile, Hitscan, Target, And Bot Simulation

Server projectiles spawn from the owning player's current position, offset slightly forward along the aim direction. With hitscan combat enabled, these projectiles are visual tracers and do not also apply target damage.

Projectiles:

- Move at fixed speed.
- Track age.
- Expire after lifetime.
- Deactivate outside the projectile arena.
- Collide with alive targets using radius overlap only when projectile collision damage is enabled.

On server hitscan fire:

1. A tracer projectile is spawned for snapshot visualization.
2. A ray is cast from the shooter's server position.
3. Alive targets, alive non-invincible bots, and other alive non-invincible players are tested.
4. Head spheres are checked before body spheres for bots and players.
5. The closest hit receives damage immediately.

Scoring:

- Server target destroyed: shooter gains +1 score and +1 target kill.
- Server bot killed: shooter gains +1 score, +1 kill, and +1 bot kill.
- Server player killed: shooter gains +2 score, +1 kill, and +1 player kill.

## Match State And Scoreboard

`GameRoom` owns one `MatchState` for Room 1.

Defaults:

- `matchDurationSeconds = 300`
- `timeRemainingSeconds = 300`
- `targetScore = 20`
- `state = in_progress`
- `matchId = 1`

The match ends when:

- any connected player reaches `targetScore`
- or `timeRemainingSeconds` reaches zero

Winner selection:

1. Highest score.
2. Higher player kills.
3. Higher bot kills.
4. Fewer deaths.
5. Lower player ID.

Snapshots include a `match` object and a sorted `scoreboard` array. The scoreboard is sorted by the same tie breaker rules, so clients can show a concise top-player list without recomputing rank order.

`debug_restart_match` is available for browser and demo testing. It resets match state, player scores and combat counters, projectiles, targets, bots, and health packs, then increments `matchId`. It is not a production rematch/lobby system.

## Bot AI

`GameRoom::InitializeDefaultBots` creates eight fixed bots in Room 1.

Each tick, bot logic is intentionally simple:

1. Dead bots count down an 8 second respawn timer.
2. Respawned bots return with full HP and 1.5 seconds of invincibility.
3. Alive bots find the nearest alive non-invincible player within 1500 units.
4. If a player is found, the bot moves toward that player until it is within 900 units.
5. In attack range, the bot applies 20 direct body damage once per second.
6. If no player is found, the bot wanders toward deterministic arena points.

There is no navmesh, pathfinding, projectile attack, animation, or bot score yet.

Bot attacks can kill players. Player death starts the same 8 second server respawn timer used by player-vs-player hitscan kills.

## Health Packs

`GameRoom::InitializeDefaultHealthPacks` creates three server-owned health packs from predefined spawn points.

Each tick:

1. Inactive health packs count down their respawn timer.
2. When the timer reaches zero, the health pack respawns at a deterministic spawn point.
3. Active health packs check alive players only.
4. Full-health players do not consume health packs.
5. Damaged players within pickup radius heal by 35 HP, clamped to max HP.
6. Picked health packs deactivate and start a 15 second respawn timer.

Bots ignore health packs in this step.

## Snapshot Broadcast

Snapshots include:

- match
- scoreboard
- players
- bots
- health_packs
- projectiles
- targets

Only joined sessions receive snapshots. `WebSocketServer` keeps weak pointers to active sessions, removes expired sessions, and broadcasts the same compact JSON string to each joined session.

## Session Send Queue

`Session::SendText` posts onto the WebSocket executor and pushes messages into a `std::deque`. Only one `async_write` is active at a time. When a write completes, the next queued message is written.

This avoids overlapping writes when protocol responses and server snapshot broadcasts happen close together.

## Debug Room

`debug_room` returns Room 1 state:

- match state
- scoreboard
- player count
- projectile count
- target count
- bot count
- health pack count
- active health pack count
- players and latest input
- bots and HP/alive state
- health packs and active/respawn state
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
- Bot AI is direct and deterministic for debugging, with no pathfinding or projectile attacks.
- Health packs are server snapshot entities only; no pickup effects, sounds, or production meshes yet.
- No target respawn.
- No deployment automation yet.
