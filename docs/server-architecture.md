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
- `MatchState`: current match timer, kill goal, game-over flag, winner, and match ID.
- `CombatEvent`: compact recent server event for kill feed and demo feedback.
- `PlayerState`: player identity, nickname, connected flag, latest input, position, speed, HP, score, and processed fire sequence.
- `PlayerInput`: latest input packet fields.
- `BotState`: server-controlled PvE combatant state, movement target, HP, respawn, invincibility, ammo, reload, fire cooldown, attack range, and accuracy.
- `HealthPackState`: server pickup state, position, active flag, heal amount, pickup radius, and respawn timer.
- `ProjectileState`: projectile ID, owner type, owner player/bot ID, position, direction, speed, age, lifetime, damage, radius, active flag, and visual-only tracer flag.
- `TargetState`: legacy/debug target state kept for compatibility; disabled by default in the main kill race flow.

## Startup Flow

1. `main` parses `ServerConfig`.
2. `GameServer::Run` logs startup settings.
3. `WebSocketServer` binds and listens on the configured host/port.
4. `RoomManager` creates fixed Room 1.
5. `GameRoom` initializes fixed server bots and server health packs. Legacy targets are only initialized when `bTargetsEnabled=true`.
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

Arena bounds:

```text
x = -1800..1800
y = -1200..1200
```

These bounds are used by player movement, bot movement, bot wander target generation, and projectile expiry.

Player spawn points:

1. P1 `(-1200, 0)`
2. P2 `(1200, 0)`
3. P3 `(0, 900)`
4. P4 `(0, -900)`

Players spawn from this list by player ID on join, respawn, and debug match restart.

Legacy target/core positions:

1. CORE-1 `(0, 0)`
2. CORE-2 `(700, 500)`
3. CORE-3 `(700, -500)`
4. CORE-4 `(-700, 500)`
5. CORE-5 `(-700, -500)`

The C++ model remains `TargetState`, but the core/target objective is no longer the main game mode. `GameRoom::bTargetsEnabled` defaults to `false`, so these entities are debug-only unless explicitly re-enabled in code. Each legacy target starts with:

- `hp = 100`
- `maxHp = 100`
- `radius = 80`
- `alive = true`

Default bots:

1. BOT-1 `(-300, 300)`
2. BOT-2 `(-300, -300)`
3. BOT-3 `(300, 300)`
4. BOT-4 `(300, -300)`
5. BOT-5 `(1000, 0)`
6. BOT-6 `(-1000, 0)`
7. BOT-7 `(0, 700)`
8. BOT-8 `(0, -700)`

Each bot starts with:

- `hp = 100`
- `maxHp = 100`
- `speed = 500`
- `alive = true`
- `invincible = false`

Health pack spawn points:

1. `(-1300, 700)`
2. `(1300, 700)`
3. `(0, -1100)`
4. `(-1300, -700)`
5. `(1300, -700)`
6. `(0, 1100)`

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
6. Legacy projectile-target collision is skipped by default and only used when target debug gameplay is enabled.
7. Inactive projectiles are removed.
8. Bot respawns, weapon timers, movement, and shooter AI are updated.
9. Health pack respawns and player pickups are processed.
10. Match time is decremented and the win condition is checked.
11. The default room snapshot is built.
12. Snapshot JSON is broadcast to joined active sessions.

If the match is already over, `GameRoom::Tick` stops new combat and score changes. Snapshots continue so clients can display the final scoreboard.

Movement is intentionally simple:

- Normalize move vector if length is greater than 1.
- Apply `speed * deltaSeconds`.
- Clamp x to `-1800..1800`.
- Clamp y to `-1200..1200`.

## Projectile, Hitscan, And Bot Simulation

Server projectiles are visual tracer records. They spawn from the owning player or bot server fire origin, offset slightly forward along the shot direction, and include explicit `start_*` / `end_*` points for client visualization. With hitscan combat enabled, these projectiles do not apply gameplay damage.

Projectiles:

- Store owner type, player owner ID, bot owner ID, current position, direction, start point, end point, and visual-only flag.
- Move only as short-lived visual state for snapshot interpolation.
- Track age.
- Expire after lifetime.
- Collide with alive legacy targets only if target debug gameplay and projectile collision damage are explicitly enabled.

On server hitscan fire:

1. A visual-only tracer is spawned for snapshot visualization.
2. A ray is cast from the shooter's server position.
3. Alive non-invincible bots and other alive non-invincible players are tested. Legacy targets are tested only when `bTargetsEnabled=true`.
4. Head spheres are checked before body spheres for bots and players.
5. The closest hit receives damage immediately.

Kill race scoring:

- Server bot killed: shooter gains +1 kill and +1 `bot_kills`.
- Server player killed: shooter gains +1 kill and +1 `player_kills`.
- Primary score is `bot_kills + player_kills`.
- Legacy target kills can still increment `target_kills` in debug mode, but they do not affect primary score while targets are disabled.

The current combat tuning values are centralized in `GameRoom.cpp` for demo readability:

- body damage: `20`
- headshot damage: `40`
- hitscan range: `3000`
- bot body/head damage: `10/20`
- bot max HP: `100`
- bot respawn: `8s`
- bot invincibility after respawn: `1.5s`
- bot kill score: `+1`
- player kill score: `+1`
- legacy target/core kill score: `0`

`debug_room` includes these values so the browser test page can show a compact combat tuning summary.

## Match State And Scoreboard

`GameRoom` owns one `MatchState` for Room 1.

Defaults:

- `matchDurationSeconds = 300`
- `timeRemainingSeconds = 300`
- `targetScore = 20` kill goal
- `state = in_progress`
- `matchId = 1`

The match ends when:

- any connected player reaches the kill goal
- or `timeRemainingSeconds` reaches zero

For demo iteration, `GameRoom` owns `bAutoEndMatchByTimer`. When disabled through `debug_set_match_timer`, `timeRemainingSeconds` may reach zero but the timer alone will not set `game_over`. Kill-goal wins still end the match.

Winner selection:

1. Highest score.
2. Higher player kills.
3. Higher bot kills.
4. Fewer deaths.
5. Lower player ID.

Snapshots include a `match` object and a sorted `scoreboard` array. The scoreboard is sorted by the same tie breaker rules, so clients can show a concise top-player list without recomputing rank order.

`debug_restart_match` is available for browser and demo testing. It resets match state, player scores and combat counters, projectiles, legacy targets, bots, and health packs, then increments `matchId`. It is not a production rematch/lobby system.

Debug restart preserves current demo settings such as bot difficulty, bot attacks enabled, and timer auto-end.

## Combat Events

`GameRoom` keeps a bounded `recentEvents` queue of the last 20 `CombatEvent` records.

Each event stores:

- event ID
- event type
- display message
- compact `short_message` for shot result HUD/browser text
- server room time
- actor player ID
- target player ID
- bot ID
- target ID
- health pack ID
- headshot flag

Events are generated when:

- a player hits or destroys a legacy target only when target debug gameplay is enabled
- a player kills a bot
- a player kills another player
- a bot kills a player
- a player respawns
- a player picks up a health pack
- a match ends
- a match restarts
- a processed fire input hits or misses

Snapshots and `debug_room` include the recent `events` array. Unreal deduplicates by `event_id` and displays the last few messages in the existing HUD as a simple kill feed.

Shot result events use compact text such as `SERVER HIT BOT-3 -20`, `SERVER HEADSHOT BOT-3 -40`, `SERVER HIT PLAYER -20`, `SERVER MISS`, `BOT HIT YOU -10`, or `KILLED BY BOT-3`. Unreal displays these separately from the persistent event feed so automatic fire does not bury kill, death, pickup, and match events.

## Bot AI

`GameRoom::InitializeDefaultBots` creates eight fixed bots in Room 1.

Each tick, bot logic is intentionally simple:

1. Dead bots count down an 8 second respawn timer.
2. Respawned bots return with full HP and 1.5 seconds of invincibility.
3. Alive bots find the nearest alive non-invincible player within the configured detect range.
4. If a player is found, the bot faces that player and moves toward the preferred combat range.
5. In attack range, the bot fires low-accuracy hitscan shots when bot attacks are enabled and its weapon can fire.
6. Bot weapons have 30-round magazines, infinite reserve ammo, 2.5 second reloads, and difficulty-controlled fire interval/spread.
7. Bot body hits deal 10 damage and bot headshots deal 20 damage.
8. If no player is found, the bot wanders toward deterministic arena points.

`GameRoom::ApplyBotDifficulty` supports the current debug/demo profiles:

| Difficulty | Body / Head | Fire Interval | Spread | Detect Range | Attack Range | Speed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Easy | 10 / 20 | 0.75s | 18 deg | 1000 | 1200 | 400 |
| Normal | 10 / 20 | 0.5s | 12 deg | 1500 | 1400 | 500 |
| Hard | 10 / 20 | 0.35s | 7 deg | 1800 | 1600 | 600 |

`debug_set_bot_attacks` can disable bot shooting while leaving bot movement, chasing, respawn, snapshots, and ghost visualization active.

Safe demo mode applies the easy profile, disables bot attacks, disables timer-based match ending, resets players/bots/health packs, clears projectiles, resets legacy targets internally if present, and restarts the match in `in_progress`. This is the recommended startup state for portfolio recording and local combat tests.

There is no navmesh, cover, humanoid animation, weapon socket, or advanced target selection yet.

Bot hitscan shots can kill players. Player death starts the same 8 second server respawn timer used by player-vs-player hitscan kills.

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

- arena bounds and fixed layout spawn lists
- match
- scoreboard
- events
- players
- bots
- health_packs
- projectiles
- targets as an empty legacy/debug array by default

Only joined sessions receive snapshots. `WebSocketServer` keeps weak pointers to active sessions, removes expired sessions, and broadcasts the same compact JSON string to each joined session.

## Session Send Queue

`Session::SendText` posts onto the WebSocket executor and pushes messages into a `std::deque`. Only one `async_write` is active at a time. When a write completes, the next queued message is written.

This avoids overlapping writes when protocol responses and server snapshot broadcasts happen close together.

## Debug Room

`debug_room` returns Room 1 state:

- match state
- scoreboard
- recent combat events
- player count
- projectile count
- legacy target debug count
- bot count
- health pack count
- active health pack count
- players and latest input
- bots and HP/alive state
- health packs and active/respawn state
- active projectiles
- legacy target debug state when enabled

This is for browser testing.

`GameRoom::ToDebugJson` also includes the full fixed arena layout for internal/debug consumers. The current snapshot includes the same fixed layout so browser and Unreal tests can confirm the active limited arena without relying on map assets.

## Current Limitations

- Fixed single room.
- Process-local player IDs.
- No authentication.
- No database.
- No real matchmaking.
- No binary protocol.
- Bot AI is direct and deterministic for debugging, with hitscan shooting but no pathfinding, cover, or animation state.
- Health packs are server snapshot entities only; no pickup effects, sounds, or production meshes yet.
- Legacy targets/cores are disabled by default.
- No deployment automation yet.
