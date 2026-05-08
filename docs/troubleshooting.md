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

## Health Pack Does Not Heal

Check:

- The player is alive and below max HP. Full-health players do not consume server health packs.
- The health pack snapshot has `active=true`.
- The player is within the server pickup radius, currently `120`.
- Client position sync is enabled so the server player position matches the local pawn position.
- The browser `healthPacksSummary` shows the pack as `H#:+35`, not a countdown.
- `debug_room` reports `health_pack_pickup_radius=120` and `health_pack_respawn_seconds=15`.

When pickup succeeds, the event feed should include `health_pack_picked`, the local HUD should briefly show `HEALED +35`, and the server HP snapshot should increase.

## Health Pack Visual Stays Active After Pickup

Check:

- The snapshot `health_packs[*].active` flag changed to `false`.
- The health pack ghost actor is receiving updated snapshots.
- `bShowServerHealthPackGhosts=true` on the PlayerController.
- The inactive ghost label should read `HPACK-# Ns` and use the smaller/dim inactive visual settings.

If the server snapshot still says `active=true`, the player may be full HP, outside pickup radius, dead, or not sending client position to the server.

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
- Or, for bot ghosts, enable `bUseSimpleBotAnimationPlayback` and assign compatible `BotIdleAnimation`, `BotRunAnimation`, and `BotDeathAnimation` assets.
- Retarget animations if the imported asset uses a different skeleton.
- Verify the Anim Class is saved on the active Blueprint used by the level.
- For a demo, avoid enabling humanoid bot visuals until animation and scale are acceptable.

## Player Model Visible But Not Animated

The player mesh can use either an Animation Blueprint or the simple C++ animation playback path.

Check:

- If using an AnimBP, assign the Anim Class on `BP_BattleGridCharacter`'s inherited `Mesh` component and keep `bUseSimplePlayerAnimationPlayback=false`.
- If using simple playback, enable `bUseSimplePlayerAnimationPlayback` and assign compatible `PlayerIdleAnimation`, `PlayerRunAnimation`, `PlayerJumpStartAnimation`, `PlayerJumpLoopAnimation`, and `PlayerJumpLandAnimation`.
- Assigned animation assets must use the same skeleton as the player Skeletal Mesh.
- Rebuild C++ and restart Unreal if the new `BattleGrid|Animation` properties are not visible.

## Jump Animation Repeats Rapidly In Air

This happens when a short apex clip is used as the only jump animation and gets restarted while `CharacterMovement->IsFalling()` remains true.

Fix:

- Assign `Jump_Start` to `PlayerJumpStartAnimation`.
- Assign `Jump_Apex` to `PlayerJumpLoopAnimation`.
- Assign `Jump_Land` to `PlayerJumpLandAnimation`.
- Set `bLoopPlayerJumpLoopAnimation=false` for short apex clips.
- Leave `PlayerJumpLoopAnimation` empty if the apex pose looks bad and no proper looping/falling animation exists yet.

The simple player animation path now changes jump state only on takeoff, first airborne loop/apex, and landing. It does not replay the same `JumpLoop` state every tick.

## Player Model Rotated Sideways

Tune `BP_BattleGridCharacter` Class Defaults under `BattleGrid|Visual`:

- Start with `PlayerMeshRelativeRotation = (0, -90, 0)`.
- Try yaw values `90`, `-90`, `180`, or `0` depending on the asset.
- Keep actor/controller rotation logic unchanged; use mesh relative rotation for asset orientation only.

## Player Model Floats Or Sinks

Tune `PlayerMeshRelativeLocation.Z`.

Recommended starting value:

```text
PlayerMeshRelativeLocation = (0, 0, -90)
```

The Character capsule is still the gameplay collision shape. Do not move the capsule just to fix visual foot placement.

## Camera Clips Into Player Model

If the camera clips into the player body or shoulder:

- Increase `AdsSocketOffset.Y` to move the ADS camera farther over the shoulder.
- Increase `AdsArmLength` slightly.
- Reduce `PlayerMeshRelativeScale3D`.
- Tune `DefaultSocketOffset` / `AdsSocketOffset`.
- Increase `PlayerCameraCollisionProbeSize` slightly if camera collision feels too tight.

Do not change server position correction or movement settings to fix camera clipping.

## Murdock Bot Is T-Pose

For `BP_BattleGridServerBotGhostActor`, check:

- `bUseSkeletalMeshVisual=true`.
- `SkeletalMeshComponent` has the Murdock Skeletal Mesh assigned.
- If using simple playback, `bUseSimpleBotAnimationPlayback=true`.
- `BotIdleAnimation`, `BotRunAnimation`, and `BotDeathAnimation` use the same skeleton as the assigned Murdock mesh.
- If using an Animation Blueprint instead, leave simple playback disabled so the AnimBP owns the pose.

## Murdock Bot Slides

The ghost is moving, but the visible mesh is not switching to a run animation.

Check:

- `BotRunAnimation` is assigned.
- `bUseSimpleBotAnimationPlayback=true`.
- `BotRunSpeedThreshold` is not too high. Start with `20`.
- The selected run/jog animation uses the same skeleton as the Skeletal Mesh.

If no run asset is ready, sliding is expected during setup. Server movement and hit tests are still working.

## Bot Slides Sideways With Run Animation

If the run animation plays but the bot appears to strafe sideways, the issue is usually visual orientation, not server movement.

Common causes:

- Server yaw was raw arena yaw instead of converted Unreal yaw.
- Actor rotation was following snapshot yaw instead of visible movement.
- The skeletal mesh local forward axis does not match the actor forward axis.

Fix:

- In `BP_BattleGridServerBotGhostActor`, enable `bFaceMovementDirection`.
- Keep `bUseServerYawWhenNotMoving` enabled for stationary/combat facing.
- Start with `RotationInterpSpeed=12` and `MovementFacingThreshold=5`.
- Tune `MeshForwardYawOffset` first. Try `0`, `90`, `-90`, and `180`.
- If the whole model is also tilted, floating, or globally rotated, tune `HumanoidMeshRelativeRotation` after the forward offset.

`MeshForwardYawOffset` only changes the skeletal mesh relative rotation. It should not be used to replace actor rotation.

## Murdock Bot Faces Wrong Direction

Tune the bot skeletal mesh transform offsets in `BP_BattleGridServerBotGhostActor`:

- `HumanoidMeshRelativeRotation`: adjust yaw, usually `90`, `-90`, or `180`.
- `MeshForwardYawOffset`: adjust final local-forward correction, usually `0`, `90`, `-90`, or `180`.
- `HumanoidMeshRelativeLocation`: adjust if feet float above or sink below the floor.
- `HumanoidMeshRelativeScale3D`: adjust asset-specific scale.

These are visual-only offsets. They do not change server `BotState` position, damage, or hit volumes.

## Bot Label Is Inside The Humanoid Mesh

When bot ghost skeletal visuals are enabled, tune:

- `HumanoidLabelHeight`
- `HumanoidAliveScale`
- `HumanoidDeadScale`

These settings live on `BP_BattleGridServerBotGhostActor` under `BattleGrid|Visual`. They only affect visualization and do not change server hit volumes.

If the label overlaps the body after changing mesh scale, raise `HumanoidLabelHeight`. Dead bots still show `BOT-* DOWN`, and invincible bots still show `BOT-* INV`.

## Tracer Does Not Appear From The Weapon Area

Server tracer records are visual-only. Unreal can now override the visible tracer start with a local muzzle approximation while keeping server hitscan damage unchanged. The C++ classes expose muzzle helpers for alignment work:

- `ABattleGridClientCharacter`: `MuzzleSocketName`, `MuzzleFallbackOffset`
- `ABattleGridServerBotGhostActor`: `MuzzleSocketName`, `MuzzleFallbackOffset`
- `ABattleGridServerGhostActor`: `MuzzleFallbackOffset` for remote server player echoes

Check:

- The weapon or character skeleton has a socket such as `Muzzle`, `weapon_r`, `hand_r`, or `hand_rSocket`.
- If no muzzle socket exists, tune the fallback offset in the Blueprint Class Defaults.
- Temporarily enable `bShowMuzzleDebug` and tune the offset until the debug sphere sits near the weapon barrel.
- In the active PlayerController Blueprint, keep `bSpawnLocalProjectileFromMuzzle=true` for local visual shots.
- Keep `bUseClientMuzzleForServerTracerStart=true` if server tracer ghosts should visually begin at the local player, bot ghost, or remote server player ghost muzzle approximation.
- `BP_BattleGridServerProjectileGhostActor` has `bUseTracerLineVisual=true`.
- `PlayerProjectileMaterial` and `BotProjectileMaterial` are assigned if you need different tracer colors.
- Server snapshots include `visual_only=true`, `start_x/y/z`, and `end_x/y/z` for projectiles.

Tracer ghosts do not apply damage. Server shot result events and bot/player HP changes are the authoritative combat result.

If the tracer starts from the chest or body, increase/decrease `MuzzleFallbackOffset.X` and `MuzzleFallbackOffset.Z`. If it starts on the wrong side, adjust `MuzzleFallbackOffset.Y`. For humanoid bot ghosts, also check `MeshForwardYawOffset`; the fallback uses the skeletal mesh transform when skeletal visual mode is active.

## ADS Tracer Goes Down-Left Or Away From The Crosshair

In third-person ADS, the camera and weapon muzzle are offset from each other. If the visual projectile uses camera forward directly from the muzzle, it travels along a parallel line and appears to miss the screen-center crosshair.

Expected behavior after this ADS alignment fix:

- The PlayerController deprojects the center of the viewport.
- It traces from the camera through the crosshair to find an aim point.
- The local visual projectile starts at `GetApproximateMuzzleWorldLocation()`.
- The local visual projectile points from muzzle to that crosshair aim point.
- The server shot direction is still crosshair-derived and keeps weapon spread.

To debug:

- In the active PlayerController Blueprint, enable `BattleGrid|Weapon Visual > bShowAimDebug`.
- In `BP_BattleGridCharacter`, enable `bShowMuzzleDebug`.
- Verify the camera debug line hits the crosshair target and the green muzzle line converges to the same point.
- Keep `bSpawnLocalProjectileFromMuzzle=true`.
- Tune `MuzzleFallbackOffset` only after confirming the aim debug line is correct.

## Server Projectile Ghost Tracer Goes The Wrong Direction

If disabling `Show Server Projectile Ghosts` makes the bad yellow line disappear, the issue is the server projectile ghost/tracer visualization, not the local sphere projectile.

Expected behavior after the server projectile ghost alignment fix:

- Local projectile direction and local projectile behavior are unchanged.
- Server projectile ghost start/end are built together in final Unreal world space.
- If a muzzle override is used for a local player, bot, or remote server player ghost, the tracer end is recomputed as `start + converted server shot direction * tracer length`.
- The tracer no longer mixes a local muzzle start with an old converted server endpoint.
- `ConvertServerDirectionToUnrealDirection` respects `ServerAimSignX` and `ServerAimSignY`, matching the direction signs used when sending shot input to the server.

To debug:

- Enable PlayerController `BattleGrid|Server Snapshot > bShowProjectileGhostDebug`.
- The debug view draws green start, red end, and yellow tracer line.
- Check the Output Log for `Server projectile visual owner=... start=... end=...`.
- If the server tracer is still mirrored, check `ServerAimSignX` and `ServerAimSignY` on the active PlayerController Blueprint.

## Why Does The Old Sphere Bullet Differ From The Yellow Tracer?

BattleGrid has two projectile visual layers:

- Legacy local sphere projectile: the original offline/local test projectile. It can collide with local test targets.
- Yellow/server tracer ghost: the server-authoritative shot visual created from server projectile snapshots. It represents server hitscan fire and is paired with `SERVER HIT`, `SERVER HEADSHOT`, or `SERVER MISS`.

In connected PvPvE server mode, the server tracer is the primary visual. The active PlayerController should normally use:

- `bUseServerAuthoritativeFireVisuals=true`
- `bSpawnLegacyLocalProjectileWhenConnected=false`
- `bAllowLegacyLocalProjectileDamageWhenConnected=false`
- `bSpawnLegacyLocalProjectileWhenOffline=true`

With those defaults, the old sphere bullet is not spawned while connected and joined, so it cannot visually disagree with the server tracer or damage local test targets during a server demo. If you intentionally enable `bSpawnLegacyLocalProjectileWhenConnected` for debugging, keep `bAllowLegacyLocalProjectileDamageWhenConnected=false` unless you specifically want to test the old local damage layer.

## Local Projectile Damages BP_BattleGridDamageableTarget In Server Mode

`BP_BattleGridDamageableTarget` is an offline/debug target actor. It should not contribute to the connected PvPvE demo.

Check:

- Active PlayerController has `bUseServerAuthoritativeFireVisuals=true`.
- Active PlayerController has `bSpawnLegacyLocalProjectileWhenConnected=false`.
- Active PlayerController has `bAllowLegacyLocalProjectileDamageWhenConnected=false`.
- The target actor has `bAllowDamageInServerMode=false`.

Expected behavior:

- While connected and joined, the legacy sphere projectile is not spawned by default.
- If a legacy projectile exists for debugging, local damageable targets ignore damage unless the target is explicitly allowed to take server-mode damage.
- Server combat should be verified through `SERVER HIT`, `SERVER HEADSHOT`, bot/player HP changes, ranking, and kill feed events.

## Local Player And SERVER ECHO Drift Apart

The local pawn uses Unreal movement and collision. The server does not know the Unreal map collision, so pure input simulation can let `SERVER ECHO` continue through a wall after the local pawn has stopped. For the current prototype/demo, the Unreal client can send its local pawn position to the server so `PlayerState` follows the actual visible character.

Check:

- In the active PlayerController Blueprint, keep `bAutoCalibrateServerSnapshotOrigin=true`.
- Keep `bSendClientPositionToServer=true` for the demo.
- Keep `bSnapLocalPawnToServerOnJoin=true` and `bSnapLocalPawnToServerOnRespawn=true`.
- Use movement speeds `NormalMoveSpeed=600`, `SprintMoveSpeed=850`, and `ADSMoveSpeed=400`.
- Rebuild and restart the server so the server also uses `walk_speed=600`, `sprint_speed=850`, and `ads_walk_speed=400`.
- In `debug_room`, inspect `use_client_position_for_player_movement=true`, `latest_input.has_client_position=true`, `latest_input.client_x/y/z`, and `effective_speed`.
- Enable `bShowLocalDebugHud` temporarily and watch `Server Error` plus `Correction`.

Correction modes:

- `Off`: no continuous correction, best default for avoiding running-in-place.
- `Gentle`: optional interpolation only when error exceeds `GentleCorrectionThreshold`.
- `Hard`: legacy correction using `bUseServerPositionCorrection`; useful for debugging large desync, but can feel aggressive.

If client-position sync is disabled, error can still grow while holding Shift or RMB if the client and server speed settings are out of sync. If the error points in a wrong direction, inspect the `Move send local_axes=... server_move=...` log and confirm camera-relative movement is being converted to server axes.

## SERVER ECHO Follows X/Y But Not Height

Client-position sync sends `client_z` so the server snapshot can follow the local pawn's Unreal world height. If `SERVER ECHO` stays at a fixed height while the player jumps, walks up ramps, or stands on a raised platform, check:

- Active PlayerController `BattleGrid|Server Movement > bSendClientPositionToServer=true`.
- Active PlayerController `BattleGrid|Server Snapshot > bUseSnapshotZForServerPlayerGhosts=true`.
- `debug_room.players[].latest_input.has_client_position=true`.
- `debug_room.players[].latest_input.client_z` changes when the local pawn changes height.
- Snapshot `players[].z` changes after the server accepts the client position.
- If the ghost is consistently too high or too low, tune `ServerPlayerGhostZOffset`.
- For visual-only debugging, `bUseLocalPawnZForOwnServerGhost=true` can make the own `SERVER ECHO` use local pawn Z even before trusting server Z.

The server still does not simulate terrain or platform physics. Z sync is prototype movement alignment, not production authoritative vertical movement.

## Invisible Walls While Correction Is Enabled

If hard or gentle server position correction is enabled, the local pawn can feel like it hits an invisible wall when the server `PlayerState` reaches the server arena clamp before the visible Unreal map does. Correction then pulls the pawn back to the authoritative server position. Client-position sync should avoid this in the demo because the server follows the local pawn instead of simulating through missing server collision.

Fix:

- Keep `bUseServerPositionCorrection=false` for normal demo recording.
- Keep `bUseGentleServerPositionCorrection=false` unless you are actively testing smoothing.
- Keep `bSendClientPositionToServer=true` so the server ghost follows the Unreal pawn without correction.
- If correction is needed, enable only gentle correction first. If both correction modes are enabled, the client uses gentle correction and logs a warning.
- Enable PlayerController `BattleGrid|Debug > bDrawServerArenaBounds` to draw the cyan server arena rectangle in Unreal.
- Check `debug_room` or snapshots for `arena.bounds`; the demo server default is now `x=-5000..5000`, `y=-5000..5000`.
- If the cyan rectangle still does not match the visible map, adjust the Unreal map layout manually or update the server arena constants for the intended demo area.

Recommended correction defaults:

- `bUseServerPositionCorrection=false`
- `bUseGentleServerPositionCorrection=false`
- `GentleCorrectionThreshold=300`
- `GentleCorrectionInterpSpeed=2`
- `HardCorrectionThreshold=2500`

## Bots Outside The Visible Demo Map

The player arena bounds are intentionally wide for the demo, but bots use a smaller server bot area so they stay near the current visible map.

Check:

- Click `Apply Safe Visual Demo` so bots reset to the current demo spawn list.
- `debug_room.bot_area_bounds` should read approximately `x=-1500..1500`, `y=-900..900`.
- `debug_room.bot_waypoints` should contain 13 fixed waypoints.
- Bot spawns should be the eight symmetric demo positions from `BOT-1 (-900,500)` through `BOT-8 (900,-500)`.
- In Unreal, enable PlayerController `bDrawServerBotAreaBounds` to draw the green server bot area rectangle.
- If bots appear too far out, tune the server `BotAreaMinX`, `BotAreaMaxX`, `BotAreaMinY`, and `BotAreaMaxY` constants rather than shrinking the player arena bounds.
- The server still does not know Unreal collision or NavMesh. These bounds and waypoints are a prototype constraint, not final navigation.

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
- Keep `bVerboseBattleGridLogs=false`.
- Keep `bVerboseNetworkLogs`, `bVerboseSnapshotLogs`, and `bVerboseInputLogs` false.
- Keep `SnapshotLogInterval` and `InputAckLogInterval` at `60` or higher.

Important logs such as server profile, connect, connection error, join, preset-on-join, victory, respawn, reload start/finish, and actual errors should still appear. Repetitive input, input ack, snapshot, coordinate conversion, client-position sync, fire-origin, and server position error logs are hidden unless verbose logging or a specific debug draw flag is enabled.

## Too Many Server Logs

The demo server defaults keep repetitive logs quiet:

- `verbose_input_logs=false`
- `verbose_hitscan_candidate_logs=false`
- `verbose_bot_state_logs=false`
- `bot_shot_events_verbose=false`

Use `Send Debug Room` in the browser to confirm these values. Important logs such as server startup, room bounds, joins, preset application, match restart, kills, deaths, health pickups, invalid JSON, and warnings still appear. Per-input logs, input acknowledgements, selected hitscan candidate logs, normal body-shot spam, and bot combat-state transition spam are hidden by default.

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
   - `bot_fire_interval=1.0`
   - `bot_aim_spread=18`
   - `max_bots_targeting_one_player=8`
   - `max_bots_shooting_one_player=2`
   - `respawn_invincible_seconds=2.5`

The equivalent JSON commands are:

```json
{ "type": "debug_set_bot_difficulty", "difficulty": "easy" }
```

```json
{ "type": "debug_set_bot_attacks", "enabled": false }
```

If you still want bots to shoot but need a playable demo, keep attacks enabled on easy. Easy mode deliberately limits pressure: nearby bots can still aim at the player, but only a small number can fire during the same short window, bot shots have wider spread, and bot fire cooldowns include small jitter so volleys do not land together.

If easy is still too punishing during recording:

- Keep moving; standing still should still be dangerous.
- Click `Disable Bot Attacks` for visual-only movement tests.
- In code, reduce `MaxBotsShootingOnePlayer`, increase `botAimSpreadDegrees`, or increase `BotShotRandomDelayMax`.

## Bot In Front Of Me Does Not Shoot

Bot shooting can be blocked for valid debug reasons even when the bot sees the player.

Use `tools/websocket-test.html`:

1. Click `Send Debug Room`.
2. Check the `Bots` summary or the `bot_states` log line.
3. Look for each bot's `combat_state` and `fire_block_reason`.

Common states and reasons:

- `shoot`: bot is firing this tick.
- `aim` + `cooldown`: bot sees the player but its weapon is waiting on fire cooldown.
- `aim` + `bot_attacks_disabled`: safe/demo setting prevents damage.
- `suppressed` + `shooting_cap`: bot sees and faces the player, but the per-player shooting cap is already full.
- `reload` + `reloading` or `no_ammo`: bot is reloading its 30-round magazine.
- `chase` + `out_of_attack_range`: bot sees the player but is not close enough to fire.
- `wander` + `out_of_detect_range`: no valid player is inside detect range.
- `wander` + `target_invincible`: the player is alive but currently protected after respawn.

In the current demo tuning, easy mode uses `MaxBotsTargetingOnePlayer=8` so nearby bots should generally react, face, chase, aim, or suppress instead of wandering away. `MaxBotsShootingOnePlayer=2` keeps the player from being deleted by every bot at once.

## Game Starts In `game_over`

This usually means the previous server match reached the kill goal or the timer expired. For demo iteration:

1. Click `Apply Safe Visual Demo`, `Apply Combat Demo`, or `Apply Match Demo`.
2. Click `Send Debug Room`.
3. Confirm `match.state=in_progress`, `match.game_over=false`, and `current_demo_preset` matches the button you clicked. For `safe_visual`, bot attacks and timer auto-end should be false; for `combat_demo`, bot attacks should be true and timer auto-end false; for `match_demo`, both bot attacks and timer auto-end should be true.

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

## Too Many Manual Debug Buttons

Use one-click demo presets instead of manually combining Safe Demo Mode, bot attack toggles, difficulty, timer, and restart:

- `safe_visual`: visual/HUD/model pass, bot attacks off.
- `combat_demo`: playable bot combat pass, bot attacks on, timer off.
- `match_demo`: match-flow pass, bot attacks on, timer on.
- `debug_visual`: bounds/ghost visual debug pass, bot attacks off.

In the browser, click the matching preset button and then `Send Debug Room`. In Unreal, use `BattleGrid|Demo > bApplyDemoPresetOnJoin=true` and set `DemoPresetOnJoin` to the desired preset.

## Winner P0

`Winner: P0` means an old server/client path invented a winner when no real connected player had a nonzero kill-race score.

Expected behavior after the match-flow cleanup:

- A real winner shows `GAME OVER`, `Winner: player*`, and `Score: N`.
- If everyone has zero kills, the server sends `winner_player_id=0`, `winner_score=0`, `is_draw=true`, and `no_winner=true`.
- Browser and Unreal should display `Draw` / no winner, not `Winner: P0`.

Fix:

1. Rebuild/restart the server container so the new match fields are active.
2. Rebuild the Unreal client C++ so `GetGameOverText()` parses `is_draw` / `no_winner`.
3. Click a demo preset button or `Debug Restart Match`.
4. Click `Send Debug Room` and confirm `match_id` changed and `game_over=false`.

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

The Tab scoreboard can be held manually at any time. It only auto-shows on `game_over` if the active PlayerController has `BattleGrid|HUD > bAutoShowScoreboardOnGameOver=true`; the default is false so the center game-over result and top-right ranking stay readable.

If the scoreboard appears stuck, an old server session may still be in `game_over`, or the auto-show setting may be enabled.

Fix:

1. Open `tools/websocket-test.html`.
2. Click `Apply Safe Visual Demo`.
3. Click `Send Debug Room`.
4. Confirm `match.state=in_progress` and `game_over=false`.

For Unreal-only recording, enable `bApplyDemoPresetOnJoin` in the active PlayerController Blueprint and set `DemoPresetOnJoin=safe_visual` or `combat_demo` so the server is reset after `join_ok`.

## How To Use Demo/Debug Controls

Open `tools/websocket-test.html`, connect to the same server Unreal uses, then send debug commands before starting the Unreal demo or immediately after joining:

- `Apply Safe Visual Demo`: one-click visual preset with easy bots, bot attacks off, timer off, full reset.
- `Apply Combat Demo`: one-click combat preset with easy bots, bot attacks on, timer off, full reset.
- `Apply Match Demo`: one-click match-flow preset with normal bots, bot attacks on, timer on, full reset.
- `Apply Debug Visual Demo`: one-click bounds/ghost inspection preset with easy bots, bot attacks off, timer off.
- `Apply Safe Demo Mode`: legacy convenience reset. Prefer the named preset buttons for recording.
- `Bot Difficulty Easy`: lowers bot shot cadence, range, speed, and accuracy.
- `Disable Bot Attacks`: keeps bot movement/ghosts active but prevents bot shooting and player damage.
- `Disable Match Timer`: prevents timer-based `game_over` during long recordings.
- `Send Debug Restart Match`: clears game-over state while preserving the current demo/debug settings.

In Unreal, enable `bApplyDemoPresetOnJoin` on the active PlayerController Blueprint and set `DemoPresetOnJoin` to `safe_visual`, `combat_demo`, or `match_demo`. Use `bApplySafeDemoModeOnJoin` or `bApplyDemoServerSettingsOnJoin` only when testing the older compatibility paths.

## Server Bot HP Does Not Decrease

Server bot damage comes from server hitscan, not from local Unreal projectile overlap logs. Local logs that mention `StaticMeshActor` or local targets only prove the offline projectile layer is working.

Use `tools/websocket-test.html`:

1. Connect and send `Join`.
2. Click `Apply Safe Visual Demo`.
3. Click `Send Debug Room`.
4. Confirm the combat tuning panel shows body/head damage `20/40`, bot difficulty `easy`, bot attacks disabled, timer auto-end disabled, and kill score values `1/1`.
5. Wait for a snapshot with `bots=8/8`.
6. Click `Send Fire At Nearest Bot`.
7. For a faster kill test, click `Fire 5 Shots At Nearest Bot`.
8. For a headshot-specific test, click `Send Headshot At Nearest Bot`.

Expected server logs:

```text
[BattleGridServer] Hitscan fire shooter=1 seq=...
[BattleGridServer] Shot result shooter=1 result=hit target=bot damage=20 headshot=false
[BattleGridServer] Hitscan bot hit shooter=1 bot=... group=body damage=20 hp=80/100 dir=(...)
```

The browser page now also shows a `Shot Result` summary. A valid server hit should show `SERVER HIT BOT-* -20` or `SERVER HEADSHOT BOT-* -40`. If the match is `game_over`, click `Apply Combat Demo` or `Apply Safe Visual Demo` before testing shots; fire inputs can still be acknowledged, but server combat is disabled while the match is over.

For prototype aiming, bot hitscan uses 3D head/body spheres and a forgiving 2D body fallback. The fallback is only for bot hit testing, and it should not override a valid headshot. Disabling bot attacks does not make bots invulnerable.

`Fire 5 Shots At Nearest Bot` sends five spread-free server fire inputs toward the nearest alive bot using the latest snapshot position. It does not rely on local projectile collision, and it keeps small delays between inputs so the browser stays responsive.

`Send Fire At Nearest Bot` intentionally sends `shot_dir_z=0`, so it is a body-shot test. `Send Headshot At Nearest Bot` computes a 3D direction from the joined player's server fire origin to the nearest bot head volume and should produce `SERVER HEADSHOT BOT-* -40` when the ray is unobstructed and the bot is alive.

If Unreal shots against the visible bot head still deal only 20:

- Check the Output Log for `Fire server shot_dir=(X,Y,Z)`. The Z value should be nonzero when aiming above the body.
- Enable `BP_BattleGridServerBotGhostActor > BattleGrid|Debug > bShowServerHitVolumes`.
- Confirm the magenta head sphere is where you are aiming and the cyan body sphere is below it.
- Confirm the server log says `group=head damage=40` for headshots.

## Bot Shots Do Not Appear

Safe visual/demo mode intentionally sets `bot_attacks_enabled=false`. Bots still move, but they will not fire. To test Bot Shooter AI v2:

1. Click `Apply Combat Demo`.
2. Click `Send Debug Room`.
3. Confirm `current_demo_preset=combat_demo`, `bot_attacks_enabled=true`, and `bot_difficulty=easy`.
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

## A Different Bot Loses HP Than The One Aimed At

Server hitscan selects the closest valid hit along the shot ray, not the first bot found in the server container. If the wrong bot still appears to take damage:

- Click `Send Debug Room` in `tools/websocket-test.html` and check `enable_bot_2d_fallback_hit` and `bot_2d_fallback_radius_scale`.
- Check `use_client_fire_origin_for_hitscan=true`. This lets the server use the client-provided muzzle/crosshair fire origin when it is close enough to the authoritative player position.
- Check `use_client_position_for_player_movement=true` and, from Unreal, `latest_input.has_client_position=true`; this keeps the server origin close to the local pawn during movement.
- Temporarily enable `BattleGrid|Debug > bShowServerHitVolumes` on `BP_BattleGridServerBotGhostActor` to see the server body/head spheres.
- Temporarily enable `BattleGrid|Debug > bShowServerShotDebug` on the PlayerController to draw the world-space shot ray sent to the server.
- If the local pawn and `SERVER ECHO` are separated, check the server log for `Client fire origin far from server position` or `Rejected client fire origin`. Rejected origins fall back to the server player position and can still feel offset.
- If nearby bots still steal body hits, reduce `Bot2DFallbackRadiusScale` in `GameRoom` or disable `bEnableBot2DFallbackHit` for stricter testing.
- Remember that fallback hits are forgiving 2D body checks; true head/body sphere hits always have priority for the same bot, and the closest selected candidate wins globally.

## Local Pawn And SERVER ECHO Are Separated While Shooting

The server is still authoritative for damage, but in prototype/demo mode the client sends an optional `fire_origin_x/y/z` with each fire input. Unreal builds that origin from the approximate player muzzle in world space, converts it back into server arena coordinates, and sends it with the camera/crosshair shot direction.

The server accepts the client fire origin only when it is within `max_accepted_client_fire_origin_distance` of the authoritative player position. If it is farther away, the server falls back to `PlayerState.x/y/z + fire height`.

Debug steps:

- Keep `BattleGrid|Server Movement > bSendClientPositionToServer=true` for normal demo testing so the accepted server `PlayerState` stays close to the local pawn before shots are fired.
- In Unreal, enable `BattleGrid|Debug > bShowServerShotDebug` to draw the local fire ray.
- In the browser, click `Send Debug Room` and verify `use_client_fire_origin_for_hitscan=true` and `use_client_position_for_player_movement=true`.
- Watch server logs for `Fire origin player=... source=client` on fire inputs.
- If logs show `source=server`, confirm Unreal is connected/joined and the active PlayerController has the latest C++ build.
- If logs show `Rejected client fire origin`, the local pawn and server ghost are too far apart for the configured sanity limit; use Safe Demo Mode, origin calibration, or temporarily increase `MaxAcceptedClientFireOriginDistance` for local testing.

## Server HP Differs From Local HP

This is expected during the transition from local prototype gameplay to server-authoritative PvPvE gameplay.

- `SERVER HP` comes from the own player snapshot in the C++ server.
- `Local HP` comes from Unreal's offline hazard/local damage test layer.
- By default, the HUD prioritizes `SERVER HP` and `SERVER Kills`.
- Enable `bShowLocalDebugHud` in the active PlayerController Blueprint only when you need to compare local/offline state with server state.

If the server says the player is dead, local movement/fire can be locked even if the local pawn still appears alive. Use `Apply Safe Visual Demo` or `Apply Combat Demo` if the server state needs to be reset for testing.

## I Die But No Respawn Message Appears

The center death/respawn message uses the existing `CombatMessageText` binding.

Check:

- The active combat HUD widget has a TextBlock named `CombatMessageText`.
- `CombatMessageText` has `Is Variable` enabled.
- The active PlayerController has `BattleGrid|Server State > bShowServerDeathStatus=true`.
- The own player snapshot includes `alive=false`, `respawn_timer`, `invincible`, and `invincible_timer`.
- `BattleGrid|HUD > bUseGameplayHudLayout=true` is okay; C++ makes `CombatMessageText` visible whenever it has death, respawn, invincibility, or shot-result text.

Expected center messages:

- `KILLED BY BOT-*`
- `Respawn in Ns`
- `INVINCIBLE Ns`

## I Can Move While Dead

Server death input lock is controlled by the active PlayerController.

Check:

- `BattleGrid|Server State > bRespectServerDeathState=true`.
- `BattleGrid|Server State > bServerDeathLocksInput=true`.
- `debug_room` or the browser snapshot shows the own player as `alive=false`.
- The active Blueprint is using the rebuilt C++ PlayerController class.

When the server marks the player dead, movement, fire, ADS, sprint, jump, and reload input should be cleared or ignored until the server snapshot returns to `alive=true`.

## Why SERVER HUD Is Primary

The PvPvE demo is meant to show server-authoritative match state. The server owns player HP, deaths, respawn timers, kills, bot/health-pack state, match timer, scoreboard, and combat events.

The gameplay HUD therefore shows server state first:

- HP and ammo in the bottom HUD area.
- Top 5 ranking in the top-right HUD area.
- Kill feed on the left side.
- Match/server status in small status text.
- Temporary game-over result, death/respawn, invincibility, health pickup, reload/empty, and server shot results such as `SERVER HIT BOT-3 -20` in the message area.

Local HP/score remains available for offline debugging, but it should not be used to explain server bot damage, match scoring, or winner state.

## HUD Still Shows Large Debug Text

The final demo layout is controlled from the active PlayerController Blueprint under `BattleGrid|HUD`.

For a clean recording, set:

- `bUseGameplayHudLayout=true`
- `bShowDebugHud=false`
- `bShowControlsHelp=false`
- `bShowCombatEventFeed=false`
- `bShowLocalDebugHud=false`
- `bShowServerTargetGhosts=false`
- `bDrawServerArenaBounds=false`
- `bDrawServerBotAreaBounds=false`
- `bShowServerShotDebug=false`
- `bShowAimDebug=false`

If a large block still appears, check whether the active widget is missing optional gameplay bindings. Without `RankingText`, `KillFeedLine1-5`, `AmmoText`, `MatchText`, or `SmallServerStatusText`, the C++ fallback may place compact fallback text into `ControlsText`.

## Ammo Text Missing

`AmmoText` is optional. If it is missing, the HUD falls back to `ScoreText` and shows ammo beside K/D or match text.

For the cleaner layout:

- Add a TextBlock named `AmmoText` to the active combat HUD widget.
- Enable `Is Variable`.
- Place it near the bottom-center HP area.
- Recompile the widget Blueprint.

## Gun Does Not Fire

Connected PvPvE mode blocks client fire before sending `fire=true` to the server when the local weapon cannot shoot.

Check:

- Ammo is above 0. Empty magazines show `EMPTY - PRESS R` unless auto-reload immediately starts.
- The weapon is not reloading. While reloading, the center message can show `RELOADING...` and `AmmoText` should show `Reloading Ns`.
- The server snapshot does not mark the player dead. Dead players cannot fire, reload, sprint, ADS, jump, or move until respawn.
- Fire cooldown has elapsed. The default fire rate is 8 shots per second.
- The active PlayerController has `BattleGrid|Weapon > bAutoReloadOnEmpty=true` if you expect empty magazines to reload automatically.

Expected defaults:

- `MagazineSize=30`
- `CurrentAmmo=30`
- `ReloadTimeSeconds=2.0`
- `FireRatePerSecond=8.0`
- `bResetAmmoOnServerRespawn=true`

## Ranking Not Visible

The Top 5 ranking uses the optional `RankingText` TextBlock.

Check:

- Add a TextBlock named `RankingText`.
- Enable `Is Variable`.
- Place it at the top right.
- In the active PlayerController Blueprint, keep `BattleGrid|HUD > bShowTopFiveRanking=true`.
- Confirm the server has joined and scoreboard data exists.

If `RankingText` is missing, the C++ fallback can append ranking text to `ControlsText`, but the result looks more like a debug overlay.

## Kill Feed Not Visible In Gameplay Layout

The left-side kill feed uses optional TextBlocks named `KillFeedLine1` through `KillFeedLine5`.

Check:

- Add all five TextBlocks.
- Enable `Is Variable` on each.
- Place them vertically on the left side.
- In the active PlayerController Blueprint, keep `BattleGrid|HUD > bShowKillFeed=true`.
- Confirm the server sends kill events such as `bot_killed`, `player_killed`, or `bot_killed_player`.

Shot events such as `shot_hit_bot` and `shot_miss` are temporary hit marker text, not persistent kill feed lines.

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

Expected normal snapshot field:

```json
"targets": []
```

Use `debug_room.targets_debug` when intentionally inspecting the legacy target/core system.

## `server/build` Should Not Be Committed

`server/build` is generated output. Do not commit it. Keep generated build directories, Unreal `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` out of source control.
