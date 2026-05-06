# BattleGrid Arena Layout

This document defines the current server-side arena layout for the PvPvE kill race prototype. The Unreal map is still aligned manually by the human developer. Codex does not edit `.umap` or `.uasset` files.

## Arena Concept

The server owns a compact rectangular arena with mirrored player spawns, central core targets, a bot pressure ring, and health packs near the outer lanes.

The layout is intentionally simple:

- Players spawn on the four cardinal sides.
- Core targets sit in the center and diagonal mid-lanes.
- Bots start around the center and side lanes to create immediate PvE pressure.
- Health packs spawn near the perimeter to encourage movement instead of camping.

## Server Coordinate System

Server coordinates are logical 2D arena coordinates:

- `x`: horizontal arena axis.
- `y`: vertical arena axis.
- `z`: currently used only for snapshot metadata and hit volumes.

The Unreal client converts server coordinates into world coordinates for ghost visualization. Current ghost conversion swaps server X/Y for the top-down/third-person template orientation, so the Unreal map should be validated visually against server ghost actors.

## Arena Bounds

| Field | Value |
| --- | ---: |
| `min_x` | `-1800` |
| `max_x` | `1800` |
| `min_y` | `-1200` |
| `max_y` | `1200` |

The server uses these bounds for:

- player movement clamp
- bot movement clamp
- bot wander target selection
- projectile out-of-bounds expiry
- health pack spawn validation through fixed spawn candidates

## Player Spawn Points

| Spawn | X | Y |
| --- | ---: | ---: |
| P1 | `-1200` | `0` |
| P2 | `1200` | `0` |
| P3 | `0` | `900` |
| P4 | `0` | `-900` |

Player spawn is selected by `playerId` modulo the four spawn points. Join, debug match restart, and server respawn all reuse the same spawn assignment.

## Core Target Positions

The C++ class remains `TargetState`, but the gameplay label is `CORE`.

| Core | X | Y | HP | Radius |
| --- | ---: | ---: | ---: | ---: |
| CORE-1 | `0` | `0` | `100` | `80` |
| CORE-2 | `700` | `500` | `100` | `80` |
| CORE-3 | `700` | `-500` | `100` | `80` |
| CORE-4 | `-700` | `500` | `100` | `80` |
| CORE-5 | `-700` | `-500` | `100` | `80` |

## Bot Spawn Positions

| Bot | X | Y |
| --- | ---: | ---: |
| BOT-1 | `-300` | `300` |
| BOT-2 | `-300` | `-300` |
| BOT-3 | `300` | `300` |
| BOT-4 | `300` | `-300` |
| BOT-5 | `1000` | `0` |
| BOT-6 | `-1000` | `0` |
| BOT-7 | `0` | `700` |
| BOT-8 | `0` | `-700` |

Bots respawn at their fixed spawn point after the server respawn delay. Wander targets are generated inside the arena bounds.

## Health Pack Spawn Points

| Spawn | X | Y |
| --- | ---: | ---: |
| HPACK-SPAWN-1 | `-1300` | `700` |
| HPACK-SPAWN-2 | `1300` | `700` |
| HPACK-SPAWN-3 | `0` | `-1100` |
| HPACK-SPAWN-4 | `-1300` | `-700` |
| HPACK-SPAWN-5 | `1300` | `-700` |
| HPACK-SPAWN-6 | `0` | `1100` |

The room starts with three active health packs:

- HealthPack 1 at spawn 1.
- HealthPack 2 at spawn 3.
- HealthPack 3 at spawn 5.

Each health pack heals `+35`, uses pickup radius `90`, and respawns after `15` seconds.

## Recommended Visual Layout

Use a simple rectangular arena floor first:

- Server X span: `3600` units.
- Server Y span: `2400` units.
- Keep the playable area visually bounded with walls or low barriers.
- Put a visible central landmark at CORE-1.
- Put diagonal core markers at the four mid-lane core positions.
- Put bot placeholders around the central area and side lanes.
- Put health pack placeholders near the outer lanes.

Because the current Unreal client uses ghost actors for server entities, the fastest validation flow is to run the server, press Play in Unreal, and align the map by watching server player, bot, core, projectile, and health pack ghosts.

## Manual Unreal Editor Layout Guide

1. Do not edit server coordinates in Unreal assets.
2. In Unreal Editor, build or adjust the arena floor so the ghost actors fit inside the visible play space.
3. Place visual markers or placeholder meshes for the five CORE positions.
4. Place optional bot spawn markers matching the eight BOT positions.
5. Place optional health pack markers matching the six HPACK spawn candidates.
6. Start the C++ server and Play in Editor.
7. Confirm ghost actors appear in the expected relative layout:
   - P1/P2 on opposite horizontal sides.
   - P3/P4 on opposite vertical sides.
   - CORE-1 in the center.
   - Bots near center and side lanes.
   - Health packs near the outer lanes.
8. Adjust only Unreal map visuals and ghost scale/height as needed.

## Current Limitations

- Server layout and Unreal map visuals are manually aligned.
- There is no server obstacle map or navmesh.
- Bots do not avoid walls or arena props.
- Local Unreal targets and server CORE targets are still separate layers.
- Snapshot ghosts are the source of truth for verifying server layout in Unreal.
