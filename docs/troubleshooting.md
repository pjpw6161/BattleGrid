# Troubleshooting

## `cmake` Not Found

Install CMake and ensure it is on `PATH`.

Verify:

```powershell
cmake --version
```

## Server Executable Not Found

Build the server first:

```powershell
cmake -S server -B server/build -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build server/build --config Debug
```

Then run:

```powershell
.\server\build\Debug\battlegrid-server.exe --host 127.0.0.1 --port 7777 --tick-rate 30
```

## Port 7777 Already In Use

An old server process may still be running. Close the terminal running it, stop the process in Task Manager, or run the server on another port:

```powershell
.\server\build\Debug\battlegrid-server.exe --port 7788
```

If the port changes, update the Unreal PlayerController `ServerUrl`.

For Docker Compose, stop the container:

```powershell
docker compose down
```

Then verify the port:

```powershell
netstat -ano | findstr :7777
```

## Docker Desktop Not Running

If `docker compose up --build` fails before building the image, open Docker Desktop and wait until the engine is running.

Verify:

```powershell
docker version
docker compose version
```

## Docker Build Dependency Error

The server Dockerfile installs Ubuntu packages during the build. If dependency installation fails, check:

- Docker Desktop network access.
- Corporate proxy or firewall settings.
- Available disk space.
- Whether the error occurs during `apt-get update` or CMake configure.

Rebuild from a clean Docker cache if needed:

```powershell
docker compose build --no-cache battlegrid-server
```

## Container Exits Immediately

Check logs:

```powershell
docker compose logs battlegrid-server
```

Common causes:

- Port bind failure.
- Missing runtime library.
- Server command-line parse error.
- Another container or native server already using `7777`.

## Browser Cannot Connect To Docker Server

Check:

- `docker compose ps` shows the service running.
- Ports show `7777:7777`.
- Browser test uses `ws://127.0.0.1:7777`.
- Windows firewall is not blocking Docker networking.

## GCP Firewall TCP 7777 Not Open

If the server works on the VM but cannot be reached from your local machine, check the GCP firewall rule:

- Ingress rule exists.
- Target tag is `battlegrid-server`.
- VM has the `battlegrid-server` network tag.
- Protocol/port includes `tcp:7777`.
- Source range includes your local client IP.

## VM Missing Network Tag

A firewall rule with target tag `battlegrid-server` does nothing unless the VM has the same network tag. Add the tag to the VM network settings, then test again:

```text
ws://<GCP_EXTERNAL_IP>:7777
```

## Docker Container Running But External Connection Fails

Check on the VM:

```bash
docker compose ps
docker compose logs battlegrid-server
sudo ss -ltnp | grep 7777
```

The server must listen on `0.0.0.0:7777`, not only `127.0.0.1:7777`, for external clients to connect.

## Old Server Process Occupying 7777 On VM

Stop old containers or native processes:

```bash
docker compose down
sudo ss -ltnp | grep 7777
```

If a native process is still using the port, stop that process before restarting Docker Compose.

## `RemoteServerUrl` Accidentally Committed With Personal IP

Do not hardcode or commit personal VM IPs in C++ defaults or documentation. Use:

```text
ws://<GCP_EXTERNAL_IP>:7777
```

The actual `RemoteServerUrl` should be set manually in Unreal Editor Blueprint defaults for local testing and demos.

## Browser Connects But Unreal Fails

Check:

- Unreal `bUseRemoteServer` is true for GCP mode.
- `RemoteServerUrl` is exactly `ws://<GCP_EXTERNAL_IP>:7777`.
- There is no trailing whitespace in the URL.
- The browser test and Unreal are using the same network.
- The Unreal Output Log shows `Server profile: Remote`.
- The Unreal Output Log shows either WebSocket connected or a connection error.

## Profile Shows Local Even When Remote Is Checked

Check:

- `RemoteServerUrl` is not empty.
- `RemoteServerUrl` uses the full WebSocket URL, for example `ws://<GCP_EXTERNAL_IP>:7777`.
- You edited the PlayerController Blueprint used by the active GameMode.
- The Blueprint defaults were saved before pressing Play.
- Unreal was restarted or the project was rebuilt if C++ metadata did not refresh.
- The Output Log shows `Server profile: Remote`.

## Browser WebSocket CSP Error

Do not test WebSockets from `chrome://` pages or other restricted browser pages. Open `tools/websocket-test.html` directly or serve it from a normal local web page.

## Unreal WebSocket Connection Fails

Check:

- The C++ server is running.
- The server URL is `ws://127.0.0.1:7777`.
- Windows firewall is not blocking the process.
- `BattleGridClient.Build.cs` includes `WebSockets`, `Json`, and `JsonUtilities`.
- The Output Log contains either connected, closed, or connection error messages.

## Too Many Unreal Logs

For a clean demo recording, use the PlayerController Blueprint defaults:

- Set `bDemoMode` to true.
- Keep `bVerboseNetworkLogs`, `bVerboseSnapshotLogs`, and `bVerboseInputLogs` false.
- Keep `SnapshotLogInterval` and `InputAckLogInterval` at `60` or higher.

Important logs such as server profile, connect, connection error, join, ghost spawns, victory, and respawn should still appear. Repetitive input, input ack, snapshot, coordinate conversion, and server position error logs are throttled.

## Blueprint Class Variables Not Visible

Check:

- The C++ class uses `UCLASS(Blueprintable)` where needed.
- Properties use `UPROPERTY` with `EditDefaultsOnly` or `EditAnywhere`.
- The project was rebuilt in Visual Studio.
- Unreal Editor was restarted if hot reload did not refresh metadata.
- The Blueprint parent class is the expected C++ class.
- In stubborn cases, recreate the Blueprint from the updated C++ parent.

## New C++ Files Not Visible In Visual Studio

Regenerate project files from the `.uproject` context menu or Unreal Editor project tools, then reopen the solution.

## Actor Overlap Works But Damage Does Not Apply

Check:

- The overlapping actor is the expected character class.
- The damage path calls `UGameplayStatics::ApplyDamage`.
- The target/character overrides `TakeDamage`.
- Logs show `TakeDamage` and health before/after.
- Collision presets allow overlap or hit events as expected.

## Hazard Overlap Fires But Player HP Does Not Decrease

Check:

- The active pawn derives from `ABattleGridClientCharacter`.
- `ABattleGridHazardActor` logs actor-level overlap.
- `TryDamageActor` logs a successful cast.
- `ABattleGridClientCharacter::TakeDamage` logs damage.
- `UBattleGridHealthComponent::ApplyDamage` logs HP before and after.

## Ghost Direction Is Wrong

The server uses logical 2D arena coordinates. The Unreal top-down template currently swaps axes for visualization:

```text
WorldX = ServerY
WorldY = ServerX
```

If projectile direction is mirrored, check `ServerAimSignX` and `ServerAimSignY` on the PlayerController.

## Ghost Movement Speed Mismatch

Check:

- Movement release events are bound to `Completed` and `Canceled`.
- Unreal sends immediate zero input on release.
- Server player speed is close to Unreal CharacterMovement speed.
- Ghost interpolation speed is high enough for debugging.
- Optional snap mode is enabled on ghost actors only when needed.

## Targets Missing From Snapshot

Check:

- `TargetState.cpp` is listed in `server/CMakeLists.txt`.
- `GameRoom` initializes default targets.
- `BuildSnapshotJson` always emits `targets`.
- `debug_room` includes `target_count` and `targets`.
- The running server process is the rebuilt executable, not an old process.

Expected snapshot field:

```json
"targets": [
  { "target_id": 1, "x": 600.0, "y": 0.0, "hp": 100, "max_hp": 100, "alive": true }
]
```

## `server/build` Should Not Be Committed

`server/build` is generated output. Do not commit it. Keep generated build directories, Unreal `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` out of source control.
