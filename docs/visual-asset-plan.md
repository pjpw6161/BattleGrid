# BattleGrid Visual Asset Plan

BattleGrid is currently a gameplay and networking prototype with placeholder visuals. The next visual pass should improve readability without changing server-authoritative gameplay rules.

## Current Visual Layer

- Local player uses the existing Unreal Character setup.
- Server player state is shown through `SERVER ECHO` ghost actors.
- Server bots are shown through bot ghost actors.
- Health packs are shown through health pack ghost actors.
- Projectiles/tracers are shown through projectile ghost actors.
- The old core/target visuals are legacy/debug-only and are no longer the main objective.

## Near-Term Goals

1. Add one humanoid player Skeletal Mesh.
2. Add one humanoid bot Skeletal Mesh.
3. Add one simple rifle Static Mesh.
4. Attach the rifle to the right-hand socket on the player and bot ghosts.
5. Keep bot labels readable above humanoid meshes.
6. Keep placeholder materials available as fallback visuals.

## Suggested Asset Roles

| Role | First-pass asset | Notes |
| --- | --- | --- |
| Player | Fab/Epic humanoid character | Must align to the Character capsule. |
| Bot | Fab/Epic humanoid, robot, minion, or soldier | Visual-only ghost; gameplay remains server-side. |
| Rifle | Simple static mesh rifle | Attached to `hand_rSocket` or equivalent. |
| Animations | Existing compatible AnimBP or retargeted animation set | T-pose is acceptable only during setup, not demo recording. |
| Materials | Simple team/readability colors | Keep server ghosts easy to distinguish from local player. |

## Implementation Status

- Player C++ now exposes a `WeaponMeshComponent` and socket/offset settings.
- Player C++ now exposes humanoid mesh transform offsets and optional simple Idle/Run/Jump animation playback.
- Player C++ exposes camera collision probe sizing for quick third-person camera clipping adjustments.
- Bot ghost C++ now exposes optional Skeletal Mesh visualization and a weapon mesh slot.
- Bot ghost C++ now exposes optional simple Idle/Run/Death animation playback for humanoid ghosts.
- Bot ghost C++ exposes humanoid mesh transform offsets for Paragon orientation, floor height, and scale fixes.
- Player, bot ghost, and server player ghost visual classes now expose muzzle socket names or fallback offsets for muzzle/tracer alignment.
- PlayerController can spawn local visual projectiles from the local muzzle and can override visual-only server tracer starts with local player, bot ghost, or remote server player ghost muzzle approximations.
- Server projectile ghosts support visual-only tracer lines with player/bot material slots.
- External assets are not imported yet.
- No animation retargeting, aim offsets, firing montages, reload montages, or weapon sockets are authored by Codex.

## Manual Editor Work

The human developer should import and configure assets in Unreal Editor:

- Add third-party assets under `Content/BattleGrid/Art/...`.
- Assign player Skeletal Mesh and optional AnimBP in `BP_BattleGridCharacter`.
- Tune player mesh location, rotation, scale, and camera offsets so movement/ADS remains readable.
- Optionally enable simple player animation playback only while testing compatible Idle, Run, and Jump assets.
- Assign bot Skeletal Mesh and optional AnimBP in `BP_BattleGridServerBotGhostActor`.
- Optionally enable `bUseSimpleBotAnimationPlayback` and assign compatible Idle, Run, and Death animation assets.
- Tune `HumanoidMeshRelativeLocation`, `HumanoidMeshRelativeRotation`, and `HumanoidMeshRelativeScale3D` for the selected bot asset.
- Create or verify right-hand weapon sockets.
- Create or verify muzzle sockets when the weapon mesh is ready.
- Tune mesh scale, location, rotation, label height, and weapon offsets.
- Use `bShowMuzzleDebug` briefly to verify muzzle spheres/lines, then disable it before recording.
- Assign separate player and bot projectile materials to the server projectile ghost Blueprint if clearer tracer colors are needed.
- Record all third-party asset sources in `docs/asset-credits.md`.

## Future Visual Work

- Humanoid movement and aim animations.
- Player Animation Blueprint/retargeting pass.
- Firing and reload animation montages.
- Weapon socket alignment pass.
- Niagara muzzle flash, tracer, and impact effects.
- Hit reactions and death poses for bot ghosts.
- Large arena environment art and minimap support.
