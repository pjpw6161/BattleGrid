# BattleGrid Server Architecture

## Current Scope

The custom C++20 server currently accepts WebSocket JSON messages and stores minimal authoritative room state for local testing.

## Main Components

- `GameServer`: owns startup/shutdown flow and starts the WebSocket server.
- `WebSocketServer`: owns the Boost.Asio `io_context`, TCP acceptor, fixed `RoomManager`, player ID allocator, active session list, and fixed-rate tick timer.
- `Session`: owns one WebSocket connection, per-session join state, and an async outgoing message queue.
- `MessageDispatcher`: parses protocol messages and mutates session/room state.
- `RoomManager`: owns the fixed default `GameRoom`.
- `GameRoom`: thread-safe storage and movement simulation for players in Room 1.
- `PlayerState`: server-side player identity, nickname, connection flag, latest input, position, HP, and score.
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

## Game Tick

`WebSocketServer` schedules a Boost.Asio `steady_timer` at `1 / tickRate` seconds using the configured server tick rate.

On each tick:

1. `tickNumber` increments.
2. `RoomManager::TickAll` ticks the fixed default room.
3. `GameRoom::Tick` integrates each connected player's latest movement input into `x` / `y`.
4. `RoomManager::BuildDefaultRoomSnapshotJson` builds a snapshot for Room 1.
5. `WebSocketServer` broadcasts compact snapshot JSON to all joined active sessions.

Movement is intentionally simple for now. The latest move vector is normalized if its length is greater than 1, applied at `PlayerState::speed`, then clamped to a square arena from `-2000` to `2000` on both axes.

## Session Send Queue

`Session::SendText` posts server-initiated messages onto the WebSocket executor, pushes them into a `std::deque`, and starts an async write only when no write is already active. When a write finishes, the next queued message is sent.

This queue is used by both direct protocol responses and server-initiated snapshot broadcasts, preventing overlapping `async_write` calls on the same WebSocket stream.

## Debugging

`debug_room` returns the current Room 1 state as JSON, including all connected players, position fields, HP, score, and latest stored input. This is for browser testing only.

## Current Limitations

- Only one fixed room exists.
- Player IDs reset when the process restarts.
- Movement simulation is simple position integration, not validated against collisions or game rules.
- There is no Unreal snapshot rendering yet.
- There is no projectile simulation, collision, damage, score update, respawn, authentication, database, or multi-room matchmaking yet.
