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
11. Adjust `WeaponRelativeLocation`, `WeaponRelativeRotation`, and `WeaponRelativeScale` under `BattleGrid|Visual`.

If the socket is missing, C++ attaches the weapon to the character mesh root and logs a one-time warning. This prevents crashes while the asset setup is still in progress.

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
- player `MuzzleFallbackOffset`: `(80, 20, 80)`
- bot `BotWeaponSocketName`: `hand_rSocket`
- bot `MuzzleSocketName`: `Muzzle`
- bot `MuzzleFallbackOffset`: `(80, 20, 100)`

If no muzzle socket exists yet, use the fallback offset first so the demo can proceed without editing assets. Later, add a real muzzle socket to the weapon or character skeleton and retune the relative weapon offsets.

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
