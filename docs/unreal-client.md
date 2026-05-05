# Unreal Client

## Role

The Unreal client is the playable front end for BattleGrid. It handles local arena shooter gameplay while also connecting to the custom C++ server and visualizing authoritative server snapshots.

## Gameplay Framework Classes

### `ABattleGridClientPlayerController`

The player controller is the main client-side gameplay coordinator.

Responsibilities:

- Adds the Enhanced Input mapping context.
- Handles WASD movement.
- Handles mouse aim rotation.
- Handles local fire input.
- Sends input messages to the server.
- Tracks local HP, local score, target score, victory, and restart.
- Stores network display settings.
- Converts server 2D coordinates to Unreal world coordinates.
- Spawns and updates server player, projectile, and target ghost actors.
- Calculates local-to-server position error.
- Optionally corrects the local pawn toward the server snapshot position.

### `ABattleGridClientCharacter`

The character represents the local player pawn.

Responsibilities:

- Provides the visual/movable pawn.
- Owns a health component.
- Takes local hazard damage.
- Dies at 0 HP.
- Disables movement/collision while dead.
- Respawns at the initial spawn transform after a short delay.
- Syncs HP and death state back to the player controller for the HUD.

### `UBattleGridHealthComponent`

Reusable health component for local damageable actors.

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
- Spawns from the local player controller.
- Moves forward with no gravity.
- Applies Unreal damage to hit actors.
- Destroys itself on hit or after lifespan.

### `ABattleGridDamageableTarget`

Local offline target actor.

Responsibilities:

- Owns visible mesh and health component.
- Receives projectile damage through Unreal `TakeDamage`.
- Destroys itself at 0 HP.
- Awards local score to the attacking player controller.

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

- Local HP progress bar.
- Local HP text.
- Local score and target score.
- Server score.
- Combat or victory message.
- Controls.
- Detailed server summary.
- Optional crosshair text.

The HUD reads player state from `ABattleGridClientPlayerController`.

## Network Subsystem

`UBattleGridNetworkSubsystem` is a `UGameInstanceSubsystem`.

Responsibilities:

- Connects and disconnects a WebSocket.
- Sends `ping`, `join`, and `input`.
- Parses `pong`, `join_ok`, `input_ack`, `error`, and `snapshot`.
- Stores connection state, player ID, room ID, nickname, last error, and last message.
- Stores latest player, projectile, and target snapshots.
- Provides server summary helpers for the HUD.

The game remains playable if the server is not running.

## Server Ghost Actors

### `ABattleGridServerGhostActor`

Visualizes server player positions.

- Uses a mesh and label.
- Interpolates toward latest server snapshot position.
- Can show the local player's own server position or remote player positions.

### `ABattleGridServerProjectileGhostActor`

Visualizes server projectiles.

- Uses a small mesh.
- Interpolates toward latest server projectile position.
- Rotates using converted server direction.

### `ABattleGridServerTargetGhostActor`

Visualizes server targets.

- Uses a mesh and label.
- Shows target ID and HP.
- Shows dead state when server target HP reaches 0.

## Coordinate Conversion

The server uses logical 2D arena coordinates. The current Unreal top-down template/camera orientation needs the server X/Y axes swapped for ghost visualization:

```text
WorldX = ServerY * ServerToUnrealScale
WorldY = ServerX * ServerToUnrealScale
```

Aim direction is converted with the same mapping before Unreal sends input to the server.

## Server Position Error And Correction

The controller compares the local pawn location against the latest server snapshot for the same `player_id`.

The HUD displays the 2D error distance. Optional correction can be enabled in Blueprint defaults:

- Small error: smoothly interpolate toward server position.
- Large error: snap to server position.

Correction is disabled by default so local offline gameplay remains unchanged.

## Current Limitations

- Server ghosts do not replace local gameplay.
- Server target ghosts are not synchronized with Unreal-placed local targets.
- Local and server scores are displayed separately.
- No remote player mesh.
- No prediction history or input replay.
- No server-side player damage yet.
