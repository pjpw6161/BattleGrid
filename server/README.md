# BattleGrid Server

BattleGrid Server is a custom C++20 WebSocket game server for the BattleGrid arena shooter portfolio project.

It currently supports local development and demo testing. It is not production-ready networking.

## Implemented Features

- CMake C++20 console application.
- Command-line config parsing.
- Simple logging.
- Boost.Beast WebSocket listen/accept/session handling.
- JSON protocol with nlohmann-json.
- `ping` / `pong`.
- `join` / `join_ok`.
- `input` / `input_ack`.
- `debug_room` / `room_state`.
- Fixed Room 1.
- Player ID allocation.
- `PlayerState` and latest `PlayerInput`.
- Fixed-rate server tick using `--tick-rate`.
- Server-side 2D player movement.
- Snapshot broadcasts to joined sessions.
- Server projectile spawn, movement, lifetime, and snapshot state.
- Fixed server targets.
- Projectile-target collision.
- Server target HP.
- Server score when targets are destroyed.
- Async session send queue for responses and snapshots.

## Dependencies

Install vcpkg packages:

```powershell
C:\tools\vcpkg\vcpkg.exe install boost-beast:x64-windows boost-system:x64-windows nlohmann-json:x64-windows
```

The repository also includes `server/vcpkg.json`, so vcpkg manifest mode can restore dependencies during CMake configure when the vcpkg toolchain is used.

## Configure

From the repository root:

```powershell
cmake -S server -B server/build -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## Build

```powershell
cmake --build server/build --config Debug
```

## Run

```powershell
.\server\build\Debug\battlegrid-server.exe --host 127.0.0.1 --port 7777 --tick-rate 30
```

Help:

```powershell
.\server\build\Debug\battlegrid-server.exe --help
```

## WebSocket Browser Test

1. Start the server.
2. Open `tools/websocket-test.html`.
3. Click `Connect`.
4. Click `Send Ping`.
5. Click `Send Join`.
6. Click `Send Debug Room`.
7. Click `Start Moving Right`.
8. Click `Send Fire Input`.
9. Watch snapshots for players, projectiles, targets, target HP, and score.

Expected response examples:

```json
{"type":"pong"}
{"type":"join_ok","player_id":1,"room_id":1,"nickname":"player1"}
{"type":"input_ack","seq":1,"player_id":1}
{"type":"snapshot","tick":1,"room_id":1,"players":[...],"projectiles":[...],"targets":[...]}
{"type":"room_state","room_id":1,"player_count":1,"projectile_count":0,"target_count":5}
```

## PowerShell Ping Test

```powershell
$ws = [System.Net.WebSockets.ClientWebSocket]::new()
$uri = [Uri]"ws://127.0.0.1:7777"
$ct = [Threading.CancellationToken]::None
$ws.ConnectAsync($uri, $ct).GetAwaiter().GetResult()
$message = [Text.Encoding]::UTF8.GetBytes('{"type":"ping"}')
$ws.SendAsync([ArraySegment[byte]]::new($message), [Net.WebSockets.WebSocketMessageType]::Text, $true, $ct).GetAwaiter().GetResult()
$buffer = [byte[]]::new(1024)
$result = $ws.ReceiveAsync([ArraySegment[byte]]::new($buffer), $ct).GetAwaiter().GetResult()
[Text.Encoding]::UTF8.GetString($buffer, 0, $result.Count)
$ws.Dispose()
```

Expected:

```json
{"type":"pong"}
```

## Current Limitations

- Fixed single room.
- Player IDs reset on server restart.
- No authentication.
- No database.
- No binary protocol.
- No server-side player damage.
- No target respawn.
- No deployment automation.
- No bot benchmark yet.
- Unreal local targets and server targets are separate layers.

## Next Planned Work

- Dockerize the server.
- Deploy to GCP.
- Add bot client and benchmark scripts.
- Add server-side player damage and respawn.
- Improve reconciliation and eventually replace local-only combat with server-authoritative state.
