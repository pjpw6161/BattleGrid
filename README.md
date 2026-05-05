# BattleGrid

Unreal Engine C++ Multiplayer Arena with Custom C++ Authoritative Server

BattleGrid is a top-down arena game built with Unreal Engine C++ and a custom C++20 WebSocket game server. Unreal handles local gameplay, input, visuals, UI, and server ghost visualization. The custom C++ server handles sessions, room state, player input, fixed-rate server tick, snapshots, server projectiles, server targets, and server-authoritative scoring.

This project exists to demonstrate both Unreal gameplay programming and C++ real-time network server programming in one portfolio-scale repository.

## Current Features

Unreal local gameplay:

- WASD movement
- Mouse aim
- Local projectile firing
- Local damageable targets
- Local HP, score, victory, and restart
- Player hazard damage and respawn
- UMG combat HUD

Custom C++ server:

- C++20 CMake console server
- Boost.Beast WebSocket server
- JSON message protocol
- `ping` / `pong`
- `join` / `join_ok`
- `input` / `input_ack`
- Room and player state
- Fixed-rate server tick
- Snapshot broadcast
- Server projectiles
- Server targets
- Server projectile-target collision
- Server score

Unreal/server integration:

- WebSocket connection from Unreal
- Join and input messages from Unreal to the server
- Snapshot parsing in an Unreal `UGameInstanceSubsystem`
- Server player ghost visualization
- Server projectile ghost visualization
- Server target ghost visualization
- Server position error display
- Optional server position correction
- Server-authoritative HUD summary

## Demo Status

The current project is ready for a portfolio demo recording focused on architecture and feature bring-up:

- Local Unreal gameplay is playable offline.
- The custom C++ server can run locally with Docker Compose or remotely on a GCP VM with Docker Compose.
- Unreal can switch between local and remote server profiles in PlayerController Blueprint defaults.
- The HUD shows local HP/score separately from server score, target count, projectile count, snapshot tick, position error, and correction state.
- Server player, projectile, and target ghosts make authoritative server state visible without hiding the local gameplay layer.
- Demo mode keeps repetitive input, snapshot, and ack logs quiet while preserving connection, join, spawn, error, victory, and respawn logs.

## Architecture

```text
                local input / visuals / offline gameplay
                              |
                              v
+------------------+   WebSocket JSON   +----------------------+
| Unreal Client    | -----------------> | C++20 Server          |
| PlayerController | input/join/ping    | WebSocketServer       |
| Character        |                    | Session              |
| UMG HUD          | <----------------- | RoomManager/GameRoom  |
| Ghost Actors     | snapshot JSON      | Player/Projectile/Target State |
+------------------+                    +----------------------+
                              |
                              v
              Unreal renders server snapshots as ghost actors
```

The current demo intentionally keeps two layers visible:

- Local offline layer: Unreal local character, local projectiles, local damageable targets, local HP, local score, victory, restart, and respawn.
- Server-authoritative visualization layer: server players, projectiles, targets, positions, target HP, and server score shown through ghost actors and HUD summary.

This makes the networking bring-up easy to inspect before replacing local systems with fully authoritative gameplay.

## Repository Layout

```text
client-unreal/   Unreal Engine C++ client project
server/          Custom C++20 WebSocket game server
tools/           Browser WebSocket protocol test page
docs/            Architecture, protocol, demo, and troubleshooting docs
```

## Build And Run

### Server

Install dependencies with vcpkg:

```powershell
C:\tools\vcpkg\vcpkg.exe install boost-beast:x64-windows boost-system:x64-windows nlohmann-json:x64-windows
```

Configure and build from the repository root:

```powershell
cmake -S server -B server/build -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build server/build --config Debug
```

Run:

```powershell
.\server\build\Debug\battlegrid-server.exe --host 127.0.0.1 --port 7777 --tick-rate 30
```

### Docker Server

Build and run the C++ server with Docker Compose:

```powershell
docker compose up --build
```

Run in the background:

```powershell
docker compose up -d --build
```

View logs:

```powershell
docker compose logs -f battlegrid-server
```

Stop:

```powershell
docker compose down
```

The container exposes WebSocket port `7777` on the host, so Unreal and `tools/websocket-test.html` can use `ws://127.0.0.1:7777`.

### GCP Docker Server

The same Docker Compose setup can run on an Ubuntu GCP VM. The current manual deployment flow is:

```bash
git clone https://github.com/pjpw6161/BattleGrid.git
cd BattleGrid
docker compose up -d --build
docker compose logs -f battlegrid-server
```

Open TCP port `7777` on the VM firewall, then test:

```text
ws://<GCP_EXTERNAL_IP>:7777
```

See [GCP deployment](docs/deployment-gcp.md) for the full VM setup and operation flow.

### Unreal Client

1. Open `client-unreal/BattleGridClient/BattleGridClient.uproject`.
2. Build `BattleGridClientEditor` in Visual Studio with `Development Editor | Win64`.
3. Open the Unreal Editor.
4. Press Play.

Unreal server profile switching:

- Local Docker server: set `bUseRemoteServer = false`, keep `LocalServerUrl = ws://127.0.0.1:7777`.
- GCP server: set `bUseRemoteServer = true`, set `RemoteServerUrl = ws://<GCP_EXTERNAL_IP>:7777`.
- The HUD shows the active profile as `Profile: Local` or `Profile: Remote`.

Unreal Editor assets such as Input Actions, Mapping Contexts, Blueprints, maps, and Widget Blueprints are edited by the human developer in Unreal Editor, not generated by Codex.

## Current Demo Flow

1. Start the C++ server natively, with local Docker Compose, or on the GCP VM.
2. Open `tools/websocket-test.html` and verify `ping`, `join`, `debug_room`, and snapshots.
3. Open Unreal Editor.
4. Press Play.
5. Verify the HUD shows server connection, player ID, room ID, snapshot tick, server score, targets, projectiles, position error, and correction state.
6. Move with WASD and aim with the mouse.
7. Fire with left mouse button.
8. Observe local projectiles and server projectile ghosts.
9. Observe server player, projectile, and target ghost actors.
10. Damage local targets and compare local score with server score.
11. Test hazard damage, HP loss, death, respawn, victory, and R restart.
12. Stop the server and verify local offline gameplay still works.

## Current Limitations

- Local targets and server targets are separate layers.
- Local projectile damage and server projectile damage are separate.
- Full prediction, input replay, and reconciliation history are not implemented.
- Remote player meshes are not implemented.
- Server-side player damage is not implemented.
- Target respawn is not implemented.
- There is no persistence or database.
- GCP deployment is manual and not automated.
- Bot load testing and benchmarking are not complete.
- Networking is not production-ready and has no authentication or anti-cheat.

## Next Steps

- Plan the third-person PvPvE kill race upgrade.
- Automate GCP deployment.
- Add a bot client.
- Add benchmark and load-test scripts.
- Polish a demo video.
- Improve client prediction and reconciliation.
- Integrate local and server target layers.
- Move more combat rules to the server.

## Documentation

- [Project brief](PROJECT_BRIEF.md)
- [Planned game design](docs/game-design.md)
- [Implementation roadmap](docs/implementation-roadmap.md)
- [Overall architecture](docs/architecture.md)
- [Unreal client](docs/unreal-client.md)
- [Server architecture](docs/server-architecture.md)
- [WebSocket JSON protocol](docs/protocol.md)
- [Docker deployment](docs/deployment-docker.md)
- [GCP deployment](docs/deployment-gcp.md)
- [Development log](docs/development-log.md)
- [Demo checklist](docs/demo-checklist.md)
- [Troubleshooting](docs/troubleshooting.md)
