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

## SERVER ECHO Does Not Follow The Local Player

If the local Unreal character moves but `SERVER ECHO` stays near spawn, check `debug_room`.

If the server player shows `alive=false`, `hp=0`, and `respawn_timer > 0`, bots are killing the server-side `PlayerState` before movement can be verified. For movement debugging, open `tools/websocket-test.html`, connect to the server, then click `Disable Bot Attacks`.

Expected response:

```json
{"type":"debug_ok","message":"bot attacks disabled"}
```

Then click `Send Debug Restart Match` or wait for respawn. With `bot_attacks_enabled=false`, bots still move, chase, and appear in snapshots, but they do not damage players.

## Bots Kill Player Too Quickly

Use the debug/demo controls from `tools/websocket-test.html`:

1. Connect and send `Join`.
2. Click `Bot Difficulty Easy`.
3. Click `Disable Bot Attacks` if you only need to verify movement and ghost following.
4. Click `Send Debug Room` and confirm:
   - `bot_attacks_enabled=false` if attacks are disabled
   - `bot_difficulty=easy`
   - `bot_attack_damage=10`
   - `bot_attack_cooldown=1.8`

The equivalent JSON commands are:

```json
{ "type": "debug_set_bot_difficulty", "difficulty": "easy" }
```

```json
{ "type": "debug_set_bot_attacks", "enabled": false }
```

## Game Starts In `game_over`

This usually means the previous server match reached target score or the timer expired. For demo iteration:

1. Click `Apply Safe Demo Mode`.
2. Click `Send Debug Room`.
3. Confirm `match.state=in_progress`, `match.game_over=false`, `bot_attacks_enabled=false`, `bot_difficulty=easy`, and `auto_end_match_by_timer=false`.

If you want to preserve existing debug settings instead:

1. Click `Disable Match Timer`.
2. Click `Send Debug Restart Match`.
3. Click `Send Debug Room`.
4. Confirm `auto_end_match_by_timer=false` and `match.state=in_progress`.

The equivalent JSON commands are:

```json
{ "type": "debug_set_match_timer", "enabled": false }
```

```json
{ "type": "debug_restart_match" }
```

Disabling the match timer only disables timer-based `game_over`. A player can still end the match by reaching the target score.

## Browser Freezes After `Send Join`

The server broadcasts snapshots continuously after join. Keep `Log Raw Snapshots` unchecked in `tools/websocket-test.html`; the page stores the latest snapshot internally and updates compact summary panels instead.

If the log is noisy:

- Click `Pause Snapshot Log`.
- Keep `Snapshot Summary Interval` at `30` or higher.
- Click `Clear Log`.

The page limits the visible log to 200 lines. Raw snapshot logging is only for short debugging sessions.

## How To Use Demo/Debug Controls

Open `tools/websocket-test.html`, connect to the same server Unreal uses, then send debug commands before starting the Unreal demo or immediately after joining:

- `Apply Safe Demo Mode`: resets the match, respawns players, resets bots/cores/health packs, sets bot difficulty to easy, disables bot attacks, and disables timer-based game over.
- `Bot Difficulty Easy`: lowers bot damage, range, speed, and attack cadence.
- `Disable Bot Attacks`: keeps bot movement/ghosts active but prevents player damage.
- `Disable Match Timer`: prevents timer-based `game_over` during long recordings.
- `Send Debug Restart Match`: clears game-over state while preserving the current demo/debug settings.

In Unreal, enable `bApplySafeDemoModeOnJoin` on the active PlayerController Blueprint to apply the full stable demo reset automatically after `join_ok`. Use `bApplyDemoServerSettingsOnJoin` only when you want to apply the older individual demo settings instead.

## Server Bot HP Does Not Decrease

Server bot damage comes from server hitscan, not from local Unreal projectile overlap logs. Local logs that mention `StaticMeshActor` or local targets only prove the offline projectile layer is working.

Use `tools/websocket-test.html`:

1. Connect and send `Join`.
2. Click `Apply Safe Demo Mode`.
3. Click `Send Debug Room`.
4. Confirm the combat tuning panel shows body/head damage `20/40`, bot difficulty `easy`, bot attacks disabled, timer auto-end disabled, and score values `1/2/1`.
5. Wait for a snapshot with `bots=8/8`.
6. Click `Send Fire At Nearest Bot`.
7. For a faster kill test, click `Fire 5 Shots At Nearest Bot`.

Expected server logs:

```text
[BattleGridServer] Hitscan fire shooter=1 seq=...
[BattleGridServer] Shot result shooter=1 result=hit target=bot damage=20 headshot=false
[BattleGridServer] Hitscan bot hit shooter=1 bot=... damage=20 hp=80/100
```

The browser page now also shows a `Shot Result` summary. A valid server hit should show `SERVER HIT BOT-* -20` or `SERVER HEADSHOT BOT-* -40`. If the match is `game_over`, click `Apply Safe Demo Mode` before testing shots; fire inputs can still be acknowledged, but server combat is disabled while the match is over.

For prototype aiming, bot hitscan uses 3D head/body spheres and a forgiving 2D body fallback. The fallback is only for bot hit testing; disabling bot attacks does not make bots invulnerable.

`Fire 5 Shots At Nearest Bot` sends five spread-free server fire inputs toward the nearest alive bot using the latest snapshot position. It does not rely on local projectile collision, and it keeps small delays between inputs so the browser stays responsive.

## Local Projectile Hit Logs Do Not Mean Server Bot Damage

The Unreal client still has a local projectile and local target layer for offline gameplay. Output Log lines about local projectiles hitting `StaticMeshActor` or local targets are not authoritative PvPvE combat results.

For server combat, check one of these instead:

- Server bot/core ghost labels show HP decreasing.
- Browser snapshots show `bots`, `targets`, `scoreboard`, or `events` changing.
- Browser `Shot Result` shows `SERVER HIT`, `SERVER HEADSHOT`, or `SERVER MISS`.
- Server logs show `Shot result`, `Hitscan bot hit`, `Hitscan target hit`, or a combat event.
- The HUD `CombatMessageText` briefly shows `SERVER HIT ...` or `SERVER MISS`, then returns to the event feed.

## Server HP Differs From Local HP

This is expected during the transition from local prototype gameplay to server-authoritative PvPvE gameplay.

- `SERVER HP` comes from the own player snapshot in the C++ server.
- `Local HP` comes from Unreal's offline hazard/local damage test layer.
- By default, the HUD prioritizes `SERVER HP` and `SERVER Score`.
- Enable `bShowLocalDebugHud` in the active PlayerController Blueprint only when you need to compare local/offline state with server state.

If the server says the player is dead, local movement/fire can be locked even if the local pawn still appears alive. Use `Apply Safe Demo Mode` if the server state needs to be reset for testing.

## Why SERVER HUD Is Primary

The PvPvE demo is meant to show server-authoritative match state. The server owns player HP, deaths, respawn timers, score, bot/core/health-pack state, match timer, scoreboard, and combat events.

The HUD therefore shows server state first:

- `SERVER HP` in the health area.
- `SERVER Score` and K/D in the score area.
- Match time, cores, bots, health packs, and projectiles in the controls/status area.
- Recent server combat events in the message area.

Local HP/score remains available for offline debugging, but it should not be used to explain server bot damage, match scoring, or winner state.

## SERVER ECHO Follows But Starts Far Away

Server arena coordinates use fixed spawns such as `P1 = (-1200, 0)`. If Unreal simply adds those coordinates to the local pawn location, the own server ghost appears far in front of the local character.

Keep `bAutoCalibrateServerSnapshotOrigin` enabled on the PlayerController. On the first own-player snapshot, Unreal aligns that server coordinate to the current local pawn XY position, then all player, bot, core, projectile, and health pack ghosts use the same calibrated origin.

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
