# BattleGrid Demo Script

This is a two-minute recording script for the current portfolio prototype. Keep raw snapshot logging off in the browser test page before recording.

## 2-Minute Timeline

| Time | Shot | Talking Point |
| --- | --- | --- |
| 0:00 | Show README title or project title screen. | BattleGrid is a third-person PvPvE arena shooter prototype built with Unreal Engine C++ and a custom C++20 authoritative WebSocket server. |
| 0:10 | Show Docker server terminal already running, or start it. | The server runs locally with Docker Compose and exposes WebSocket port 7777. |
| 0:20 | Open `tools/websocket-test.html`. | This browser tool tests the JSON protocol directly without Unreal. |
| 0:25 | Click `Apply Safe Demo Mode`. | Safe demo mode resets the match, disables bot attacks, disables timer-based game over, and sets bot difficulty to easy for stable recording. |
| 0:30 | Click `Send Join` if not already joined. | The server allocates a player, joins Room 1, and starts sending snapshots. |
| 0:35 | Click `Fire 5 Shots At Nearest Bot`. | The browser sends server fire inputs using the latest snapshot position. |
| 0:45 | Show `SERVER HIT` / bot HP / bot kill in the browser. | Server hitscan damage, bot HP, events, and scoreboard updates are visible without relying on local projectile logs. |
| 0:55 | Open Unreal Editor or switch to PIE already running. | Unreal is the playable client and renders server state through ghost actors. |
| 1:05 | Show HUD: `Profile: Local`, `Server: Connected`, `SERVER HP`, and server score. | The HUD is server-centric by default while local offline/debug state remains available. |
| 1:15 | Move with WASD, ADS, sprint, and jump. | The client uses third-person shooter controls and keeps local movement responsive. |
| 1:25 | Frame SERVER ECHO, bots, cores, and health packs. | Server snapshots are visualized as readable ghost actors for debugging and demo clarity. |
| 1:35 | Shoot a bot in Unreal. | The server emits explicit shot result feedback such as `SERVER HIT BOT-3 -20` or `SERVER MISS`. |
| 1:45 | Hold Tab to show the scoreboard. | Match state and scoreboard come from the authoritative server snapshot. |
| 1:55 | Show Docker/GCP docs or terminal logs briefly. | The same server can run locally with Docker or on a GCP VM. |
| 2:00 | End on README or Unreal gameplay. | Current status: portfolio prototype, not production networking or final art. |

## Recording Notes

- Use local Docker for the cleanest recording unless the demo specifically needs GCP.
- Keep `Log Raw Snapshots` unchecked in the browser.
- Use `Apply Safe Demo Mode` before every take.
- Keep `bApplySafeDemoModeOnJoin` enabled in the active PlayerController Blueprint for Unreal takes.
- Keep `bUseServerAuthoritativeHud` enabled and `bShowLocalDebugHud` disabled for the main recording.
- Use Tab only when explaining match state and scoring.
- Explain that local projectile visuals and server hitscan are currently separate prototype layers.

## Short Voiceover Version

BattleGrid is a third-person PvPvE arena shooter prototype. The Unreal client handles controls, visuals, HUD, and ghost rendering, while a custom C++20 server owns the match state, bots, health packs, hitscan combat, score, and snapshots. The browser tool verifies the WebSocket JSON protocol directly, and safe demo mode resets the room into a stable test state. In Unreal, server snapshots drive the SERVER ECHO, bot, core, and health pack ghosts. Server shot result events make authoritative hits explicit, and the Tab scoreboard shows the server match state.
