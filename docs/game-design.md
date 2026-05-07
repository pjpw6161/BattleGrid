# BattleGrid Game Design

BattleGrid is now a third-person PvPvE Kill Race Shooter prototype.

The previous target-objective prototype is legacy debug content. The main game objective is kill-based: connected players and humanoid shooter bots fight in the same arena, and the player with the most kills wins.

This is a portfolio prototype. The server-authoritative kill race loop is being built in stages, while some local Unreal offline systems and debug ghost visuals remain for comparison and testing.

## High Concept

- Third-person over-the-shoulder arena shooter.
- Players fight server-controlled humanoid bots and other players.
- Kills are the score.
- Ranking shows up to 5 connected players.
- Server-authoritative kills, deaths, respawns, health packs, match state, scoreboard, and kill log are central to the demo.

## Main Objective

- Bot kill: +1 kill.
- Player kill: +1 kill.
- Total score is `bot_kills + player_kills`.
- The player with the most kills wins.
- Legacy target/core kills are not part of the primary objective.

## Controls

- WASD: move.
- Mouse: look and aim.
- Left click: fire.
- Right mouse hold: ADS / shoulder aim.
- Shift: sprint.
- Space: jump.
- R: reload.
- Tab: scoreboard.

ADS moves the camera slightly to the right, reduces weapon spread, and lowers movement speed. Sprinting cancels ADS. Jumping cancels ADS and greatly increases spread.

## Combat Rules

- Player HP: 100.
- Player body damage: 20.
- Player headshot damage: 40.
- Bot damage is planned to be half player damage:
  - body: 10
  - headshot: 20
- Bot accuracy should be intentionally low for readable demos.
- Death starts an 8 second respawn timer.
- Respawn grants 1.5 seconds of invincibility.
- Health packs heal +35 and cannot raise HP above max.

The current server has hitscan shot result events:

- `SERVER HIT BOT-* -20`
- `SERVER HEADSHOT BOT-* -40`
- `SERVER HIT PLAYER -20`
- `SERVER MISS`

## Weapon Rules

- Unlimited reserve ammo.
- Magazine size: 30.
- Reload time: 2 seconds.
- Fire rate: 8 shots per second.
- Hip spread: 3.5 degrees.
- ADS spread: 0.8 degrees.
- Sprint spread: 6.0 degrees.
- Jump spread: 9.0 degrees.

Local projectile visuals are still separate from server hitscan damage. The server shot result event is the authoritative hit confirmation.

## Bot Rules

- Recommended default bots: 8.
- Bots are server-owned and appear in snapshots.
- Bots are intended to be humanoid shooter enemies.
- Current v1 bots use simple server movement, respawn, invincibility, and direct body attack for gameplay pressure.
- Future bot shooter AI should use visible firing, lower accuracy, and readable attack timing.

Debug/demo difficulty values:

| Difficulty | Damage | Cooldown | Detect Range | Attack Range | Speed |
| --- | ---: | ---: | ---: | ---: | ---: |
| Easy | 10 | 1.8s | 1000 | 650 | 400 |
| Normal | 20 | 1.0s | 1500 | 900 | 500 |
| Hard | 25 | 0.7s | 1800 | 1100 | 600 |

Safe demo mode applies easy difficulty, disables bot attacks, disables timer-based match end, clears stale game-over state, resets players/bots/health packs, and keeps the match in progress.

## Health Pack Rules

- Three health packs are active by default.
- Heal amount: +35.
- Respawn delay: 15 seconds.
- Only players pick up health packs for now.
- Bots ignore health packs for now.

## Ranking And Match End

- Kill goal: 20 kills.
- Match duration: 5 minutes.
- Ranking sort:
  1. higher score / kills
  2. higher player kills
  3. higher bot kills
  4. fewer deaths
  5. lower server player ID

The HUD and Tab scoreboard should show the top 5 connected players when available.

## Kill Log

The kill log should prioritize server-authoritative events:

- My kill: green.
- Any player or bot killing the local player: red.
- Other kills and world events: neutral.

Current implementation uses text events in the existing HUD. Dedicated kill feed styling is future work.

## HUD Requirements

Primary HUD:

- Server HP.
- Server kills / goal.
- Ammo and reload state.
- Match timer.
- Bot and health pack counts.
- Recent server shot result.
- Recent kill log events.

Optional debug HUD:

- Local offline HP/score.
- Server position error.
- Correction state.
- Server profile.

Future HUD:

- Animated crosshair spread.
- Bottom HP/ammo presentation.
- Dedicated top-5 ranking panel.
- Large-map minimap / radar.

## Legacy Target/Core System

`TargetState` still exists for debugging and compatibility, but targets/cores are disabled by default for the main PvPvE kill race path.

When disabled:

- Targets are not included as active gameplay objectives.
- Target kills do not affect primary score.
- Unreal target/core ghosts are hidden by default.
- Browser test treats target data as debug-only.

## Current Limitations

- Local projectile visuals and server hitscan damage are separate layers.
- Local offline targets still exist but are no longer the main PvPvE objective.
- Bots use simple server AI, not full shooter behavior or navigation.
- No final humanoid player/bot models yet.
- No weapon model/socket integration yet.
- No animated kill feed, minimap, or production scoreboard UI yet.
- No database, matchmaking, anti-cheat, or advanced lag compensation.
