# Development Log

## Implementation Phases

1. Project brief and repository setup.
2. Unreal C++ project setup from the top-down template.
3. Replacement of click-to-move with BattleGrid input.
4. WASD movement, mouse aim, and fire input logging.
5. Local offline projectile firing.
6. Local health component and damageable target.
7. Local combat HUD, score, and kill message.
8. UMG combat HUD.
9. Player HP, hazard damage, death, and respawn.
10. Offline victory condition, target score, and restart input.
11. C++20 custom server skeleton.
12. Boost.Beast WebSocket echo server.
13. JSON protocol with `ping`, `join`, and errors.
14. Unreal WebSocket connection and join flow.
15. Unreal input messages and server `input_ack`.
16. Server `RoomManager`, `GameRoom`, `PlayerState`, and `PlayerInput`.
17. Fixed-rate server tick, movement simulation, and snapshot broadcast.
18. Unreal server player ghost visualization.
19. Server position error display and optional correction.
20. Server projectile simulation and Unreal projectile ghosts.
21. Server targets, projectile-target collision, server score, and target ghosts.
22. HUD server-authoritative summary.
23. Documentation pass for the portfolio repository.
24. Docker Compose deployment for the custom C++ server.
25. Local Docker validation and server operation cleanup.
26. GCP VM Docker deployment validation.
27. Unreal local/remote server profile support and GCP deployment documentation.
28. Demo polish, log throttling, and recording-oriented HUD text.

## Notable Troubleshooting

- Unreal C++ properties were not visible in Blueprint until the class used the proper `UCLASS` and `UPROPERTY` metadata, the project was rebuilt, and Blueprint assets were refreshed or recreated.
- Visual Studio project files did not show new C++ files until Unreal project files were regenerated.
- Browser WebSocket testing failed from restricted browser pages because of Chrome CSP rules. Opening `tools/websocket-test.html` as a normal file/page avoids that issue.
- `cmake` was unavailable until CMake was installed and added to `PATH`.
- Boost.Asio timer cancellation used the wrong overload on the installed Boost version. The fix was to use the no-argument `cancel()` overload.
- Hazard overlap fired but damage did not apply until actor-level overlap and explicit damage path logging were added.
- Hazard damage also depended on the active pawn being compatible with `ABattleGridClientCharacter`.
- Server and Unreal coordinate directions did not match until the server X/Y to Unreal X/Y mapping was swapped.
- Server projectile direction used the wrong coordinate mapping until aim vectors were converted consistently.
- Short movement taps caused ghost overshoot because release input was not sent immediately. Binding Enhanced Input `Completed` and `Canceled` events fixed stale movement.
- Server targets were missing from browser snapshots until target initialization and snapshot/debug serialization were checked and reinforced.
- Demo recording became hard to follow when input, snapshot, and ack logs printed too often. Step 28 added demo log settings and throttled repetitive logs.

## Current State

BattleGrid is a playable local Unreal arena shooter with a visible custom C++ server-authoritative layer. The server simulates players, projectiles, fixed targets, target HP, and score. Unreal visualizes that server state through ghosts and a detailed HUD summary.

## Next Engineering Focus

- Add bot client and benchmark scripts.
- Polish the demo video and capture repeatable presentation footage.
- Replace local-only target and projectile rules with server-authoritative rules.
- Add prediction history and reconciliation.
- Add server-side player damage and respawn.
