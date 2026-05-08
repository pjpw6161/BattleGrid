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
  - `use_client_fire_origin_for_hitscan=true`
  - `use_client_position_for_player_movement=true`
  - `min_player_z=-1000` and `max_player_z=5000`
  - easy bot pressure values: `bot_fire_interval=1.0`, `bot_aim_spread=18`, `max_bots_targeting_one_player=8`, `max_bots_shooting_one_player=2`, and `respawn_invincible_seconds=2.5`
  - `bot_area_bounds` is approximately `x=-1500..1500`, `y=-900..900`
- `bot_attacks_enabled=false` means bots still move but do not shoot players.
- Active Unreal PlayerController Blueprint defaults:
  - `bUseRemoteServer=false` for a local Docker demo.
  - `bApplySafeDemoModeOnJoin=true`.
  - `bUseServerAuthoritativeHud=true`.
  - `bUseGameplayHudLayout=true`.
  - `bShowDebugHud=false`.
  - `bShowControlsHelp=false`.
  - `bShowSmallServerStatus=true`.
  - `bShowLocalDebugHud=false`.
  - `bShowCombatEventFeed=false`.
  - `bShowTopFiveRanking=true`.
  - `bShowKillFeed=true`.
  - `bUseServerPositionCorrection=false`.
  - `bUseGentleServerPositionCorrection=false` unless you are testing optional smoothing.
  - `bDrawServerArenaBounds=false` for recording; enable only while verifying invisible-wall/correction issues.
  - `bDrawServerBotAreaBounds=false` for recording; enable only while verifying bot bounds/spawns.
  - `bSendClientPositionToServer=true`.
  - `bSnapLocalPawnToServerOnJoin=true`.
  - `bSnapLocalPawnToServerOnRespawn=true`.
  - `JoinSnapDelaySeconds=0.2`.
  - `NormalMoveSpeed=600`, `SprintMoveSpeed=850`, `ADSMoveSpeed=400`.
  - `bAutoCalibrateServerSnapshotOrigin=true`.
  - `bUseSnapshotZForServerPlayerGhosts=true`.
  - `ServerPlayerGhostZOffset=0`.
  - `bUseLocalPawnZForOwnServerGhost=false` unless you are debugging server Z lag.
  - `bShowServerProjectileGhosts=true`.
  - `bUseServerAuthoritativeFireVisuals=true`.
  - `bSpawnLegacyLocalProjectileWhenConnected=false`.
  - `bAllowLegacyLocalProjectileDamageWhenConnected=false`.
  - `bSpawnLegacyLocalProjectileWhenOffline=true`.
  - Gameplay HUD optional widgets are arranged for recording:
    - `HealthText` / `HealthBar` at the bottom center for HP.
    - `AmmoText` near HP for ammo/reload, or use the `ScoreText` fallback.
    - `MatchText` and `SmallServerStatusText` as compact status text.
    - `RankingText` at the top right.
    - `KillFeedLine1` through `KillFeedLine5` on the left side.
    - `DebugText` hidden unless `bShowDebugHud=true`.
  - Dynamic crosshair Border widgets are present, or the fallback `CrosshairText` is acceptable for the current recording.
- If humanoid assets are assigned for the recording:
  - Player Skeletal Mesh appears correctly in `BP_BattleGridCharacter`.
  - `PlayerMeshRelativeLocation`, `PlayerMeshRelativeRotation`, and `PlayerMeshRelativeScale3D` align the player mesh with the capsule.
  - WASD movement, mouse look, ADS, sprint, and jump still work with the humanoid mesh assigned.
  - Crosshair stays centered and the over-the-shoulder camera is not blocked by the player body.
  - Player weapon mesh is attached to the intended hand socket.
  - Optional player Idle/Run/JumpStart/JumpLoop/JumpLand animations play only if `bUseSimplePlayerAnimationPlayback=true`.
  - Short `Jump_Apex` clips are assigned to `PlayerJumpLoopAnimation` with `bLoopPlayerJumpLoopAnimation=false` and do not restart rapidly in air.
  - Bot humanoid mesh appears in `BP_BattleGridServerBotGhostActor` when `bUseSkeletalMeshVisual=true`.
  - Bot weapon mesh is attached and bot labels remain readable.
  - Bot idle animation is assigned if simple animation playback is enabled.
  - Bot run animation is assigned and switches while the ghost moves.
  - Bot death/down animation or down scale is readable when the bot dies.
  - `HumanoidMeshRelativeLocation`, `HumanoidMeshRelativeRotation`, and `HumanoidMeshRelativeScale3D` are tuned so the bot faces forward and stands on the ground.
  - `bFaceMovementDirection=true`, `bUseServerYawWhenNotMoving=true`, and `MeshForwardYawOffset` is tuned so Murdock runs forward instead of sideways.
  - Player and bot muzzle fallback offsets look reasonable if real muzzle sockets are not present yet.
  - `bShowMuzzleDebug` was enabled briefly to verify muzzle placement, then disabled before recording.
  - PlayerController `bSpawnLocalProjectileFromMuzzle=true` and `bUseClientMuzzleForServerTracerStart=true`.
  - PlayerController `bShowServerShotDebug=false` for recording, but it can be enabled briefly to verify the fire ray when local pawn and SERVER ECHO are separated.
  - Server projectile ghost has visually distinct player/bot materials if materials were created locally.
  - Assigned Animation Blueprints prevent visible T-pose during the demo.

## During Recording

- Show the browser test page applying safe demo mode.
- Show `Fire 5 Shots At Nearest Bot` and verify `SERVER HIT` / bot HP changes.
- Show Unreal connected with `Profile: Local` or `Profile: Remote`.
- Show SERVER ECHO, bot ghosts, and HPACK ghosts. If debugging bot placement, briefly enable `bDrawServerBotAreaBounds` and verify bots are inside the green bot-area rectangle.
- If humanoid assets are assigned, show the local player mesh and humanoid bot ghosts without changing the server gameplay explanation.
- Show the crosshair tightening in ADS and widening during sprint/jump.
- Shoot a bot in Unreal and show `SERVER HIT` or `SERVER MISS` in the HUD.
- Verify the normal gameplay HUD is clean:
  - HP and ammo are bottom center.
  - Top 5 ranking is top right.
  - Kill feed is left side.
  - Crosshair is centered.
  - Large debug text and controls help are hidden.
- Verify SERVER ECHO remains close to the local pawn while walking into visible Unreal walls; it should stop with the pawn instead of continuing through the wall.
- Jump and verify SERVER ECHO follows the local pawn height.
- Stand on any available ramp or raised platform and verify SERVER ECHO height follows.
- If SERVER ECHO is visibly offset from the local pawn, shoot a clearly selected bot and verify the server log uses `Fire origin ... source=client`.
- Hold Tab to show the server scoreboard.
- Mention that connected PvPvE fire feedback is server-authoritative: server tracer ghosts plus `SERVER HIT` / `SERVER MISS`. The old local sphere projectile is kept for offline testing only.

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
11. In the PlayerController Blueprint defaults, use the remote profile and confirm the gameplay HUD shows compact server HP, ammo, match status, Top 5 ranking, and kill feed with debug text hidden.
12. Move with WASD.
13. Show the local character and the server player ghost moving together.
14. Fire with left mouse button and show the server tracer ghost plus `SERVER HIT` / `SERVER MISS`; the legacy local sphere projectile should not be visible while connected and joined.
15. Confirm server target/core ghosts are hidden by default for the kill race demo.
16. Hit a local target only after switching to offline/local test mode if you want to explain the legacy layer.
17. Validate the dynamic crosshair:
   - idle hip fire: medium gap
   - hold RMB ADS: tight gap
   - hold Shift sprint: wider gap
   - jump/fall: widest gap
   - reload or server dead: dimmed/unavailable color if the optional Border widgets are present
18. Hit a server bot and show the explicit server shot result:
   - Browser `Shot Result`: `SERVER HIT BOT-* -20`, `SERVER HEADSHOT BOT-* -40`, or `SERVER MISS`.
   - Unreal HUD hit marker text: `SERVER HIT ...` or `SERVER MISS` for about 1.25 seconds by default.
   - Server kills / HP changes in the HUD and browser summaries when the hit deals damage.
19. Validate visual placeholder readability:
   - `SERVER ECHO` label is visible on the server player ghost.
   - `BOT-*` labels are visible and dead bots read as `DOWN`.
   - `HPACK-*` labels are visible and inactive packs show a countdown.
   - local objects and server ghost objects are visually distinguishable.
   - player-fired and bot-fired tracers are visually distinguishable if `PlayerProjectileMaterial` and `BotProjectileMaterial` are assigned
20. Validate the arena layout: P1/P2/P3/P4 spawns sit on the four sides, bots spawn inside the smaller demo bot area, and health packs are near the perimeter.
21. Verify the server combat event feed appears after a bot/player event and that shot results are distinct from local projectile overlap logs.
22. Hold Tab and verify the server scoreboard overlay appears; release Tab and verify the normal HUD returns.
23. Optional: enable bot attacks on easy to show server shooter danger after the stable shooting demo. Bots should fire low-accuracy hitscan shots rather than applying proximity damage.
24. While moving on easy, verify the player can usually survive at least 15-30 seconds. If the player stands still in range, bots should still be able to kill the player and produce a `bot_killed_player` event.
25. Walk into the hazard and show local HP decrease, death, and respawn only as an offline/local debug layer.
26. Toggle `bShowLocalDebugHud` if you need to show local HP/score and position error during explanation.
27. Toggle optional server position correction on/off if useful for the recording.
28. Stop the server or switch back to the Local profile and show that offline gameplay still runs.

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
11. Confirm `use_client_fire_origin_for_hitscan=true`, `client_fire_origin_warning_distance=300`, and `max_accepted_client_fire_origin_distance=2000`.
12. Confirm `use_client_position_for_player_movement=true`, `max_client_position_delta_per_second=1400`, and `max_client_position_snap_distance=3000`.
13. Confirm `bot_area_bounds` is approximately `x=-1500..1500`, `y=-900..900` and `bot_waypoints` count is `13`.
14. Confirm `walk_speed=600`, `sprint_speed=850`, and `ads_walk_speed=400`.
15. Click `Send Fire At Nearest Bot` and verify the `Shot Result` card shows `SERVER HIT BOT-* -20` or `SERVER MISS`.
16. Click `Send Headshot At Nearest Bot` and verify the `Shot Result` card shows `SERVER HEADSHOT BOT-* -40`.
17. Click `Fire 5 Shots At Nearest Bot` and verify bot HP drops quickly. A 100 HP bot should be killed by five body hits at 20 damage each unless shots miss or the match is over.
18. Verify the combat tuning summary shows player body/head damage `20/40`, bot body/head damage `10/20`, easy bot difficulty, pressure `target/shoot=8/2`, respawn invincibility `2.5s`, bot attacks disabled, timer auto-end disabled, kill score values `1/1`, and client fire origin enabled.
19. Verify the arena layout values from the first snapshot output:
   - bounds `x=-5000..5000`, `y=-5000..5000`
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
3. Click `Send Fire At Nearest Bot` and watch bot HP plus the `Shot Result` card update with a body shot, usually `SERVER HIT BOT-* -20`.
4. Click `Send Headshot At Nearest Bot` and verify the card shows `SERVER HEADSHOT BOT-* -40`.
5. Click `Fire 5 Shots At Nearest Bot` and watch bot HP decrease across multiple snapshots, then confirm a kill event and scoreboard kill update when the bot reaches 0 HP.
6. Watch bot HP and server kills change after server hits.
7. Click `Enable Bot Attacks` to test Bot Shooter AI v2.
8. Stand near a bot and verify bot shot events appear as `BOT-* hit player* -10`, `BOT-* headshot player* -20`, or `BOT-* killed player*`.
9. Click `Send Debug Room` and verify bots show readable combat states such as `shoot`, `aim`, `suppressed`, `chase`, `wander`, and `reload`.
10. On easy, keep moving and verify the bot pressure is readable: nearby bots face/aim at the player, only about 1-2 bots fire at the same time, and respawn invincibility gives recovery time.
11. Confirm projectile snapshots include `owner_type`, `visual_only`, `start_*`, and `end_*` fields.

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
   - Top-right `TOP 5` ranking when the optional `RankingText` widget is present, or a fallback ranking block in the HUD text.
   - Server connection.
   - Profile label, either `Profile: Local` or `Profile: Remote`.
   - Match time and kill goal.
   - Bot count.
   - Health pack count.
   - Server projectile count.
   - Recent combat event feed.
   - Left-side kill log when optional `KillFeedLine1` through `KillFeedLine5` widgets are present, or fallback kill log text in the HUD.
   - Dynamic crosshair when optional `CrosshairTop`, `CrosshairBottom`, `CrosshairLeft`, `CrosshairRight`, and `CrosshairCenter` Border widgets are present.
   - Temporary server hit marker text after shots: `SERVER HIT ...`, `SERVER HEADSHOT ...`, or `SERVER MISS`.
   - Optional local debug line only when `bShowLocalDebugHud` is enabled.
   - Position error and correction state.
7. Move with WASD.
8. On the first server snapshot after join, verify the local pawn snaps to the own server spawn and SERVER ECHO stays close.
9. Verify `bSendClientPositionToServer=true`; with local debug HUD enabled, server position error should remain small without hard or gentle correction.
10. Jump and verify SERVER ECHO follows the local pawn height instead of staying at fixed Z.
11. Stand on a ramp or raised platform if the current map has one and verify SERVER ECHO height follows.
12. Walk into a visible Unreal wall/obstacle and verify SERVER ECHO stops with the local pawn instead of continuing forward.
13. Hold Shift and RMB separately; with local debug HUD enabled, verify server position error does not steadily grow during sprint or ADS walk.
14. If movement feels blocked by an invisible wall with correction enabled, temporarily enable `bDrawServerArenaBounds` and verify the cyan server rectangle matches the intended playable area.
15. Aim with the mouse.
16. Fire with left mouse button.
17. Hold RMB ADS and verify the crosshair gap tightens.
18. Hold Shift sprint and verify the crosshair gap widens.
19. Jump/fall and verify the crosshair reaches its widest state.
20. Reload or enter server-dead state and verify the crosshair dims if the Border widgets are present.
21. Confirm connected server mode does not show the legacy local sphere projectile by default.
22. Disconnect or switch to offline/local mode only if you need to show legacy local projectile and local target damage.
23. Show local player hazard damage, death, and respawn only if explaining offline/local debug behavior.
24. Show server player ghost movement.
25. Show server tracer/projectile ghosts for player shots.
26. If testing tracer alignment, briefly enable muzzle debug and verify the player and bot debug spheres sit near the weapon/muzzle area, then turn debug off before recording.
27. In ADS, fire at a visible crosshair target and verify the server tracer aligns with the crosshair direction.
28. If ADS alignment is wrong, briefly enable PlayerController `bShowAimDebug` and character `bShowMuzzleDebug`; the camera aim line and muzzle-to-aim line should converge at the same point.
29. Confirm server target/core ghosts are hidden by default.
30. Show server kills increasing when server bots are killed.
31. Verify the top-right ranking updates after a bot kill and highlights the local player with `YOU`.
32. Kill a bot and verify the kill log shows your kill in green.
33. If useful, enable easy bot attacks and let a bot kill the player; verify the kill log shows that player death in red.
34. Verify only the most recent 5 kill log lines remain visible.
35. Aim at a bot body and verify `SERVER HIT BOT-* -20`.
36. Aim at a bot head and verify `SERVER HEADSHOT BOT-* -40`; if needed, enable the bot ghost `BattleGrid|Debug > bShowServerHitVolumes` temporarily.
37. Aim between two nearby bots and verify only the closest valid ray hit loses HP.
38. If hit selection is unclear, temporarily enable PlayerController `BattleGrid|Debug > bShowServerShotDebug` and bot ghost `BattleGrid|Debug > bShowServerHitVolumes`, then compare the purple shot ray against the cyan/magenta server hit spheres.
39. Validate polished server placeholder labels:
   - player ghost: `SERVER ECHO P#`
   - bot ghost: `BOT-# hp/max`, `BOT-# INV`, or `BOT-# DOWN`
   - health pack ghost: `HPACK-# +35` or `HPACK-# Ns`
40. Optional humanoid visual checks:
   - player mesh aligns with the capsule
   - player mesh faces forward while moving
   - player simple Idle/Run/JumpStart/JumpLoop/JumpLand animations work if enabled
   - Jump_Apex or the air pose does not restart every frame while falling
   - ADS view remains usable and does not clip into the shoulder/body
   - player weapon is attached to the right hand or selected socket
   - player muzzle socket or fallback offset is plausible for future muzzle flash alignment
   - bot ghost skeletal mesh appears when `bUseSkeletalMeshVisual=true`
   - bot weapon follows the bot ghost
   - bot idle animation plays while standing or moving very slowly
   - bot run animation plays while chasing or wandering
   - bot actor faces its movement direction while moving
   - stationary or attacking bot uses server-facing yaw without snapping sideways
   - `MeshForwardYawOffset` corrects only the humanoid mesh local forward direction
   - bot down/death visual appears when HP reaches 0
   - bot muzzle socket or fallback offset is plausible
   - `BOT-*` labels stay above the head and remain readable
   - no visible T-pose if an AnimBP is assigned
41. Validate the server layout against the Unreal map:
   - player ghosts remain inside the intended arena bounds
   - bot ghosts start inside the smaller demo bot area
   - when `bDrawServerBotAreaBounds=true`, the green bot-area rectangle surrounds the bot spawns and wander area
   - health pack ghosts appear near outer lanes
42. Switch from local mode to remote mode and show the HUD profile label changing.
43. Show server ghosts driven by the remote GCP server.
44. Hold Tab and verify the detailed server scoreboard overlay appears. It should also show automatically when the server match is `game_over`.
45. Toggle optional server position correction in the PlayerController Blueprint defaults if needed, then compare error behavior.
46. Stop the server and verify local offline gameplay still works.

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
