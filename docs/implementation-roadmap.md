# BattleGrid Implementation Roadmap

BattleGrid has pivoted from its legacy target-objective prototype into a third-person PvPvE Kill Race Shooter.

The current codebase already demonstrates Unreal third-person controls, local weapon feedback, WebSocket JSON networking, a custom C++ server, server snapshots, bots, health packs, server hitscan shot result events, safe demo mode, match state, scoreboard, Docker deployment, and GCP deployment documentation.

## Completed Foundation

- Unreal third-person camera and camera-relative movement.
- ADS, sprint, jump, reload, ammo, automatic fire, and local spread values.
- Custom C++20 WebSocket JSON server.
- Join, input, input_ack, snapshots, debug room, safe demo mode.
- Server-side players, bots, health packs, projectiles/tracers, match state, scoreboard, and combat events.
- Server hitscan against bots and players.
- Server death, respawn, and invincibility synchronization to the local player.
- Text HUD summaries and Tab scoreboard overlay.
- Browser websocket test page with throttled snapshot summaries.

## Legacy Systems

- The old `TargetState` objective remains in code as legacy debug content.
- It is disabled by default for the main kill race path.
- It should not drive primary score, HUD counts, or demo flow.

## Step 51: Ranking HUD

Goal: make the top-5 connected-player ranking readable without relying on the Tab text overlay.

Work:

- Keep server scoreboard as the source of truth.
- Show rank, nickname, total kills, bot kills, player kills, and deaths.
- Keep the HUD server-centric.
- Avoid adding a complex UMG table until the layout is stable.

## Step 52: Kill Log

Goal: make server-authoritative kills clear and visually distinct.

Work:

- Use server `events` as the only authoritative kill feed source.
- Color my kills green.
- Color deaths against the local player red.
- Keep other events neutral.
- Keep shot result text separate from persistent kill log spam.

## Step 53: Crosshair

Goal: communicate weapon spread in the center of the screen.

Work:

- Add crosshair expansion based on hip/ADS/sprint/jump spread.
- Show recent server hit/miss feedback.
- Keep the implementation compatible with the existing weapon component.

## Step 54: Bot Shooter AI

Goal: replace simple direct bot damage with readable shooter behavior.

Work:

- Give bots low-accuracy ranged attacks.
- Add attack windup/cooldown state.
- Add simple line-of-sight or range checks.
- Keep deterministic debug behavior where possible.

## Step 55: Paragon Model Integration

Goal: replace placeholder ghost actors with readable humanoid characters.

Work:

- Import or reference approved Unreal/Fab assets manually in the Editor.
- Add player and bot skeletal mesh presentation.
- Keep source code independent of asset redistribution.
- Document asset credits and license constraints.

## Step 56: Tracer Visuals

Goal: make shooting readable without confusing local visuals and server authority.

Work:

- Add cosmetic muzzle flash/tracer/impact visuals.
- Make server shot result events remain the authority.
- Avoid claiming local projectile overlap equals server damage.

## Step 57: Large Map And Minimap

Goal: expand the current bounded arena into a larger kill-race map.

Work:

- Define larger server arena bounds.
- Add spawn zones and health pack zones.
- Add a minimap/radar concept.
- Keep server layout documented so the Unreal map can be manually aligned.

## Risks And Simplifications

- Full lag compensation is not implemented.
- Bots do not use navmesh/pathfinding yet.
- Server ammo/reload validation remains future work.
- Placeholder visuals are intentionally simple.
- Local offline gameplay remains separate from server-authoritative PvPvE state.
