# BattleGrid Demo Checklist

Use this checklist to run the current portfolio demo.

## Server Smoke Test

1. Build the server:

   ```powershell
   cmake --build server/build --config Debug
   ```

2. Start the server:

   ```powershell
   .\server\build\Debug\battlegrid-server.exe --host 127.0.0.1 --port 7777 --tick-rate 30
   ```

3. Open `tools/websocket-test.html`.
4. Click `Connect`.
5. Click `Send Ping` and verify `pong`.
6. Click `Send Join` and verify `join_ok`.
7. Click `Send Debug Room` and verify Room 1, player state, and targets.
8. Click `Start Moving Right` and watch snapshots update player `x` / `y`.
9. Click `Send Fire Input` and watch projectiles appear in snapshots.
10. Watch target HP and server score change after projectile hits.

## Unreal Demo

1. Build `BattleGridClientEditor` in Visual Studio with `Development Editor | Win64`.
2. Open `client-unreal/BattleGridClient/BattleGridClient.uproject`.
3. Press Play.
4. Verify the HUD shows:
   - Local HP.
   - Local score.
   - Server score.
   - Server connection.
   - Player ID and room ID.
   - Snapshot tick.
   - Server target count.
   - Server projectile count.
   - Position error and correction state.
5. Move with WASD.
6. Aim with the mouse.
7. Fire with left mouse button.
8. Show local projectile and local target damage.
9. Show local player hazard damage, death, and respawn.
10. Show server player ghost movement.
11. Show server projectile ghost movement.
12. Show server target ghost labels and HP.
13. Show server score increasing when server targets are destroyed.
14. Toggle optional server position correction in the PlayerController Blueprint defaults if needed, then compare error behavior.
15. Stop the server and verify local offline gameplay still works.

## Victory And Restart

1. Place enough local damageable targets for the configured local `TargetScore`.
2. Destroy local targets.
3. Verify local victory message.
4. Press R to restart the level.

## Demo Talking Points

- BattleGrid demonstrates both Unreal gameplay programming and custom C++ server programming.
- The ghost layer makes server state visible without hiding local gameplay.
- Local score and server score are separate on purpose during the bring-up phase.
- The next step is to merge local and server gameplay layers into a stronger authoritative flow.
