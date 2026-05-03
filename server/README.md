# BattleGrid Server

BattleGrid Server is the planned custom C++20 authoritative server for the BattleGrid arena shooter.

## Current Status

This is the initial server skeleton with a Boost.Beast WebSocket JSON protocol server.
It includes:

- CMake C++20 console application setup
- command-line configuration parsing
- simple standard-output logging
- `GameServer` startup/shutdown structure
- `GameLoop` placeholder structure
- WebSocket listen/accept/session handling
- JSON message parsing and dispatch
- `ping` / `pong`
- `join` / `join_ok`
- `input` / `input_ack`
- fixed Room 1 state with `PlayerState` and latest `PlayerInput`
- `debug_room` / `room_state`
- JSON error responses

Multiple rooms, game simulation, snapshots, damage, score, and respawn are not implemented yet.

## Dependencies

Install Boost.Beast, Boost.System, and nlohmann-json with vcpkg:

```powershell
C:\tools\vcpkg\vcpkg.exe install boost-beast:x64-windows boost-system:x64-windows nlohmann-json:x64-windows
```

The server also includes `server/vcpkg.json`, so the vcpkg toolchain can restore these dependencies during CMake configure.

## Build on Windows PowerShell

From the repository root:

```powershell
cmake -S server -B server/build -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build server/build --config Debug
```

## Run

```powershell
.\server\build\Debug\battlegrid-server.exe --host 0.0.0.0 --port 7777 --tick-rate 30
```

For help:

```powershell
.\server\build\Debug\battlegrid-server.exe --help
```

## Test WebSocket JSON Protocol

Start the server, then open:

```text
tools/websocket-test.html
```

Use the page buttons:

- `Connect`
- `Send Ping`
- `Send Join`
- `Send Input`
- `Send Fire Input`
- `Send Debug Room`
- `Send Invalid JSON`
- `Close`

Expected responses:

```json
{"type":"pong"}
{"type":"join_ok","player_id":1,"room_id":1,"nickname":"player1"}
{"type":"input_ack","seq":1,"player_id":1}
{"type":"room_state","room_id":1,"player_count":1,"players":[...]}
{"type":"error","message":"invalid json"}
```

You can also test from PowerShell:

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

Expected PowerShell output:

```json
{"type":"pong"}
```

## Next Planned Step

Add server-side game state and movement validation.
