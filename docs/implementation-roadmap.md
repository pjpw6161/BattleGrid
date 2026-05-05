# BattleGrid Implementation Roadmap

This roadmap describes the planned upgrade from the current BattleGrid prototype into a third-person PvPvE kill race arena shooter.

The upgrade is not complete yet. The current project already has useful networking foundations: WebSocket JSON, sessions, room state, player input, fixed-rate server tick, snapshots, server projectiles, server targets, server score, and Unreal ghost visualization. The next work should convert those foundations into a playable third-person PvPvE loop.

## Phase 1: Third-Person Camera And Controls

Goal: replace the current top-down arena shooter presentation with a third-person shoulder camera.

Work:

- Add a third-person spring arm and camera setup.
- Move from mouse world-point aim to camera-forward weapon aim.
- Add right-click hold ADS / shoulder aim.
- Shift sprint.
- Jump input.
- Make jumping cancel ADS.
- Add movement speed states:
  - normal speed: 600
  - sprint speed: 850
  - ADS speed: 400
- Keep the current local/remote server profile system.
- Keep server ghosts behind a demo/debug toggle during transition.

Validation:

- Player can move, sprint, jump, ADS, and aim in third person.
- ADS visibly shifts camera slightly to the right.
- Jump cancels ADS.
- Local offline movement still works when the server is unavailable.

## Phase 2: Weapon System

Goal: replace simple local projectile firing with a reusable weapon state model.

Work:

- Add weapon state to Unreal and server design:
  - magazine ammo
  - unlimited reserve ammo
  - reload state
  - fire cooldown
  - ADS state
  - spread state
- Implement magazine size 30.
- Implement reload time 2 seconds.
- Implement fire rate 8 shots per second.
- Add spread rules:
  - normal spread
  - reduced ADS spread
  - greatly increased jump spread
- Add HUD ammo and reload display.
- Keep local muzzle/tracer visuals cosmetic.

Validation:

- Player fires up to 30 shots.
- Firing is blocked during reload.
- Reload refills the magazine after 2 seconds.
- ADS reduces spread.
- Jumping increases spread and cancels ADS.

## Phase 3: Server Hitscan Combat

Goal: make combat damage and score server-authoritative.

Work:

- Extend input protocol with fire intent, aim origin, aim direction, ADS state, and jump/spread state.
- Server validates fire rate, ammo, reload, and player alive state.
- Server performs hitscan traces against players and bots.
- Add body and head hit regions or simplified hit zones.
- Apply damage:
  - body damage: 20
  - headshot damage: 40
- Add death, respawn timer, and invincibility state to server player state.
- Add server-side player score:
  - bot kill: +1
  - player kill: +2
- Broadcast combat events and updated snapshots.
- Keep client local effects responsive, but treat server result as authoritative.

Validation:

- Server rejects impossible fire rates.
- Server applies body and headshot damage.
- Player deaths and respawns are visible in snapshots.
- Score changes come from server state.

## Phase 4: Bots

Goal: add server-controlled PvE enemies that players can farm or contest.

Work:

- Add `BotState` or extend a generic combatant state model.
- Spawn 8 bots by default.
- Give bots HP, position, aim, fire cooldown, and respawn/wave behavior.
- Implement simple AI:
  - choose nearest or most visible player
  - move toward engagement range
  - fire at a controlled rate
- Include bots in snapshots.
- Add Unreal bot ghost or real bot actor visualization.
- Award +1 score to the player who kills a bot.

Validation:

- Bots spawn and move under server control.
- Bots can damage players if server player damage is enabled.
- Players can kill bots and gain score.
- Multiple players can contest the same bot kills.

## Phase 5: Health Packs

Goal: add map pickups that create rotation decisions and survival options.

Work:

- Add server-side health pack state:
  - id
  - position
  - active/inactive
  - respawn timer
- Spawn health packs randomly or from configured spawn points.
- Heal amount: +35.
- Respawn time: 15 seconds.
- Clamp player HP to 100.
- Include health packs in snapshots.
- Add Unreal health pack visuals and pickup feedback.

Validation:

- Health packs appear in the arena.
- Damaged players can pick them up.
- HP increases by 35 but never exceeds 100.
- Picked-up packs disappear and respawn after 15 seconds.

## Phase 6: Match And Scoreboard

Goal: turn the PvPvE loop into a complete match.

Work:

- Add match duration: 5 minutes.
- Add target score: 20.
- End match when a player reaches target score or timer expires.
- Add scoreboard fields:
  - player name
  - score
  - player kills
  - bot kills
  - deaths
  - ping if available
- Add match start, match active, match ended states.
- Disable combat after match end.
- Add restart/rematch flow for local demo.

Validation:

- Match timer counts down.
- Reaching target score ends the match.
- Highest score wins when time expires.
- Scoreboard clearly shows why a player won.

## Phase 7: Visual Polish

Goal: make the portfolio demo look like a game instead of a networking diagnostic.

Work:

- Replace debug ghost visuals with polished third-person player, bot, target, projectile/tracer, and pickup visuals.
- Add muzzle flash, impact effects, hit markers, damage numbers, and kill feed.
- Add respawn and invincibility visual feedback.
- Add ADS camera animation and crosshair spread animation.
- Add audio feedback for fire, hit, kill, reload, pickup, death, and victory.
- Keep a developer overlay toggle for server snapshot, position error, and correction diagnostics.

Validation:

- The main HUD communicates gameplay first.
- Debug networking information is available but not visually dominant.
- The demo recording can explain both gameplay and server authority.

## Risks And Simplifications

Risks:

- Full third-person prediction and reconciliation can become large quickly.
- Server-side hitscan needs a clean coordinate and hitbox model.
- Bots can become expensive if pathfinding or perception is overbuilt.
- PvP fairness depends on server authority, lag handling, and clear hit validation.
- Existing local gameplay and server visualization layers may diverge if not retired carefully.

Recommended simplifications:

- Start with one fixed arena and fixed spawn points.
- Use capsule/body and simple head hit zones before complex skeletal hitboxes.
- Use simple bot steering rather than full navigation if the arena supports it.
- Keep WebSocket JSON while iterating, then consider binary/UDP only after the loop is stable.
- Keep debug ghost/server overlays available for development but hide them for normal demo mode.
- Implement one weapon first.
- Implement health packs with fixed spawn points before random weighted spawns.
- Accept simple draw handling for first match-end implementation.

## Suggested Milestone Order

1. Third-person camera, movement, ADS, sprint, and jump.
2. Local weapon state with ammo, reload, fire rate, and spread.
3. Protocol extensions for weapon and aim input.
4. Server hitscan damage against simple target/player shapes.
5. Server death, respawn, invincibility, and score.
6. Server bots and bot kill scoring.
7. Health packs.
8. Match timer, target score, and scoreboard.
9. Visual/audio polish.
10. Prediction and reconciliation improvement pass.
