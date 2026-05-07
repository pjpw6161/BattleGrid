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

## Legacy Target Ghosts No Longer Appear

Step 50 pivoted the main demo to the PvPvE Kill Race Shooter. The old `TargetState` system is now legacy/debug content and is disabled by default on the server.

Expected behavior:

- Normal snapshots contain `targets: []`.
- `debug_room` reports `targets_enabled=false`.
- Unreal `bShowServerTargetGhosts` should be false for the active PlayerController Blueprint.
- Main HUD counts show bots, health packs, and projectiles, not cores.

Re-enable target ghosts only when you specifically need to inspect legacy target/debug behavior.

## Score Equals Kills

The server ranking score now equals:

```text
score = bot_kills + player_kills
```

Bot kills and player kills are both worth 1 kill for the kill race. `target_kills` may still appear in debug data for compatibility, but it is not part of the primary ranking while targets are disabled.

## Local Projectile Hit Logs Do Not Mean Server Bot Damage

Local Unreal projectile overlap logs are part of the offline/local feedback layer. Server-authoritative bot damage is confirmed by server combat events such as:

```text
SERVER HIT BOT-3 -20
SERVER HEADSHOT BOT-3 -40
SERVER MISS
```

Use the browser `Fire 5 Shots At Nearest Bot` button or the Unreal HUD hit marker text to verify server damage.

## Humanoid Mesh Or Weapon Does Not Appear

Check:

- The asset was imported manually in Unreal Editor under `Content/BattleGrid/Art/...`.
- `BP_BattleGridCharacter` has a Skeletal Mesh assigned on the inherited `Mesh` component.
- `WeaponMeshComponent` has a Static Mesh assigned.
- `WeaponSocketName` or `BotWeaponSocketName` matches a real socket on the active skeleton.
- For bot ghosts, `bUseSkeletalMeshVisual=true` and `SkeletalMeshComponent` has a Skeletal Mesh assigned.
- If no Skeletal Mesh is assigned to the bot ghost, the static placeholder is expected to remain visible.
- If no weapon Static Mesh is assigned, the weapon component stays harmless and hidden.

If the Output Log says the socket was not found, create or rename the socket in the Skeleton editor, or adjust the socket name in the Blueprint Class Defaults. The C++ fallback attaches the weapon to the mesh root so the game can continue running during setup.

## Paragon Bot Appears Bright Purple

If only part of the humanoid bot mesh appears bright purple, check whether placeholder materials such as `M_Bot_Purple`, `M_Bot_Dead`, or `M_Bot_Invincible` are being applied to the skeletal mesh.

Expected behavior after Step 56:

- `AliveMaterial`, `DeadMaterial`, and `InvincibleMaterial` apply only to the static placeholder mesh.
- When `bUseSkeletalMeshVisual=true`, C++ preserves the Skeletal Mesh materials assigned by the imported character asset.
- The bot can still change scale, label text, hidden state, and weapon attachment without overriding Paragon/Fab material slots.

If purple still appears after rebuilding:

- Reopen `BP_BattleGridServerBotGhostActor`.
- Select `SkeletalMeshComponent`.
- Check the `Materials` override list and clear any placeholder material overrides.
- Confirm `MeshComponent` still owns placeholder materials, while `SkeletalMeshComponent` uses the imported mesh's original materials.
- Compile and save the Blueprint, then restart Play.

## Character Or Bot Is In T-Pose

The Skeletal Mesh is visible but has no compatible animation setup.

Fix:

- Assign a compatible Animation Blueprint in the Mesh or bot `SkeletalMeshComponent` details.
- Retarget animations if the imported asset uses a different skeleton.
- Verify the Anim Class is saved on the active Blueprint used by the level.
- For a demo, avoid enabling humanoid bot visuals until animation and scale are acceptable.

## Bot Label Is Inside The Humanoid Mesh

When bot ghost skeletal visuals are enabled, tune:

- `HumanoidLabelHeight`
- `HumanoidAliveScale`
- `HumanoidDeadScale`

These settings live on `BP_BattleGridServerBotGhostActor` under `BattleGrid|Visual`. They only affect visualization and do not change server hit volumes.

## Tracer Does Not Appear From The Weapon Area

Server tracer records are visual-only and currently come from the server fire origin, not from an exact Unreal weapon socket. The C++ classes expose muzzle helpers for alignment work:

- `ABattleGridClientCharacter`: `MuzzleSocketName`, `MuzzleFallbackOffset`
- `ABattleGridServerBotGhostActor`: `MuzzleSocketName`, `MuzzleFallbackOffset`

Check:

- The weapon or character skeleton has a socket such as `Muzzle`, `weapon_r`, `hand_r`, or `hand_rSocket`.
- If no muzzle socket exists, tune the fallback offset in the Blueprint Class Defaults.
- `BP_BattleGridServerProjectileGhostActor` has `bUseTracerLineVisual=true`.
- `PlayerProjectileMaterial` and `BotProjectileMaterial` are assigned if you need different tracer colors.
- Server snapshots include `visual_only=true`, `start_x/y/z`, and `end_x/y/z` for projectiles.

Tracer ghosts do not apply damage. Server shot result events and bot/player HP changes are the authoritative combat result.

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

Then click `Send Debug Restart Match` or wait for respawn. With `bot_attacks_enabled=false`, bots still move, chase, and appear in snapshots, but they do not shoot or damage players.

## Bots Kill Player Too Quickly

Use the debug/demo controls from `tools/websocket-test.html`:

1. Connect and send `Join`.
2. Click `Bot Difficulty Easy`.
3. Click `Disable Bot Attacks` if you only need to verify movement and ghost following.
4. Click `Send Debug Room` and confirm:
   - `bot_attacks_enabled=false` if attacks are disabled
   - `bot_difficulty=easy`
   - `bot_attack_damage=10`
   - `bot_headshot_damage=20`
   - `bot_fire_interval=0.75`
   - `bot_aim_spread=18`

The equivalent JSON commands are:

```json
{ "type": "debug_set_bot_difficulty", "difficulty": "easy" }
```

```json
{ "type": "debug_set_bot_attacks", "enabled": false }
```

## Game Starts In `game_over`

This usually means the previous server match reached the kill goal or the timer expired. For demo iteration:

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

Disabling the match timer only disables timer-based `game_over`. A player can still end the match by reaching the kill goal.

## Browser Freezes After `Send Join`

The server broadcasts snapshots continuously after join. Keep `Log Raw Snapshots` unchecked in `tools/websocket-test.html`; the page stores the latest snapshot internally and updates compact summary panels instead.

If the log is noisy:

- Click `Pause Snapshot Log`.
- Keep `Snapshot Summary Interval` at `30` or higher.
- Click `Clear Log`.

The page limits the visible log to 200 lines. Raw snapshot logging is only for short debugging sessions.

## Unreal Says `Server: Disconnected`

For a local Docker demo, check:

1. Docker Desktop is running.
2. The server container is up:

   ```powershell
   docker compose ps
   ```

3. The active PlayerController Blueprint has `bUseRemoteServer=false`.
4. `LocalServerUrl` is exactly:

   ```text
   ws://127.0.0.1:7777
   ```

5. `tools/websocket-test.html` can connect to the same URL.

For a GCP demo, check `bUseRemoteServer=true`, `RemoteServerUrl=ws://<GCP_EXTERNAL_IP>:7777`, and the GCP firewall/network tag allows TCP `7777`.

## Scoreboard Stays Visible

The scoreboard auto-shows when the server match is `game_over`. This can happen if an old server session reached the kill goal or the timer expired.

Fix:

1. Open `tools/websocket-test.html`.
2. Click `Apply Safe Demo Mode`.
3. Click `Send Debug Room`.
4. Confirm `match.state=in_progress` and `game_over=false`.

For Unreal-only recording, enable `bApplySafeDemoModeOnJoin` in the active PlayerController Blueprint so the server is reset after `join_ok`.

## How To Use Demo/Debug Controls

Open `tools/websocket-test.html`, connect to the same server Unreal uses, then send debug commands before starting the Unreal demo or immediately after joining:

- `Apply Safe Demo Mode`: resets the match, respawns players, resets bots/health packs, sets bot difficulty to easy, disables bot attacks, and disables timer-based game over.
- `Bot Difficulty Easy`: lowers bot shot cadence, range, speed, and accuracy.
- `Disable Bot Attacks`: keeps bot movement/ghosts active but prevents bot shooting and player damage.
- `Disable Match Timer`: prevents timer-based `game_over` during long recordings.
- `Send Debug Restart Match`: clears game-over state while preserving the current demo/debug settings.

In Unreal, enable `bApplySafeDemoModeOnJoin` on the active PlayerController Blueprint to apply the full stable demo reset automatically after `join_ok`. Use `bApplyDemoServerSettingsOnJoin` only when you want to apply the older individual demo settings instead.

## Server Bot HP Does Not Decrease

Server bot damage comes from server hitscan, not from local Unreal projectile overlap logs. Local logs that mention `StaticMeshActor` or local targets only prove the offline projectile layer is working.

Use `tools/websocket-test.html`:

1. Connect and send `Join`.
2. Click `Apply Safe Demo Mode`.
3. Click `Send Debug Room`.
4. Confirm the combat tuning panel shows body/head damage `20/40`, bot difficulty `easy`, bot attacks disabled, timer auto-end disabled, and kill score values `1/1`.
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

## Bot Shots Do Not Appear

Safe demo mode intentionally sets `bot_attacks_enabled=false`. Bots still move, but they will not fire. To test Bot Shooter AI v2:

1. Click `Apply Safe Demo Mode`.
2. Click `Enable Bot Attacks`.
3. Click `Bot Difficulty Easy`.
4. Stand within a bot attack range and watch for `BOT-* hit player* -10`, `BOT-* headshot player* -20`, or `BOT-* killed player*` events.

If no bot shots appear, click `Send Debug Room` and confirm `bot_attacks_enabled=true`, `bot_fire_interval` is nonzero, and at least one player is alive and not invincible.

## Kill Log Not Visible

The kill log uses optional UMG text bindings. If it does not appear:

- Add `KillFeedLine1`, `KillFeedLine2`, `KillFeedLine3`, `KillFeedLine4`, and `KillFeedLine5` TextBlocks to the active combat HUD widget.
- Make sure each TextBlock has `Is Variable` enabled.
- Place them on the left side of the screen.
- Check the active PlayerController Blueprint has `BattleGrid|HUD > Show Kill Feed` enabled.
- Confirm the server is sending kill events such as `bot_killed`, `player_killed`, or `bot_killed_player`.

Shot events such as `shot_hit_bot` and `shot_miss` are intentionally not shown in the kill log. They are shown as temporary server hit marker text.

## Dynamic Crosshair Not Visible

The spread crosshair uses optional UMG Border bindings. If the dynamic crosshair does not appear:

- Add `CrosshairTop`, `CrosshairBottom`, `CrosshairLeft`, `CrosshairRight`, and `CrosshairCenter` Border widgets to the active combat HUD widget.
- Make sure each Border has `Is Variable` enabled.
- Place all five widgets centered on the screen. The C++ code moves top/bottom/left/right with render translation.
- Give the line widgets small fixed sizes, for example horizontal lines `18x3`, vertical lines `3x18`, and center `4x4`.
- Check that `ShouldShowCrosshair()` is true by confirming the PlayerController still possesses the player pawn.

If the Border widgets are missing, the old `CrosshairText` `+` fallback is used. That fallback does not show spread expansion.

## Crosshair Does Not Expand Or Contract

The crosshair gap comes from the current weapon spread:

- ADS uses `AdsSpreadDegrees` and should be tight.
- Hip fire uses `HipSpreadDegrees` and should be medium.
- Sprint uses `SprintSpreadDegrees` and should be wider.
- Jumping/falling uses `JumpSpreadDegrees` and should be widest.
- Reloading or server-dead state dims the crosshair color.

If the gap does not change, verify the PlayerController input bindings for ADS, Sprint, and Jump are assigned and that the pawn has `BattleGridWeaponComponent`.

## Local Projectile Hit Logs Do Not Mean Server Bot Damage

The Unreal client still has a local projectile and local target layer for offline gameplay. Output Log lines about local projectiles hitting `StaticMeshActor` or local targets are not authoritative PvPvE combat results.

For server combat, check one of these instead:

- Server bot ghost labels show HP decreasing.
- Browser snapshots show `bots`, `scoreboard`, or `events` changing.
- Browser `Shot Result` shows `SERVER HIT`, `SERVER HEADSHOT`, or `SERVER MISS`.
- Server logs show `Shot result`, `Hitscan bot hit`, `Hitscan target hit`, or a combat event.
- The HUD `CombatMessageText` briefly shows `SERVER HIT ...` or `SERVER MISS`, then returns to the event feed.

## Server HP Differs From Local HP

This is expected during the transition from local prototype gameplay to server-authoritative PvPvE gameplay.

- `SERVER HP` comes from the own player snapshot in the C++ server.
- `Local HP` comes from Unreal's offline hazard/local damage test layer.
- By default, the HUD prioritizes `SERVER HP` and `SERVER Kills`.
- Enable `bShowLocalDebugHud` in the active PlayerController Blueprint only when you need to compare local/offline state with server state.

If the server says the player is dead, local movement/fire can be locked even if the local pawn still appears alive. Use `Apply Safe Demo Mode` if the server state needs to be reset for testing.

## Why SERVER HUD Is Primary

The PvPvE demo is meant to show server-authoritative match state. The server owns player HP, deaths, respawn timers, kills, bot/health-pack state, match timer, scoreboard, and combat events.

The HUD therefore shows server state first:

- `SERVER HP` in the health area.
- `SERVER Kills` and K/D in the score area.
- Match time, bots, health packs, and projectiles in the controls/status area.
- Recent server combat events in the message area.

Local HP/score remains available for offline debugging, but it should not be used to explain server bot damage, match scoring, or winner state.

## SERVER ECHO Follows But Starts Far Away

Server arena coordinates use fixed spawns such as `P1 = (-1200, 0)`. If Unreal simply adds those coordinates to the local pawn location, the own server ghost appears far in front of the local character.

Keep `bAutoCalibrateServerSnapshotOrigin` enabled on the PlayerController. On the first own-player snapshot, Unreal aligns that server coordinate to the current local pawn XY position, then all player, bot, projectile, and health pack ghosts use the same calibrated origin.

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
- `BuildSnapshotJson` emits `targets: []` while the legacy system is disabled.
- `debug_room` includes `targets_enabled=false` and `targets_debug_count`.
- The running server process is the rebuilt executable, not an old process.

Expected snapshot field:

```json
"targets": [
  { "target_id": 1, "x": 600.0, "y": 0.0, "hp": 100, "max_hp": 100, "alive": true }
]
```

## `server/build` Should Not Be Committed

`server/build` is generated output. Do not commit it. Keep generated build directories, Unreal `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` out of source control.
