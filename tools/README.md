# BattleGrid Tools

This directory contains small local testing tools for the BattleGrid project.

## `websocket-test.html`

Browser-based WebSocket protocol test page for the custom C++ server.

Use it to:

- Connect to `ws://127.0.0.1:7777`.
- Send `ping`.
- Send `join`.
- Send movement input.
- Send fire input.
- Request `debug_room`.
- Watch `snapshot` messages.
- Inspect player, projectile, target, target HP, and score state.

## Typical Test Flow

1. Start the server:

   ```powershell
   .\server\build\Debug\battlegrid-server.exe --host 127.0.0.1 --port 7777 --tick-rate 30
   ```

2. Open `tools/websocket-test.html`.
3. Click `Connect`.
4. Click `Send Join`.
5. Click `Send Debug Room`.
6. Click `Start Moving Right`.
7. Click `Send Fire Input`.
8. Confirm snapshots include `players`, `projectiles`, and `targets`.

## Notes

- Do not open the test script from restricted browser pages such as `chrome://`.
- If port `7777` is in use, stop the old server process or update the page/server URL for the test.
