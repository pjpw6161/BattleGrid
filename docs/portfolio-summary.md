# BattleGrid Portfolio Summary

## Project One-Liner

BattleGrid is a third-person PvPvE arena shooter prototype built with an Unreal Engine C++ client and a custom C++20 WebSocket authoritative server.

## Tech Stack

- Unreal Engine C++
- Enhanced Input
- UMG HUD
- Unreal WebSockets / JSON
- C++20
- CMake
- Boost.Asio / Boost.Beast
- nlohmann-json
- Docker Compose
- GCP Compute Engine deployment flow

## What I Implemented

- Third-person shooter controls with movement, ADS, sprint, jump, ammo, reload, and automatic fire.
- Local Unreal prototype gameplay for movement, projectiles, hazards, debug targets, HP, score, and restart.
- Custom C++20 WebSocket server with sessions, join state, room state, player input, fixed tick, and snapshot broadcast.
- Server-owned PvPvE state: players, humanoid shooter bots, health packs, projectiles/tracers, match state, scoreboard, and combat events.
- Server hitscan combat against bots and players using body/head damage rules.
- One-click demo presets for visual, combat, match-flow, and debug recording states.
- Browser WebSocket test page for protocol validation, snapshot summaries, and server shot tests.
- Unreal server ghost actors for SERVER ECHO, bots, health packs, projectiles, and optional legacy targets.
- Gameplay HUD with HP, ammo, top 5 ranking, kill feed, crosshair, hit result text, death/respawn, health pickup feedback, and Tab scoreboard.
- Local Docker deployment and documented GCP Docker deployment flow.

## Server Architecture Highlights

- `WebSocketServer` accepts Boost.Beast WebSocket sessions and broadcasts snapshots to joined clients.
- `MessageDispatcher` parses JSON protocol messages such as `join`, `input`, `debug_room`, and demo/debug controls.
- `RoomManager` owns the fixed Room 1 used by the prototype.
- `GameRoom` owns authoritative room state and runs the simulation tick.
- `PlayerState`, `BotState`, `HealthPackState`, `ProjectileState`, `MatchState`, `CombatEvent`, and legacy `TargetState` model the current server/debug state.
- Snapshots include match, scoreboard, events, players, bots, health packs, projectiles, arena layout, and empty/legacy targets for compatibility.
- Demo presets reset the server into repeatable portfolio recording states.

## Unreal Client Highlights

- `ABattleGridClientCharacter` provides third-person camera behavior and movement state.
- `ABattleGridClientPlayerController` owns input, server profile selection, local/server HUD state, server correction settings, and ghost actor management.
- `UBattleGridWeaponComponent` owns magazine ammo, reload timing, fire rate, and spread values.
- `UBattleGridNetworkSubsystem` manages WebSocket connection, JSON send/parse, snapshot storage, scoreboard text, combat events, and shot result timing.
- Ghost actor classes make server state readable while the local prototype layer remains playable.

## Networking And Protocol Highlights

- Text WebSocket protocol using JSON for fast iteration and easy inspection.
- Client sends `join`, `input`, and debug/demo commands.
- Server sends `join_ok`, `input_ack`, `snapshot`, `debug_ok`, and error messages.
- Extended input includes movement, aim, shot direction, fire, reload, ADS, sprint, jump, ammo, and spread.
- Combat event snapshots include compact shot result messages such as `SERVER HIT BOT-3 -20`.
- Browser test page can validate protocol behavior independently from Unreal.

## Debugging And Troubleshooting Highlights

- Demo presets prevent stale `game_over`, bot pressure, and respawn confusion during recording.
- Raw snapshot logging is disabled by default in the browser to avoid slowdown.
- Server shot result events distinguish authoritative hits from local projectile overlap logs.
- SERVER ECHO origin auto-calibration aligns server arena coordinates with the local Unreal pawn.
- Documentation covers Docker, GCP, protocol messages, demo flow, common Unreal settings, and known limitations.

## Current Status

BattleGrid is a portfolio prototype. It demonstrates gameplay programming, server-authoritative state design, WebSocket networking, snapshot visualization, Dockerized server deployment, and a practical debugging workflow. It is not a shipped game and not production-ready networking.

## Future Work

- Replace placeholder ghost visuals with polished arena, character, bot, weapon, pickup, and VFX assets.
- Add Niagara muzzle flash, tracer, impact, pickup, and respawn effects.
- Improve bot movement, combat behavior, and presentation.
- Replace text scoreboard and event feed with dedicated UMG widgets.
- Add stronger server validation for weapon ammo/reload/fire cadence.
- Improve prediction, reconciliation, and latency handling.
- Automate deployment and add benchmark/load-test tooling.
