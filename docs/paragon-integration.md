# Paragon/Fab Humanoid Integration

This document describes the planned manual asset setup for BattleGrid humanoid player and bot visuals.

BattleGrid should keep its own name and identity. Paragon/Fab character assets can be used as third-party art assets, but the Paragon trademark should not be used as the game title, branding, or advertising claim.

## Goal

- Use humanoid characters for the local player and server bot ghosts.
- Attach a simple rifle mesh to a hand socket.
- Keep server gameplay authoritative through `PlayerState` and `BotState`.
- Keep bot ghost meshes visual-only; they do not affect collision, damage, hit tests, AI, or scoring.

## Recommended First Assets

- One humanoid character for the player.
- One humanoid, minion, robot, or soldier-like character for bot ghosts.
- One simple rifle static mesh.
- A compatible Animation Blueprint if available.

## Recommended Import Folders

```text
Content/BattleGrid/Art/Characters
Content/BattleGrid/Art/Bots
Content/BattleGrid/Art/Weapons
Content/BattleGrid/Art/Animations
Content/BattleGrid/Art/ThirdParty/Paragon
```

These folders should be created manually in Unreal Editor when assets are imported.

## Player Setup

1. Add the selected Fab/Epic character asset to the project through Unreal Editor or Epic Launcher.
2. Open `BP_BattleGridCharacter`.
3. Select the inherited `Mesh` component.
4. Assign the humanoid Skeletal Mesh.
5. Assign an Animation Blueprint if one is compatible.
6. Adjust mesh relative location, rotation, and scale so it aligns with the Character capsule.
7. Open the character skeleton or mesh and verify a right-hand weapon socket exists.
8. Prefer `hand_rSocket` for the first pass, matching the C++ default `WeaponSocketName`.
9. Select `WeaponMeshComponent`.
10. Assign the rifle Static Mesh.
11. Adjust `WeaponRelativeLocation`, `WeaponRelativeRotation`, and `WeaponRelativeScale` under `BattleGrid|Weapon Visual`.

If the socket is missing, C++ attaches the weapon to the character mesh root and logs a one-time warning. This prevents crashes while the asset setup is still in progress.

## Player Character Setup v1

The local player character now exposes safe visual offsets for Paragon/Fab humanoid meshes without changing the Character capsule, movement, camera, or server networking.

Manual setup:

1. Open `BP_BattleGridCharacter`.
2. Select the inherited `Mesh` component.
3. Assign a Paragon/Fab humanoid Skeletal Mesh, such as Murdock or Twinblast.
4. In Class Defaults under `BattleGrid|Visual`, tune:
   - `PlayerMeshRelativeLocation`
   - `PlayerMeshRelativeRotation`
   - `PlayerMeshRelativeScale3D`
5. Recommended starting values:
   - `PlayerMeshRelativeLocation = (0, 0, -90)`
   - `PlayerMeshRelativeRotation = (0, -90, 0)`
   - `PlayerMeshRelativeScale3D = (1, 1, 1)`
6. If the character faces sideways, try yaw values `-90`, `90`, `180`, or `0`.
7. Optionally assign simple compatible animation assets:
   - `PlayerIdleAnimation`
   - `PlayerRunAnimation`
   - `PlayerJumpStartAnimation`
   - `PlayerJumpLoopAnimation`
   - `PlayerJumpLandAnimation`
8. Enable `bUseSimplePlayerAnimationPlayback` only for simple testing.
9. If you later use a Paragon Animation Blueprint, disable `bUseSimplePlayerAnimationPlayback` and assign the Anim Class on the Mesh component instead.

If the mesh floats or sinks, tune `PlayerMeshRelativeLocation.Z` first. If ADS is blocked by shoulder/body geometry, tune `AdsSocketOffset.Y`, `AdsArmLength`, mesh scale, or the camera boom socket offsets before changing movement code.

For simple jump playback:

- Assign `Jump_Start` to `PlayerJumpStartAnimation`.
- Assign `Jump_Apex` or another air pose to `PlayerJumpLoopAnimation`.
- Assign `Jump_Land` to `PlayerJumpLandAnimation`.
- Keep `bLoopPlayerJumpLoopAnimation=false` for short apex clips such as `Jump_Apex`.
- `PlayerJumpAnimation` remains as a backward-compatible fallback for the loop/apex slot.

Short apex clips should not be restarted every tick. If `Jump_Apex` looks bad as an air hold, leave `PlayerJumpLoopAnimation` empty until a better looping or falling animation is available.

## Muzzle And Socket Setup

For Murdock-style or Paragon/Fab assets, inspect sockets in the Skeleton editor before tuning offsets.

Recommended socket candidates:

- `Muzzle`
- `weapon_r`
- `hand_r`
- `hand_rSocket`

Current C++ defaults:

- player `WeaponSocketName`: `hand_rSocket`
- player `MuzzleSocketName`: `Muzzle`
- player `MuzzleFallbackOffset`: `(100, 25, 90)`
- bot `BotWeaponSocketName`: `hand_rSocket`
- bot `MuzzleSocketName`: `Muzzle`
- bot `MuzzleFallbackOffset`: `(100, 25, 100)`

If no muzzle socket exists yet, use the fallback offset first so the demo can proceed without editing assets. Later, add a real muzzle socket to the weapon or character skeleton and retune the relative weapon offsets. Murdock-style characters may already include a weapon as part of the Skeletal Mesh, so `WeaponMeshComponent` can remain empty while the muzzle location comes from the skeletal `Muzzle` socket or fallback offset.

For Step 61 muzzle/tracer polish:

- Enable `bShowMuzzleDebug` briefly on `BP_BattleGridCharacter`, `BP_BattleGridServerBotGhostActor`, or the server player ghost Blueprint to draw a small sphere and forward line at the approximate muzzle.
- For the local player, `bSpawnLocalProjectileFromMuzzle=true` spawns the local visual projectile at `ABattleGridClientCharacter::GetApproximateMuzzleWorldLocation()`.
- For server tracer ghosts, `bUseClientMuzzleForServerTracerStart=true` lets Unreal override visual tracer starts with the local player muzzle, matching bot ghost muzzle, or remote server player ghost fallback.
- These overrides are visual-only. Server hitscan damage still comes from the server snapshot and combat events.

For ADS aim/tracer alignment:

- The PlayerController uses the center of the viewport as the crosshair position.
- It deprojects that point and traces from the camera to an aim point.
- Local visual projectiles start from the approximate muzzle and rotate toward that aim point.
- The server shot direction remains crosshair-derived and still uses weapon spread.
- If the tracer travels down-left or away from the crosshair, enable PlayerController `bShowAimDebug` plus character `bShowMuzzleDebug` and verify the camera aim line and muzzle-to-aim line converge on the same target.

## Bot Ghost Setup

1. Open `BP_BattleGridServerBotGhostActor`.
2. In Class Defaults, enable `bUseSkeletalMeshVisual`.
3. Select `SkeletalMeshComponent`.
4. Assign the bot humanoid Skeletal Mesh.
5. Assign an Anim Class if a compatible one exists.
6. Select `WeaponMeshComponent`.
7. Assign the rifle Static Mesh.
8. Set `BotWeaponSocketName`, usually `hand_rSocket`.
9. Adjust `BotWeaponRelativeLocation`, `BotWeaponRelativeRotation`, and `BotWeaponRelativeScale`.
10. Tune `HumanoidAliveScale`, `HumanoidDeadScale`, and `HumanoidLabelHeight`.

When `bUseSkeletalMeshVisual` is enabled and a Skeletal Mesh is assigned, the bot ghost hides its static placeholder mesh. If no Skeletal Mesh is assigned, the existing placeholder mesh remains visible.

## Step 58: Simple Bot Animation Playback

Bot ghosts now support optional direct animation playback for a lightweight demo pose pass. This is not a full Animation Blueprint or combat montage system. It only chooses Idle, Run, or Dead based on the ghost's visual state.

1. Open `BP_BattleGridServerBotGhostActor`.
2. Enable `bUseSkeletalMeshVisual`.
3. Assign the Murdock or other humanoid Skeletal Mesh to `SkeletalMeshComponent`.
4. In `BattleGrid|Animation`, enable `bUseSimpleBotAnimationPlayback`.
5. Assign compatible assets:
   - `BotIdleAnimation`
   - `BotRunAnimation`
   - `BotDeathAnimation`
6. Start with `BotRunSpeedThreshold=20`.
7. In `BattleGrid|Visual`, tune:
   - `HumanoidMeshRelativeLocation`
   - `HumanoidMeshRelativeRotation`
   - `HumanoidMeshRelativeScale3D`
   - `HumanoidAliveScale`
   - `HumanoidDeadScale`
   - `HumanoidLabelHeight`

Suggested Content Browser searches for Murdock animation assets:

- `Idle`
- `Run`
- `Death`
- `Jog`
- `Walk`

The assigned animation assets must use the same skeleton as the active Murdock Skeletal Mesh. If the model T-poses, the likely causes are: no animation asset is assigned, the animation uses a different skeleton, or `bUseSimpleBotAnimationPlayback` is disabled. If the bot slides while moving, assign a run/jog animation or lower `BotRunSpeedThreshold`. If the model faces sideways, adjust `HumanoidMeshRelativeRotation`, usually yaw in 90 degree increments.

## Step 59: Bot Facing And Rotation

Bot ghosts now separate actor rotation from skeletal mesh forward correction.

- Actor rotation follows movement direction while the ghost is moving.
- When the ghost is nearly stationary, actor rotation can use converted server yaw.
- `MeshForwardYawOffset` only corrects the humanoid mesh's local forward axis.

Recommended defaults in `BP_BattleGridServerBotGhostActor`:

- `bFaceMovementDirection=true`
- `bUseServerYawWhenNotMoving=true`
- `RotationInterpSpeed=12`
- `MovementFacingThreshold=5`
- `MeshForwardYawOffset=0`

If Murdock moves sideways while the actor travels in the correct direction, keep actor movement-facing enabled and tune only the mesh offset. Try these values:

- `0`
- `90`
- `-90`
- `180`

Use `HumanoidMeshRelativeRotation` for broader asset orientation and `MeshForwardYawOffset` for the final local-forward correction. Actor rotation should continue to follow movement; the mesh offset should not be used as a replacement for actor yaw.

## Known Risks

- Paragon characters may use different skeletons.
- Animation retargeting may be required.
- Socket names can differ by skeleton.
- Weapon alignment usually needs manual offsets.
- Imported character scale may not match the current BattleGrid capsule or arena.
- A missing Animation Blueprint can leave a character in T-pose.
- Server bot ghosts are visual-only; server combat still uses simple `BotState` hit volumes.

## Asset Credits

Every imported third-party asset must be added to `docs/asset-credits.md` with:

- asset name
- creator/vendor
- source URL
- license or marketplace terms
- imported path
- usage
- redistribution notes

Do not claim original ownership of marketplace, Fab, Epic, Paragon, Mixamo, Freesound, or other third-party assets.
