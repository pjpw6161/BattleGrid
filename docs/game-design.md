# BattleGrid Game Design

BattleGrid is now a third-person PvPvE Kill Race Shooter prototype.

The previous target-objective prototype is legacy debug content. The main game objective is kill-based: connected players and humanoid shooter bots fight in the same arena, and the player with the most kills wins.

This is a portfolio prototype. The server-authoritative kill race loop is being built in stages, while some local Unreal offline systems and debug ghost visuals remain for comparison and testing.

## High Concept

- Third-person over-the-shoulder arena shooter.
- Players fight server-controlled humanoid bots and other players.
- Kills are the score.
- Ranking shows up to 5 connected players at the top-right of the HUD.
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
- Bot damage is half player damage:
  - body: 10
  - headshot: 20
- Bot accuracy is intentionally low for readable demos.
- Death starts an 8 second respawn timer.
- Respawn grants 1.5 seconds of invincibility.
- Health packs heal +35 and cannot raise HP above max.

The current server has hitscan shot result events:

- `SERVER HIT BOT-* -20`
- `SERVER HEADSHOT BOT-* -40`
- `SERVER HIT PLAYER -20`
- `SERVER MISS`
- `BOT HIT YOU -10`
- `BOT HEADSHOT YOU -20`

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

## Crosshair Rules

The HUD crosshair should communicate current weapon accuracy:

- ADS: tight crosshair, green-ish color.
- Hip fire: medium gap.
- Sprinting: wider gap, warm warning color.
- Jumping/falling: widest gap, warm warning color.
- Reloading or server-dead: dimmed/unavailable color.

The current C++ HUD supports optional `CrosshairTop`, `CrosshairBottom`, `CrosshairLeft`, `CrosshairRight`, and `CrosshairCenter` Border widgets. If those are not present, the legacy `CrosshairText` fallback remains safe.

## Bot Rules

- Recommended default bots: 8.
- Bots are server-owned and appear in snapshots.
- Bots are intended to be humanoid shooter enemies.
- Bots detect nearest alive non-invincible players, face the target, and fire low-accuracy server hitscan shots.
- Bots have 30-round magazines, infinite reserve ammo, and 2.5 second reloads.
- Bot shots can spawn visual-only server tracer projectiles for ghost visualization.
- If bot attacks are disabled for safe demos, bots still move/chase/wander but do not shoot players.

Debug/demo difficulty values:

| Difficulty | Body / Head | Fire Interval | Spread | Detect Range | Attack Range | Speed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Easy | 10 / 20 | 0.75s | 18 deg | 1000 | 1200 | 400 |
| Normal | 10 / 20 | 0.5s | 12 deg | 1500 | 1400 | 500 |
| Hard | 10 / 20 | 0.35s | 7 deg | 1800 | 1600 | 600 |

Safe demo mode applies easy difficulty, disables bot attacks, disables timer-based match end, clears stale game-over state, resets players/bots/health packs, and keeps the match in progress.

## Humanoid Visual Direction

- Player and bots are planned to use humanoid Skeletal Mesh assets.
- A simple rifle Static Mesh should attach to a right-hand socket, defaulting to `hand_rSocket`.
- Bot humanoid meshes are visual-only server ghosts; gameplay hit volumes still come from server `BotState`.
- Fab/Epic/Paragon-style assets may be used later, but BattleGrid remains the project name and brand.
- Current C++ exposes the player and bot weapon slots, bot skeletal mesh toggle, humanoid scale, and label-height settings. No external assets are imported yet.

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

The top-right HUD ranking and Tab scoreboard show up to 5 connected players when available. Bots never appear in the player ranking; bot kills only contribute to the owning player's total kills.

## Kill Log

The left-side kill log should prioritize server-authoritative kill/death events:

- My kills: green.
- Any player death caused by another player or bot: red.
- Other players killing bots: neutral white/gray.
- Show the most recent 5 kill-related events.

Current implementation supports optional `KillFeedLine1` through `KillFeedLine5` TextBlocks for color-coded lines, with a fallback text block when those widgets have not been added yet.

## HUD Requirements

Primary HUD:

- Server HP.
- Server kills / goal.
- Top-right Top 5 ranking based on total kills.
- Ammo and reload state.
- Match timer.
- Bot and health pack counts.
- Recent server shot result.
- Left-side recent kill log events.
- Dynamic spread-based crosshair.

Optional debug HUD:

- Local offline HP/score.
- Server position error.
- Correction state.
- Server profile.

Future HUD:

- Bottom HP/ammo presentation.
- More polished top-5 ranking panel.
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
- Bots use simple server shooter AI, not full navigation, cover, animations, or advanced target selection.
- No final humanoid player/bot models yet.
- Weapon model/socket integration is prepared in C++ but still needs manual asset assignment and offset tuning.
- No animated kill feed, minimap, or production scoreboard UI yet.
- No database, matchmaking, anti-cheat, or advanced lag compensation.
