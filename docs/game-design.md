# BattleGrid Game Design

This document defines the planned next BattleGrid upgrade. It rebrands the project from a debug-style server visualization arena into a third-person PvPvE kill race arena shooter.

This upgrade is not fully implemented yet. The current project still contains local Unreal gameplay, server snapshots, and ghost visualization layers used for networking bring-up.

## High Concept

BattleGrid is a third-person PvPvE arena shooter where up to four players fight each other and server-controlled bots inside a limited arena. The winner is the player with the highest score when the match ends or the first player to reach the target score.

Players can score by farming bots, but fighting other players matters because it denies bot kills, creates pressure, and awards higher score for player kills.

## Core Loop

1. Spawn into the arena with full HP and a loaded weapon.
2. Move, sprint, aim, and fight bots or other players.
3. Secure kills to increase score.
4. Use health packs to survive extended fights.
5. Reload after each 30-shot magazine.
6. Respawn after death and re-enter the fight.
7. Win by reaching the target score or leading when the timer ends.

## Controls

- WASD: move.
- Mouse: aim camera and weapon.
- Left click: fire.
- Right click hold: ADS / shoulder aim.
- Shift: sprint.
- Space: jump.
- R: reload.

ADS moves the camera slightly to the right, reduces spread, and lowers movement speed. Jumping cancels ADS and greatly increases spread.

## Combat Rules

- Player HP: 100.
- Body damage: 20.
- Headshot damage: 40.
- Player death triggers an 8 second respawn timer.
- A respawned player has 1.5 seconds of invincibility.
- Player kill score: +2.
- Bot kill score: +1.
- Highest score wins.

The long-term target is server-authoritative combat. The server should validate hits, damage, deaths, respawns, score changes, and match results.

## Weapon Rules

Recommended defaults:

- Unlimited reserve ammo.
- Magazine size: 30.
- Reload required every 30 shots.
- Reload time: 2 seconds.
- Fire rate: 8 shots per second.
- ADS reduces weapon spread.
- Jumping greatly increases weapon spread.

The first networked implementation should use server-side hitscan. Projectile visuals can still be rendered on clients as cosmetic tracers or impact effects.

## Movement Rules

Recommended defaults:

- Normal speed: 600.
- Sprint speed: 850.
- ADS speed: 400.
- Sprint cancels ADS if needed for readability.
- Jump cancels ADS.
- Jumping temporarily applies high spread.

Movement should remain responsive locally, but the authoritative server should own final player positions for PvP fairness.

## Bot Rules

Recommended defaults:

- Bots per match: 8.
- Bots are server-controlled.
- Bots count as PvE targets worth +1 score.
- Bots should be aggressive enough to create arena pressure but simple enough for a portfolio demo.

Initial bot behavior can be simplified:

- Spawn at fixed or random bot spawn points.
- Move toward the nearest visible player.
- Fire at a limited rate.
- Respawn after a delay, or spawn in waves if that is simpler.

Implemented v1 server bot behavior:

- Room 1 initializes eight fixed server bots.
- Bots move toward the nearest alive non-invincible player within detect range.
- Bots wander deterministically when no player is nearby.
- Bots apply simplified direct body damage when in attack range.
- Bots can be damaged by server hitscan head/body volumes.
- Bot kills award +1 server score and increment server `bot_kills`.
- Dead bots respawn after 8 seconds with 1.5 seconds of invincibility.

This v1 does not use navmesh pathfinding, bot projectile visuals, animations, or tactical decision making.

## Health Pack Rules

Recommended defaults:

- Random health packs spawn in the map.
- Health pack heal amount: +35.
- Health pack respawn time: 15 seconds.
- Health packs cannot raise HP above 100.
- Health pack pickup should be server-authoritative.

Health packs create rotation decisions and reduce the chance that the best strategy is camping bot spawn points.

## Scoring

Recommended defaults:

- Bot kill: +1.
- Player kill: +2.
- Target score: 20.
- Max players: 4.
- Match duration: 5 minutes.

Player kill value is intentionally higher than bot kill value so PvP pressure matters. Bot farming remains useful, but ignoring other players should be risky.

## Respawn

- Death starts an 8 second respawn timer.
- The dead player cannot move, fire, reload, pick up health packs, or score.
- Respawn returns the player to a valid spawn point with 100 HP.
- The player is invincible for 1.5 seconds after respawn.
- Invincibility should be visibly communicated in HUD and/or material effects.

Spawn selection should avoid placing the player directly inside immediate combat when possible.

## Match End

A match can end in either condition:

- A player reaches the target score of 20.
- The 5 minute match timer expires.

Winner selection:

- Highest score wins.
- If scores are tied, use player kills as the first tiebreaker if tracked.
- If still tied, allow a draw for the first implementation.

At match end, movement and combat can be disabled while the scoreboard is shown.

## HUD Requirements

The third-person PvPvE HUD should show:

- Current HP.
- Ammo in magazine and reload state.
- Current score.
- Target score.
- Match timer.
- Respawn countdown when dead.
- Invincibility countdown or indicator after respawn.
- Kill feed or recent combat messages.
- Health pack pickup message.
- Crosshair with spread feedback.
- ADS state through camera/crosshair changes.
- Scoreboard showing player names, score, kills, deaths, and ping if available.

The current debug-style server snapshot summary should move behind a developer/demo toggle once the PvPvE HUD becomes the primary presentation.

## Server-Authoritative Rules

The server should be authoritative for:

- Player join/session state.
- Player movement validation.
- Player HP.
- Weapon fire rate.
- Magazine ammo and reload timing.
- Hitscan hit validation.
- Headshot/body hit classification.
- Damage application.
- Death and respawn timing.
- Invincibility windows.
- Bot AI state.
- Health pack spawn, pickup, and respawn.
- Score changes.
- Match timer and match end.
- Snapshot broadcast.

Clients should handle:

- Input collection.
- Camera and local presentation.
- Prediction/interpolation where needed.
- HUD rendering.
- Cosmetic weapon/projectile effects.
- Audio and visual feedback.

## Recommended Defaults

| Setting | Value |
| --- | --- |
| Max players | 4 |
| Bots | 8 |
| Match duration | 5 minutes |
| Target score | 20 |
| Player HP | 100 |
| Body damage | 20 |
| Headshot damage | 40 |
| Death respawn | 8 seconds |
| Respawn invincibility | 1.5 seconds |
| Bot kill score | +1 |
| Player kill score | +2 |
| Health pack heal | +35 |
| Health pack respawn | 15 seconds |
| Magazine size | 30 |
| Reload time | 2 seconds |
| Fire rate | 8 shots per second |
| Normal speed | 600 |
| Sprint speed | 850 |
| ADS speed | 400 |

## Current Limitations

- The current Unreal client is still based on the earlier arena prototype and server ghost visualization.
- The current server has player, projectile, target, bot, hitscan, damage, respawn, and score state, but does not yet implement full match flow.
- Player-vs-player hit validation is implemented as simple server hitscan spheres, not lag-compensated production combat.
- Bots are implemented as simple server-side direct-damage AI, not full shooter AI.
- Health packs are not implemented.
- Magazine ammo, reload, ADS spread, jump spread, and shot direction are implemented locally and sent to the server; server ammo validation is not authoritative yet.
- Full client prediction and reconciliation are not implemented.
- Existing local targets and server targets are still separate layers.
- The current HUD is still partly a debug/demo HUD for server snapshot visibility.
