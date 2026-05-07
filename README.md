# BattleGrid

Third-Person PvPvE Arena Shooter with Unreal Engine C++ and a Custom C++ Authoritative Server

BattleGrid is a third-person PvPvE arena shooter prototype. The Unreal client handles controls, visuals, local feedback, HUD, and server ghost visualization. The custom C++20 server handles WebSocket sessions, room state, server tick, player input, bots, health packs, server hitscan combat, match state, scoreboard, combat events, and snapshots.

This project exists to demonstrate both Unreal gameplay programming and custom real-time C++ server programming in one portfolio-scale repository.

## Demo Video

Coming soon.

## Why This Project Matters

BattleGrid shows the path from a playable Unreal prototype to a server-authoritative multiplayer architecture:

- Unreal gameplay programming: third-person controls, ADS, sprint, jump, weapon state, HUD, local feedback, and prototype combat visuals.
- Custom C++ server programming: Boost.Beast WebSocket sessions, JSON protocol dispatch, fixed-rate room tick, authoritative game state, bots, pickups, match state, and snapshot broadcast.
- Server-authoritative design: server-owned HP, deaths, respawn, bot/core/health-pack state, scoreboard, and hit result events.
- Deployment workflow: local Docker Compose server and documented GCP VM Docker deployment.
- Debugging workflow: browser protocol test page, safe demo mode, compact snapshot summaries, shot result events, ghost actors, and troubleshooting docs.

## Current Demo Features

| Area | Feature | Status |
| --- | --- | --- |
| Unreal client | Third-person movement | Implemented |
| Unreal client | ADS / sprint / jump | Implemented |
| Unreal client | Ammo / reload / automatic fire | Implemented |
| Networking | WebSocket JSON connection | Implemented |
| Networking | Local/remote server profiles | Implemented |
| Demo tooling | Safe demo mode | Implemented |
| Server state | Snapshot broadcast | Implemented |
| Visualization | Server player ghost / SERVER ECHO | Implemented |
| Visualization | Server bot ghosts | Implemented |
| Visualization | Server core/target ghosts | Implemented |
| Visualization | Server health pack ghosts | Implemented |
| Combat | Server hitscan shot feedback | Implemented |
| PvE | Bot damage, death, respawn, score | Implemented |
| Match | Match state and scoreboard | Implemented |
| Deployment | Docker local server | Implemented |
| Deployment | GCP deployment flow | Documented |

## Architecture

```text
Unreal Client
  -> WebSocket JSON input
  -> C++ Server
  -> GameRoom tick
  -> snapshot
  -> Unreal ghost actors / HUD
```

Expanded view:

```text
+-----------------------+      WebSocket JSON       +------------------------+
| Unreal Engine C++     |  input / join / debug     | C++20 Server           |
| PlayerController      | ------------------------> | Boost.Beast sessions   |
| Character + Weapon    |                           | MessageDispatcher      |
| HUD / UMG             | <------------------------ | GameRoom fixed tick    |
| Server ghost actors   |       snapshot JSON       | authoritative state    |
+-----------------------+                           +------------------------+
```

The current demo intentionally keeps two layers visible:

- Server-authoritative PvPvE layer: server player HP, bots, cores, health packs, match state, scoreboard, combat events, and shot results.
- Local Unreal prototype layer: local projectile visuals, local target tests, local hazards, and offline gameplay fallback.

The server layer is the primary portfolio demo path. The local layer remains useful for offline testing and incremental client work.

## Repository Layout

```text
client-unreal/   Unreal Engine C++ client project
server/          Custom C++20 WebSocket game server
tools/           Browser WebSocket protocol test page
docs/            Architecture, protocol, deployment, demo, and troubleshooting docs
```

## Build And Run

### Docker Server

Build and run the server locally:

```powershell
docker compose up -d --build battlegrid-server
docker compose logs -f battlegrid-server
```

Stop the server:

```powershell
docker compose down
```

The server listens on WebSocket port `7777`, so local tools use:

```text
ws://127.0.0.1:7777
```

### Native Server Build

Install dependencies with vcpkg:

```powershell
C:\tools\vcpkg\vcpkg.exe install boost-beast:x64-windows boost-system:x64-windows nlohmann-json:x64-windows
```

Configure and build:

```powershell
cmake -S server -B server/build -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build server/build --config Debug
```

Run:

```powershell
.\server\build\Debug\battlegrid-server.exe --host 127.0.0.1 --port 7777 --tick-rate 30
```

### GCP Docker Server

The same Docker Compose setup can run on an Ubuntu GCP VM:

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

Server profile settings live in the active PlayerController Blueprint defaults:

- Local Docker server: `bUseRemoteServer = false`, `LocalServerUrl = ws://127.0.0.1:7777`.
- GCP server: `bUseRemoteServer = true`, `RemoteServerUrl = ws://<GCP_EXTERNAL_IP>:7777`.
- Demo startup: enable `bApplySafeDemoModeOnJoin` for stable recording.

Unreal assets such as Input Actions, Mapping Contexts, Blueprints, maps, materials, and Widget Blueprints are created or adjusted manually in Unreal Editor.

## Current Demo Flow

1. Start the server with Docker Compose.
2. Open `tools/websocket-test.html`.
3. Connect, send join, and apply safe demo mode.
4. Use `Fire 5 Shots At Nearest Bot` to verify `SERVER HIT` events and bot HP changes.
5. Open Unreal and press Play.
6. Verify the HUD shows server HP/score as primary.
7. Move, ADS, sprint, jump, and fire.
8. Show SERVER ECHO, bot ghosts, core ghosts, health pack ghosts, and server shot result text.
9. Hold Tab to show the server scoreboard.
10. Stop the server and confirm local offline gameplay still exists.

See [demo checklist](docs/demo-checklist.md) and [demo script](docs/demo-script.md) for a recording-ready sequence.

## Current Limitations

- Local projectile visuals and server hitscan damage are still separate layers.
- Local offline targets and server cores are still separate.
- Server ghost actors are prototype visualization, not final player/bot/core presentation.
- No production matchmaking or lobby flow.
- No database or persistence.
- No advanced lag compensation, rollback, or anti-cheat.
- Placeholder visuals use Unreal built-in shapes and local materials.
- No final character, weapon, animation, audio, or VFX assets yet.
- GCP deployment is documented and manual, not fully automated infrastructure.
- Networking is not production-ready.

## Next Steps

- Polish arena art and map layout.
- Add character and weapon models.
- Add Niagara muzzle flash, tracer, and impact effects.
- Improve bot behavior and presentation.
- Replace text scoreboard with a dedicated UMG scoreboard.
- Polish GCP demo server operation.
- Record and publish a demo video/GIF.
- Continue prediction/reconciliation and server authority improvements.

## Documentation

- [Project brief](PROJECT_BRIEF.md)
- [Portfolio summary](docs/portfolio-summary.md)
- [Demo script](docs/demo-script.md)
- [Game design](docs/game-design.md)
- [Implementation roadmap](docs/implementation-roadmap.md)
- [Overall architecture](docs/architecture.md)
- [Unreal client](docs/unreal-client.md)
- [Server architecture](docs/server-architecture.md)
- [WebSocket JSON protocol](docs/protocol.md)
- [Docker deployment](docs/deployment-docker.md)
- [GCP deployment](docs/deployment-gcp.md)
- [Asset credits](docs/asset-credits.md)
- [Development log](docs/development-log.md)
- [Demo checklist](docs/demo-checklist.md)
- [Troubleshooting](docs/troubleshooting.md)
