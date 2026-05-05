# BattleGrid Project Brief

## Project Name

BattleGrid

## One-line Description

BattleGrid is a top-down arena shooter built with an Unreal Engine C++ client and a custom C++20 authoritative server.

## Portfolio Goal

BattleGrid is designed to show two complementary engineering skills:

- Unreal Engine C++ gameplay programming.
- Real-time C++ server programming with WebSockets, JSON protocol handling, session state, game loop simulation, and snapshot broadcasting.

## Target Roles

- Unreal Game Client Programmer
- Game Server Programmer
- C++ Software Engineer

## Genre

Top-down multiplayer arena shooter.

## Current MVP Shape

The current project is a hybrid local/network demo:

- Unreal provides playable local arena shooter mechanics.
- The C++ server runs an authoritative simulation layer for players, projectiles, targets, and score.
- Unreal renders the server state as ghost actors instead of replacing local gameplay immediately.

This separation keeps the demo playable while making server state visible and debuggable.

## Unreal Client Responsibilities

- Enhanced Input based WASD movement.
- Mouse aim rotation.
- Local projectile spawning.
- Local target damage and destruction.
- Local player HP, hazard damage, death, respawn, score, victory, and restart.
- UMG combat HUD.
- WebSocket connection through a `UGameInstanceSubsystem`.
- JSON message sending and parsing.
- Server player, projectile, and target ghost visualization.
- Server position error display.
- Optional server position correction.

## Custom C++ Server Responsibilities

- Command-line server configuration.
- Logging.
- WebSocket accept/session handling with Boost.Beast.
- JSON protocol dispatch with nlohmann-json.
- Session join state and player ID assignment.
- Fixed Room 1 state.
- Latest player input storage.
- Fixed-rate tick.
- Player movement integration.
- Server projectile spawn, movement, lifetime, and snapshot state.
- Fixed server target HP.
- Projectile-target collision.
- Server score updates.
- Snapshot broadcast to joined sessions.

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
- Standard library threading and containers

Planned deployment and tooling:

- Docker
- GCP Compute Engine
- Bot client
- Benchmark scripts

## Implemented Scope

- Local Unreal arena shooter input and combat loop.
- Local damageable targets.
- Player hazard damage, death, and respawn.
- Local victory and restart.
- UMG HUD.
- WebSocket JSON server.
- Join/input/snapshot protocol.
- Server-side room and player state.
- Server-side projectiles and fixed targets.
- Server-side target HP and score.
- Unreal visualization of server state.

## Not Implemented Yet

- Full server-authoritative replacement of local combat.
- Input replay and reconciliation history.
- Remote player character meshes.
- Server-side player damage.
- Target respawn.
- Multiple rooms.
- Authentication.
- Database persistence.
- Docker/GCP deployment.
- Bot load testing.
- Production hardening.

## Final Portfolio Message

BattleGrid demonstrates a practical path from Unreal local gameplay to a custom authoritative multiplayer architecture. It shows that the client can remain playable while the server simulation is built, visualized, tested, and gradually promoted into the source of truth.
