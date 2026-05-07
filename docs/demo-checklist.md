# BattleGrid Demo Checklist

Use this checklist to run the current portfolio demo.

## Before Recording

- Docker Desktop is running.
- If server code changed, rebuild the local Docker server:

  ```powershell
  docker compose up -d --build battlegrid-server
  ```

- `tools/websocket-test.html` has `Log Raw Snapshots` unchecked.
- Browser test page has a fresh connection to `ws://127.0.0.1:7777` or `ws://<GCP_EXTERNAL_IP>:7777`.
- Click `Apply Safe Demo Mode`.
- Click `Send Debug Room` and confirm:
  - `match.state=in_progress`
  - `game_over=false`
  - `bot_attacks_enabled=false`
  - `bot_difficulty=easy`
  - `auto_end_match_by_timer=false`
- Active Unreal PlayerController Blueprint defaults:
  - `bUseRemoteServer=false` for a local Docker demo.
  - `bApplySafeDemoModeOnJoin=true`.
  - `bUseServerAuthoritativeHud=true`.
  - `bShowLocalDebugHud=false`.
  - `bUseServerPositionCorrection=false`.
  - `bAutoCalibrateServerSnapshotOrigin=true`.

## During Recording

- Show the browser test page applying safe demo mode.
- Show `Fire 5 Shots At Nearest Bot` and verify `SERVER HIT` / bot HP changes.
- Show Unreal connected with `Profile: Local` or `Profile: Remote`.
- Show SERVER ECHO, bot ghosts, and HPACK ghosts.
- Shoot a bot in Unreal and show `SERVER HIT` or `SERVER MISS` in the HUD.
- Hold Tab to show the server scoreboard.
- Mention that local projectile visuals and server hitscan damage are currently separate prototype layers.

## After Recording

- Stop the local Docker server if it is not needed:

  ```powershell
  docker compose down
  ```

- Review README, demo script, portfolio summary, and checklist.
- Commit the documentation and any intended source changes.
- Optionally upload the video/GIF and update the README `Demo Video` section.

## Polished Portfolio Demo Script

1. Start the GCP Docker server:

   ```bash
   docker compose up -d --build
   docker compose logs -f battlegrid-server
   ```

2. Open `tools/websocket-test.html`.
3. Connect to `ws://<GCP_EXTERNAL_IP>:7777`.
4. Click `Send Join`.
5. Click `Apply Safe Demo Mode`.
6. Click `Send Debug Room`.
7. Confirm the browser output shows `match.state=in_progress`, `game_over=false`, `bot_attacks_enabled=false`, `bot_difficulty=easy`, `auto_end_match_by_timer=false`, `targets_enabled=false`, `bots=8`, and `health_packs=3`.
8. Keep `Log Raw Snapshots` unchecked so the page shows compact summaries without freezing.
9. Click `Fire 5 Shots At Nearest Bot` and verify BOT HP decreases or a bot kill event appears.
10. Open Unreal Editor.
11. In the PlayerController Blueprint defaults, use the remote profile and confirm the HUD shows `Profile: Remote`, `Server: Connected`, and server-authoritative `SERVER HP` / `SERVER Kills` as the primary state.
12. Move with WASD.
13. Show the local character and the server player ghost moving together.
14. Fire with left mouse button and show both the local projectile and server projectile ghost.
15. Confirm server target/core ghosts are hidden by default for the kill race demo.
16. Hit a local target only if you want to explain the legacy offline layer.
17. Hit a server bot and show the explicit server shot result:
   - Browser `Shot Result`: `SERVER HIT BOT-* -20`, `SERVER HEADSHOT BOT-* -40`, or `SERVER MISS`.
   - Unreal HUD hit marker text: `SERVER HIT ...` or `SERVER MISS` for about 1.25 seconds by default.
   - Server kills / HP changes in the HUD and browser summaries when the hit deals damage.
18. Validate visual placeholder readability:
   - `SERVER ECHO` label is visible on the server player ghost.
   - `BOT-*` labels are visible and dead bots read as `DOWN`.
   - `HPACK-*` labels are visible and inactive packs show a countdown.
   - local objects and server ghost objects are visually distinguishable.
19. Validate the arena layout: P1/P2/P3/P4 spawns sit on the four sides, bots are around center/lanes, and health packs are near the perimeter.
20. Verify the server combat event feed appears after a bot/player event and that shot results are distinct from local projectile overlap logs.
21. Hold Tab and verify the server scoreboard overlay appears; release Tab and verify the normal HUD returns.
22. Optional: enable bot attacks on easy to show server danger after the stable shooting demo.
23. Walk into the hazard and show local HP decrease, death, and respawn only as an offline/local debug layer.
24. Toggle `bShowLocalDebugHud` if you need to show local HP/score and position error during explanation.
25. Toggle optional server position correction on/off if useful for the recording.
26. Stop the server or switch back to the Local profile and show that offline gameplay still runs.

## Server Smoke Test

### Option A: Docker Compose

1. Start the server container:

   ```powershell
   docker compose up -d --build
   ```

2. Verify logs:

   ```powershell
   docker compose logs -f battlegrid-server
   ```

3. Verify the container exposes port `7777`:

   ```powershell
   docker compose ps
   ```

4. Open `tools/websocket-test.html`.
5. Connect to `ws://127.0.0.1:7777`.
6. Click `Send Ping` and verify `pong`.
7. Click `Send Join` and verify `join_ok`.
8. Click `Apply Safe Demo Mode`.
9. Click `Send Debug Room` and verify Room 1, player state, bots, health packs, and `targets_enabled=false`.
10. Confirm `match.state=in_progress`, `game_over=false`, `bot_attacks_enabled=false`, `bot_difficulty=easy`, and `auto_end_match_by_timer=false`.
11. Click `Send Fire At Nearest Bot` and verify the `Shot Result` card shows `SERVER HIT BOT-* -20` or `SERVER MISS`.
12. Click `Fire 5 Shots At Nearest Bot` and verify bot HP drops quickly. A 100 HP bot should be killed by five body hits at 20 damage each unless shots miss or the match is over.
13. Verify the combat tuning summary shows body/head damage `20/40`, easy bot difficulty, bot attacks disabled, timer auto-end disabled, and kill score values `1/1`.
14. Verify the arena layout values from the first snapshot output:
   - bounds `x=-1800..1800`, `y=-1200..1200`
   - player spawns at `(-1200,0)`, `(1200,0)`, `(0,900)`, `(0,-900)`
   - bots count `8`
   - health packs count `3`

### Option B: Native Windows Build

1. Build the server:

   ```powershell
   cmake --build server/build --config Debug
   ```

2. Start the server:

   ```powershell
   .\server\build\Debug\battlegrid-server.exe --host 127.0.0.1 --port 7777 --tick-rate 30
   ```

3. Open `tools/websocket-test.html`.
4. Click `Connect`.
5. Click `Send Ping` and verify `pong`.
6. Click `Send Join` and verify `join_ok`.
7. Click `Apply Safe Demo Mode`.
8. Click `Send Debug Room` and verify Room 1, player state, bots, health packs, and `targets_enabled=false`.
9. Verify the arena bounds and server entity positions from snapshots match `docs/arena-layout.md`.

### Shared Server Test Steps

1. Click `Start Moving Right` and watch snapshots update player `x` / `y`.
2. Click `Send Fire Input` and watch projectiles appear in snapshots.
3. Click `Send Fire At Nearest Bot` and watch bot HP plus the `Shot Result` card update.
4. Click `Fire 5 Shots At Nearest Bot` and watch bot HP decrease across multiple snapshots, then confirm a kill event and scoreboard kill update when the bot reaches 0 HP.
5. Watch bot HP and server kills change after server hits.

### Option C: Remote GCP Server

1. SSH into the GCP VM.
2. Start the server:

   ```bash
   docker compose up -d --build
   ```

3. Watch logs:

   ```bash
   docker compose logs -f battlegrid-server
   ```

4. From the local browser test page, connect to:

   ```text
   ws://<GCP_EXTERNAL_IP>:7777
   ```

5. Click `Send Ping`, `Send Join`, and `Send Debug Room`.
6. Verify snapshots include players, projectiles/tracers, bots, health packs, match, scoreboard, and events from the remote server.
7. Verify snapshots include arena bounds and the same bot and health pack layout.

## Unreal Demo

1. Build `BattleGridClientEditor` in Visual Studio with `Development Editor | Win64`.
2. Open `client-unreal/BattleGridClient/BattleGridClient.uproject`.
3. For local server mode:
   - Set `bUseRemoteServer` to false.
   - Set `LocalServerUrl` to `ws://127.0.0.1:7777`.
4. For remote GCP server mode:
   - Set `bUseRemoteServer` to true.
   - Set `RemoteServerUrl` to `ws://<GCP_EXTERNAL_IP>:7777`.
5. Press Play.
6. Verify the HUD shows:
   - `SERVER HP` as the primary health readout while connected/joined.
   - `SERVER Kills` and kill goal as the primary scoring readout.
   - Server connection.
   - Profile label, either `Profile: Local` or `Profile: Remote`.
   - Match time and kill goal.
   - Bot count.
   - Health pack count.
   - Server projectile count.
   - Recent combat event feed.
   - Temporary server hit marker text after shots: `SERVER HIT ...`, `SERVER HEADSHOT ...`, or `SERVER MISS`.
   - Optional local debug line only when `bShowLocalDebugHud` is enabled.
   - Position error and correction state.
7. Move with WASD.
8. Aim with the mouse.
9. Fire with left mouse button.
10. Show local projectile and local target damage.
11. Show local player hazard damage, death, and respawn.
12. Show server player ghost movement.
13. Show server projectile ghost movement.
14. Confirm server target/core ghosts are hidden by default.
15. Show server kills increasing when server bots are killed.
16. Validate polished server placeholder labels:
   - player ghost: `SERVER ECHO P#`
   - bot ghost: `BOT-# hp/max`, `BOT-# INV`, or `BOT-# DOWN`
   - health pack ghost: `HPACK-# +35` or `HPACK-# Ns`
17. Validate the server layout against the Unreal map:
   - player ghosts remain inside the intended arena bounds
   - bot ghosts start near center/side lanes
   - health pack ghosts appear near outer lanes
18. Switch from local mode to remote mode and show the HUD profile label changing.
19. Show server ghosts driven by the remote GCP server.
20. Hold Tab and verify the server scoreboard overlay appears. It should also show automatically when the server match is `game_over`.
21. Toggle optional server position correction in the PlayerController Blueprint defaults if needed, then compare error behavior.
22. Stop the server and verify local offline gameplay still works.

## Victory And Restart

1. Place enough local damageable targets for the configured local `TargetScore`.
2. Destroy local targets.
3. Verify local victory message.
4. Press R to restart the level.

## Demo Talking Points

- BattleGrid demonstrates both Unreal gameplay programming and custom C++ server programming.
- The ghost layer makes server state visible without hiding local gameplay.
- Server kills/HP are primary for the PvPvE demo.
- Local score/HP remains available as an optional debug/offline layer through `bShowLocalDebugHud`.
