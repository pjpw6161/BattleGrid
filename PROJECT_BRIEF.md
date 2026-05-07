# BattleGrid Project Brief

## Project Name

BattleGrid

## One-Line Description

BattleGrid is a third-person PvPvE arena shooter prototype built with an Unreal Engine C++ client and a custom C++20 WebSocket authoritative server.

## Portfolio Goal

BattleGrid is designed to show two complementary engineering skills:

- Unreal Engine C++ gameplay programming.
- Real-time C++ server programming with WebSockets, JSON protocol handling, authoritative room simulation, combat state, snapshots, Docker deployment, and GCP operation flow.

## Target Roles

- Unreal Game Client Programmer
- Gameplay Programmer
- Game Server Programmer
- C++ Software Engineer

## Genre

Third-person PvPvE arena shooter.

## Current Demo Shape

The current project is a hybrid local/server portfolio prototype:

- Unreal provides playable third-person movement, local feedback, weapon state, HUD, and prototype visuals.
- The C++ server owns the PvPvE state layer: players, bots, cores, health packs, hitscan combat, score, match state, scoreboard, and combat events.
- Unreal renders server snapshots through ghost actors so authoritative state remains visible and debuggable.

The server-authoritative layer is the main demo focus. The local Unreal gameplay layer remains available for offline fallback and incremental feature development.

## Unreal Client Responsibilities

- Third-person character movement.
- ADS, sprint, jump, and camera-relative controls.
- Local weapon component with ammo, reload, fire rate, and spread values.
- Local projectile and hazard feedback for prototype/offline testing.
- UMG HUD with server-centric status.
- WebSocket connection through a `UGameInstanceSubsystem`.
- JSON message sending and snapshot parsing.
- Server player, projectile, core, bot, and health pack ghost visualization.
- Server shot result display and combat event feed.
- Server scoreboard overlay.
- Local/remote server profile selection.

## Custom C++ Server Responsibilities

- Command-line server configuration.
- Logging.
- WebSocket accept/session handling with Boost.Beast.
- JSON protocol dispatch with nlohmann-json.
- Session join state and player ID assignment.
- Fixed Room 1 state.
- Latest player input storage.
- Fixed-rate server tick.
- Player movement integration.
- Server projectile/tracer spawn and lifetime.
- Server hitscan combat with body/head damage.
- Server bots with simple movement, attack, death, respawn, and invincibility.
- Fixed server cores with HP.
- Server health packs with pickup and respawn.
- Server score, match timer, winner, and scoreboard.
- Recent combat event queue and snapshot broadcast.
- Safe demo/debug controls.

## Technology Stack

Unreal client:

- Unreal Engine C++
- Gameplay Framework
- Enhanced Input
- UMG
- WebSockets
- Json / JsonUtilities

Server:

- C++20
- CMake
- Boost.Asio
- Boost.Beast
- nlohmann-json
- Standard library containers and synchronization

Deployment and tooling:

- Docker Compose
- GCP Compute Engine deployment flow
- Browser WebSocket protocol test page

## Implemented Scope

- Third-person Unreal controls and local prototype combat feedback.
- Local weapon state with ammo, reload, automatic fire, and spread configuration.
- WebSocket JSON server and protocol.
- Join/input/snapshot/debug protocol messages.
- Server-side room, players, bots, cores, health packs, projectiles/tracers, match state, scoreboard, and combat events.
- Server hitscan combat against bots, cores, and players.
- Bot kill/respawn and score updates.
- Safe demo mode for stable recording.
- Browser protocol test page with compact summaries and server shot tests.
- Unreal visualization of server state through ghost actors.
- Server-centric HUD and Tab scoreboard.
- Docker local server setup and GCP deployment documentation.

## Not Implemented Yet

- Production-ready matchmaking or lobby flow.
- Database persistence.
- Advanced lag compensation, rollback, or anti-cheat.
- Final character, weapon, bot, pickup, audio, VFX, or arena art assets.
- Dedicated polished UMG scoreboard/kill-feed widgets.
- Automated cloud deployment pipeline.
- Large-scale load testing.

## Final Portfolio Message

BattleGrid demonstrates a practical path from Unreal local gameplay to custom server-authoritative multiplayer architecture. It shows gameplay systems, C++ server simulation, WebSocket protocol design, snapshot visualization, Docker/GCP deployment workflow, and debugging tools in a single cohesive prototype.
