# Unreal Client

## Role

The Unreal client is the playable front end for BattleGrid. It provides third-person controls, local feedback, HUD, and visualization of authoritative server snapshots. The current demo is server-centric by default, but local offline gameplay remains available for development and fallback.

## Gameplay Framework Classes

### `ABattleGridClientPlayerController`

The player controller is the main client-side gameplay coordinator.

Responsibilities:

- Adds the Enhanced Input mapping context.
- Handles camera-relative WASD movement.
- Handles mouse look.
- Handles ADS, sprint, jump, fire, reload, restart, and scoreboard input.
- Sends extended input messages to the server.
- Tracks local HP, local score, target score, victory, and restart for offline/debug mode.
- Stores local/remote server profile settings.
- Applies optional safe demo mode on join.
- Converts server 2D coordinates to Unreal world coordinates.
- Auto-calibrates server snapshot origin so SERVER ECHO aligns with the local pawn.
- Spawns and updates server player, projectile, core, bot, and health pack ghost actors.
- Calculates local-to-server position error.
- Optionally corrects the local pawn toward the server snapshot position.
- Reads own server life state to block movement/fire while the server says the player is dead.
- Provides server-centric HUD, event feed, shot result, and scoreboard text.

### `ABattleGridClientCharacter`

The character represents the local player pawn.

Responsibilities:

- Provides third-person movement.
- Owns spring arm and follow camera.
- Supports over-the-shoulder ADS camera interpolation.
- Owns the weapon component.
- Owns a health component for local/offline damage.
- Takes local hazard damage.
- Dies and respawns locally for offline testing.
- Syncs local HP/death state back to the player controller for optional debug HUD.

### `UBattleGridWeaponComponent`

Local weapon state component.

Responsibilities:

- Stores magazine size and current ammo.
- Supports unlimited reserve ammo.
- Handles reload timing.
- Enforces local fire rate.
- Stores body/headshot damage defaults for client display/future validation.
- Calculates spread based on ADS, sprint, and jump/falling state.

Current defaults:

- Magazine: 30.
- Reload: 2 seconds.
- Fire rate: 8 shots per second.
- Body damage: 20.
- Headshot damage: 40.
- Hip spread: 3.5 degrees.
- ADS spread: 0.8 degrees.
- Sprint spread: 6.0 degrees.
- Jump spread: 9.0 degrees.

### `UBattleGridHealthComponent`

Reusable local health component for prototype damageable actors.

Responsibilities:

- Stores max/current HP.
- Resets health.
- Applies clamped damage.
- Reports death state.
- Logs HP before and after damage for debugging.

### `ABattleGridProjectile`

Local offline projectile actor.

Responsibilities:

- Uses collision, mesh, and projectile movement components.
- Spawns from the local player controller after weapon fire is allowed.
- Moves forward with no gravity.
- Applies Unreal damage to hit actors.
- Destroys itself on hit or after lifespan.

Local projectile collisions are not authoritative server combat. Server combat is confirmed through server shot result events, server HP changes, ghost labels, and scoreboard changes.

### `ABattleGridDamageableTarget`

Local offline target actor.

Responsibilities:

- Owns visible mesh and health component.
- Receives projectile damage through Unreal `TakeDamage`.
- Destroys itself at 0 HP.
- Awards local score to the attacking player controller.

Local targets are separate from server cores in the current prototype.

### `ABattleGridHazardActor`

Local hazard actor for testing player HP, death, and respawn.

Responsibilities:

- Detects actor-level overlap.
- Tracks overlapping player actors.
- Applies damage on a cooldown.
- Uses Unreal damage flow so the character's `TakeDamage` path is tested.

## HUD And UMG

`ABattleGridHUD` creates a `UBattleGridCombatWidget` when `CombatWidgetClass` is assigned.

The widget currently shows:

- Server HP as the primary health readout while connected/joined.
- Server score, goal score, K/D, and ammo.
- Profile, server connection, match time, world counts, and controls.
- Recent server combat event feed.
- Temporary server shot result text such as `SERVER HIT BOT-3 -20` or `SERVER MISS`.
- Server death/respawn state when the server says the player is dead.
- Tab scoreboard text from server snapshot data.
- Optional local debug text when `bShowLocalDebugHud` is enabled.

Display priority:

1. Scoreboard overlay.
2. Server dead/respawn message.
3. Recent server shot result.
4. Server combat event feed.
5. Local combat/victory message.

## Network Subsystem

`UBattleGridNetworkSubsystem` is a `UGameInstanceSubsystem`.

Responsibilities:

- Connects and disconnects a WebSocket.
- Sends `ping`, `join`, `input`, and demo/debug commands.
- Parses `pong`, `join_ok`, `input_ack`, `error`, `debug_ok`, `match_restarted`, and `snapshot`.
- Stores connection state, player ID, room ID, nickname, last error, and last debug message.
- Stores latest snapshots for players, projectiles, cores, bots, health packs, match, scoreboard, and combat events.
- Deduplicates combat events by `event_id`.
- Stores recent server shot result text for timed HUD display.
- Provides server summary, scoreboard, event feed, and world count helpers for the HUD.

The game remains playable if the server is not running.

## Server Ghost Actors

### `ABattleGridServerGhostActor`

Visualizes server player positions as `SERVER ECHO`.

- Uses a mesh and label.
- Interpolates toward latest server snapshot position.
- Shows invincibility in the label when applicable.

### `ABattleGridServerProjectileGhostActor`

Visualizes server projectiles/tracers.

- Uses a small mesh.
- Interpolates toward latest server projectile position.
- Rotates using converted server direction.

### `ABattleGridServerTargetGhostActor`

Visualizes server cores.

- Uses a mesh and label.
- Labels targets as `CORE-*`.
- Shows HP or destroyed state.

### `ABattleGridServerBotGhostActor`

Visualizes server bots.

- Uses a mesh and label.
- Shows HP, down state, and invincibility state.

### `ABattleGridServerHealthPackGhostActor`

Visualizes server health packs.

- Uses a mesh and label.
- Shows active heal amount or inactive respawn timer.

## Coordinate Conversion

The server uses logical 2D arena coordinates. The current Unreal map alignment uses an axis swap for server ghost visualization:

```text
WorldX = ServerY * ServerToUnrealScale
WorldY = ServerX * ServerToUnrealScale
```

Aim and shot directions use the same mapping before Unreal sends input to the server. `ServerAimSignX` and `ServerAimSignY` remain available for quick sign correction if a template axis is mirrored.

`bAutoCalibrateServerSnapshotOrigin` aligns the first own-player server snapshot with the current local pawn XY position. Other server ghosts then remain spatially consistent relative to that calibrated origin.

## Server Position Error And Correction

The controller compares the local pawn location against the latest own server player snapshot.

The HUD can display the 2D error distance. Optional correction can be enabled in Blueprint defaults:

- Small error: smoothly interpolate toward server position.
- Large error: snap to server position.

Correction is disabled by default for clean demo control. Respawn snapping is separate and can move the local pawn to the server respawn point when the server says the player respawned.

## Current Limitations

- Server ghosts are prototype visualization, not final production player/bot/core art.
- Local projectile visuals and server hitscan damage are separate layers.
- Local targets and server cores are separate layers.
- No production remote player mesh or animation state.
- No full prediction history or input replay.
- Text HUD/scoreboard/event feed should be replaced with dedicated UMG widgets later.
