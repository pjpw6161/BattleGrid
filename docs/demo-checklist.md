# BattleGrid Demo Checklist

Use this checklist to run the current portfolio demo.

## Server Smoke Test

### Option A: Docker Compose

1. Start the server container:

   ```powershell
   docker compose up -d --build
   ```

2. Verify logs:

   ```powershell
   docker compose logs -f battlegrid-server
   ```

3. Verify the container exposes port `7777`:

   ```powershell
   docker compose ps
   ```

4. Open `tools/websocket-test.html`.
5. Connect to `ws://127.0.0.1:7777`.
6. Click `Send Ping` and verify `pong`.
7. Click `Send Join` and verify `join_ok`.
8. Click `Send Debug Room` and verify Room 1, player state, and targets.

### Option B: Native Windows Build

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

### Shared Server Test Steps

1. Click `Start Moving Right` and watch snapshots update player `x` / `y`.
2. Click `Send Fire Input` and watch projectiles appear in snapshots.
3. Watch target HP and server score change after projectile hits.

### Option C: Remote GCP Server

1. SSH into the GCP VM.
2. Start the server:

   ```bash
   docker compose up -d --build
   ```

3. Watch logs:

   ```bash
   docker compose logs -f battlegrid-server
   ```

4. From the local browser test page, connect to:

   ```text
   ws://<GCP_EXTERNAL_IP>:7777
   ```

5. Click `Send Ping`, `Send Join`, and `Send Debug Room`.
6. Verify snapshots include players, projectiles, and targets from the remote server.

## Unreal Demo

1. Build `BattleGridClientEditor` in Visual Studio with `Development Editor | Win64`.
2. Open `client-unreal/BattleGridClient/BattleGridClient.uproject`.
3. For local server mode:
   - Set `bUseRemoteServer` to false.
   - Set `LocalServerUrl` to `ws://127.0.0.1:7777`.
4. For remote GCP server mode:
   - Set `bUseRemoteServer` to true.
   - Set `RemoteServerUrl` to `ws://<GCP_EXTERNAL_IP>:7777`.
5. Press Play.
6. Verify the HUD shows:
   - Local HP.
   - Local score.
   - Server score.
   - Server connection.
   - Profile label, either `Profile=Local` or `Profile=Remote`.
   - Player ID and room ID.
   - Snapshot tick.
   - Server target count.
   - Server projectile count.
   - Position error and correction state.
7. Move with WASD.
8. Aim with the mouse.
9. Fire with left mouse button.
10. Show local projectile and local target damage.
11. Show local player hazard damage, death, and respawn.
12. Show server player ghost movement.
13. Show server projectile ghost movement.
14. Show server target ghost labels and HP.
15. Show server score increasing when server targets are destroyed.
16. Switch from local mode to remote mode and show the HUD profile label changing.
17. Show server ghosts driven by the remote GCP server.
18. Toggle optional server position correction in the PlayerController Blueprint defaults if needed, then compare error behavior.
19. Stop the server and verify local offline gameplay still works.

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
