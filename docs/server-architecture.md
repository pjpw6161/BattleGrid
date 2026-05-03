# BattleGrid Server Architecture

## Current Scope

The custom C++20 server currently accepts WebSocket JSON messages and stores minimal authoritative room state for local testing.

## Main Components

- `GameServer`: owns startup/shutdown flow and starts the WebSocket server.
- `WebSocketServer`: owns the Boost.Asio `io_context`, TCP acceptor, fixed `RoomManager`, and player ID allocator.
- `Session`: owns one WebSocket connection and per-session join state.
- `MessageDispatcher`: parses protocol messages and mutates session/room state.
- `RoomManager`: owns the fixed default `GameRoom`.
- `GameRoom`: thread-safe storage for players in Room 1.
- `PlayerState`: server-side player identity, nickname, connection flag, and latest input.
- `PlayerInput`: latest client input packet fields for a player.

## Room Model

`RoomManager` creates one default room with `room_id = 1`. This is intentionally fixed for the offline/network bring-up phase.

When a session sends `join`, the dispatcher:

1. Allocates a process-local `player_id`.
2. Adds a `PlayerState` to Room 1.
3. Stores `player_id`, `room_id`, nickname, and joined state on the session.
4. Returns `join_ok`.

When the session disconnects, `Session` removes the player from Room 1 and logs the disconnect.

## Input Model

After joining, a client may send `input` messages. The dispatcher validates that the session is joined and that the message `player_id` matches the session `player_id`.

Accepted input replaces `PlayerState::latestInput` in Room 1 and returns `input_ack`.

## Debugging

`debug_room` returns the current Room 1 state as JSON, including all connected players and their latest stored input. This is for browser testing only.

## Current Limitations

- Only one fixed room exists.
- Player IDs reset when the process restarts.
- Inputs are stored but not simulated.
- There are no snapshots, movement validation, projectile simulation, collision, damage, score, respawn, authentication, database, or multi-room matchmaking yet.
