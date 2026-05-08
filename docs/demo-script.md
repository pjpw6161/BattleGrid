# BattleGrid Demo Script

This is the final two-minute recording script for the PvPvE Kill Race prototype. Keep the browser raw snapshot checkbox off and keep Unreal debug HUD/controls help off unless a shot specifically needs debugging.

## 2-Minute Timeline

| Time | Shot | Talking Point |
| --- | --- | --- |
| 0:00 | Show README title or project title screen. | BattleGrid is a third-person PvPvE Kill Race shooter built with Unreal Engine C++ and a custom C++20 authoritative WebSocket server. |
| 0:10 | Show Docker server terminal already running, or start it. | The local demo server runs with Docker Compose on WebSocket port 7777. |
| 0:20 | Open `tools/websocket-test.html`. | The browser test page can drive the protocol directly and apply one-click demo presets. |
| 0:30 | Click `Apply Combat Demo`. | Combat Demo resets the room, enables easy bot attacks, disables timer auto-end, resets health packs, and clears stale game-over state. |
| 0:40 | Click `Fire 5 Shots At Nearest Bot`. | The server receives fire input, applies hitscan damage, and returns shot result events without relying on local projectile damage. |
| 0:50 | Open Unreal Editor or switch to PIE already running. | Unreal is the playable client and renders server-owned state through the gameplay HUD and ghost actors. |
| 1:00 | Show HUD: HP, ammo, top 5 ranking, kill feed, and crosshair. | The normal demo HUD is game-like: debug details, controls help, and legacy target/core UI are hidden by default. |
| 1:10 | Move, ADS, sprint, and jump. | The client provides responsive third-person controls while syncing position to the server for this prototype demo. |
| 1:20 | Shoot a bot in Unreal. | Server feedback appears as `SERVER HIT`, `SERVER HEADSHOT`, bot HP loss, tracer ghosts, ranking updates, and kill feed entries. |
| 1:35 | Let a bot damage the player briefly. | Bot shooter AI applies server-owned HP damage; death shows a respawn countdown and respawn invincibility when it happens. |
| 1:45 | Pick up a health pack. | Health packs are server-authoritative, restore +35 HP, become inactive, and respawn after a delay. |
| 1:55 | Hold Tab for scoreboard or show final browser summary. | Ranking and match state come from the server snapshot; the demo is ready for local Docker or documented GCP deployment. |

## Recording Notes

- For a clean model/HUD pass, use `Apply Safe Visual Demo` or `DemoPresetOnJoin=safe_visual`.
- For the main combat take, use `Apply Combat Demo` or `DemoPresetOnJoin=combat_demo`.
- For match-ending behavior, use `Apply Match Demo`.
- Keep `Log Raw Snapshots` unchecked.
- Keep `bShowDebugHud=false`, `bShowControlsHelp=false`, `bShowServerTargetGhosts=false`, and `bShowServerProjectileGhosts=true`.
- Use Tab only when explaining ranking or match state.
- Avoid showing legacy local target/core debug unless explicitly explaining offline test systems.

## Short Voiceover Version

BattleGrid is a third-person PvPvE Kill Race shooter prototype. The Unreal client handles controls, visuals, HUD, and local feedback, while the custom C++20 server owns player HP, bot shooter AI, health packs, hitscan combat, ranking, match flow, combat events, and snapshots. The browser page applies one-click demo presets and validates the WebSocket protocol directly. In Unreal, the clean HUD shows HP, ammo, ranking, kill feed, and crosshair while server events drive hit feedback, death/respawn, health pickup, and scoreboard updates.
