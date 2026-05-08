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
- Respawn grants difficulty-tuned invincibility, with easy giving the player more recovery time.
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

Connected server mode uses server hitscan, server tracer ghosts, and server shot result events as the primary fire feedback.
The legacy local sphere projectile remains for offline/local testing and is disabled by default while connected and joined to the server.
The server shot result event is the authoritative hit confirmation.

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
- Bot pressure is capped per player so the whole bot squad does not shoot one player at once.
- Bots that are near a player but blocked by the pressure cap can still face/aim at the player in an `aim` or `suppressed` combat state.
- Bot shots include small randomized cooldown jitter to avoid synchronized volleys.
- Bots are constrained to a smaller demo bot area (`x=-1500..1500`, `y=-900..900`) because the server does not yet know Unreal map collision or NavMesh.
- When not chasing a player, bots move between fixed demo waypoints inside that area. Basic stuck detection picks a new waypoint if a bot cannot make progress.
- If bot attacks are disabled for safe demos, bots still move/chase/wander but do not shoot players.

Debug/demo difficulty values:

| Difficulty | Body / Head | Fire Interval | Spread | Detect Range | Attack Range | Speed | Max Targeting | Max Shooting | Respawn Invincible |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Easy | 10 / 20 | 1.0s | 18 deg | 1400 | 1100 | 380 | 8 | 2 | 2.5s |
| Normal | 10 / 20 | 0.75s | 13 deg | 1600 | 1300 | 450 | 8 | 3 | 2.0s |
| Hard | 10 / 20 | 0.5s | 8 deg | 1900 | 1600 | 550 | 8 | 4 | 1.5s |

All difficulties keep bot damage at 10 body / 20 headshot. Difficulty changes pressure through accuracy, fire interval, movement speed, range, maximum concurrent bot shooting, and respawn recovery time.

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

## HUD Layout

The default demo HUD is gameplay-first instead of a large debug overlay:

- Bottom center: server HP and local weapon ammo/reload state.
- Top right: Top 5 player ranking based on total kills.
- Left side: recent 5 kill/death events.
- Center: dynamic spread-based crosshair.
- Small status: match time and local/remote server connection state.
- Temporary center message: scoreboard, server death/respawn, server shot result, or game over.

Debug details are hidden by default. Enable the PlayerController HUD debug options only when comparing local/offline state with server state.

Optional debug HUD content:

- Local offline HP/score.
- Server position error.
- Correction state.
- Client-position sync state.
- Server profile, snapshot tick, arena bounds, and bot-area bounds.

Future HUD:

- More polished top-5 ranking panel art.
- Large-map minimap / radar.

## Legacy Target/Core System

`TargetState` still exists for debugging and compatibility, but targets/cores are disabled by default for the main PvPvE kill race path.

When disabled:

- Targets are not included as active gameplay objectives.
- Target kills do not affect primary score.
- Unreal target/core ghosts are hidden by default.
- Browser test treats target data as debug-only.

## Current Limitations

- The legacy local projectile and local target layer still exist for offline testing, but the connected PvPvE demo uses server tracers and server hit markers as the primary shot feedback.
- Local offline targets still exist but are no longer the main PvPvE objective.
- Bots use simple server shooter AI, not full navigation, cover, animations, or advanced target selection.
- No final humanoid player/bot models yet.
- Weapon model/socket integration is prepared in C++ but still needs manual asset assignment and offset tuning.
- No animated kill feed, minimap, or production scoreboard UI yet.
- No database, matchmaking, anti-cheat, or advanced lag compensation.
