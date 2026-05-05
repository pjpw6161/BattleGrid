# BattleGrid Architecture

## Overview

BattleGrid is split into an Unreal Engine C++ client and a custom C++20 server.

The Unreal client is currently responsible for local playability: input, camera, character control, local projectile visuals, local damage, UMG HUD, and server ghost visualization.

The custom server is responsible for authoritative state experiments: WebSocket sessions, room state, latest player input, fixed-rate simulation, server projectiles, server targets, projectile-target collision, server score, and snapshot broadcast.

## Text Diagram

```text
Unreal Client
  PlayerController
  Character
  Local projectiles / targets / hazards
  UMG HUD
  NetworkSubsystem
  Ghost actors
        |
        | WebSocket JSON
        | ping, join, input
        v
C++20 Server
  WebSocketServer
  Session
  MessageDispatcher
  RoomManager
  GameRoom
  PlayerState / ProjectileState / TargetState
        |
        | snapshot JSON
        v
Unreal Ghost Actors
  Server player ghosts
  Server projectile ghosts
  Server target ghosts
```

Short form:

```text
Unreal Client -> WebSocket JSON -> C++ Server -> Snapshot -> Unreal Ghost Actors
```

## Unreal Client Role

The Unreal client remains playable without the server. It owns the local character, local projectiles, local targets, hazards, local HP, local score, victory, restart, and respawn.

When the server is available, the client connects over WebSocket, joins Room 1, sends input packets, receives snapshots, and renders server state as separate ghost actors. This keeps offline gameplay intact while exposing the server-authoritative layer for debugging.

## Custom C++ Server Role

The server listens on a configured host and port, accepts WebSocket connections, assigns player IDs, stores player state in a fixed room, updates state at a configured tick rate, and broadcasts snapshots to joined sessions.

The server currently simulates:

- Player 2D movement from latest input.
- Projectile spawn from fire input.
- Projectile movement and lifetime.
- Fixed server targets with HP.
- Projectile-target collision.
- Server score when targets are destroyed.

## Local Layer Vs Server Layer

The current demo intentionally has two layers:

- Local gameplay layer: Unreal local combat is immediate and playable.
- Server visualization layer: server state is rendered as ghosts and HUD summary data.

The layers are separate because the project is in the networking bring-up phase. The goal is to make server state visible before replacing local systems with fully server-authoritative gameplay.

## Why Ghost Actors Are Used

Ghost actors make server state inspectable without disrupting local controls. The player can compare:

- Local character position against server player ghost position.
- Local projectile visuals against server projectile ghost visuals.
- Local target damage against server target HP and server score.

The HUD also displays server position error and optional correction state so prediction/reconciliation work can be measured later.

## Current Data Flow

1. Unreal starts local gameplay.
2. `UBattleGridNetworkSubsystem` connects to `ws://127.0.0.1:7777`.
3. Unreal sends `ping` and `join`.
4. Server replies with `pong` and `join_ok`.
5. Unreal sends periodic `input` messages with movement, aim, fire, and sequence number.
6. Server stores latest input in Room 1.
7. Server tick updates players, projectiles, target collision, and score.
8. Server broadcasts `snapshot`.
9. Unreal parses players, projectiles, and targets.
10. Unreal updates ghost actors and HUD summary.

## Current Limitations

- Fixed single room.
- Server and local targets are separate.
- Server and local projectiles are separate.
- No full input replay or reconciliation history.
- No server-side player damage.
- No remote player mesh.
- No persistence or deployment yet.
