# BattleGrid Server

BattleGrid Server is the planned custom C++20 authoritative server for the BattleGrid arena shooter.

## Current Status

This is the initial server skeleton. It includes:

- CMake C++20 console application setup
- command-line configuration parsing
- simple standard-output logging
- `GameServer` startup/shutdown structure
- `GameLoop` placeholder structure

Networking, rooms, game simulation, snapshots, damage, score, and respawn are not implemented yet.

## Build on Windows PowerShell

From the repository root:

```powershell
cmake -S server -B server/build
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

## Next Planned Step

Add a basic WebSocket echo server so the Unreal client can verify local connectivity before game protocol work begins.
