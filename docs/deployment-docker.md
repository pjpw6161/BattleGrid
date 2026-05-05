# Docker Deployment

This document covers local Docker deployment for the BattleGrid C++ server. This is not GCP deployment yet.

## Prerequisites

- Docker Desktop installed and running.
- Docker Compose available through the `docker compose` command.
- Port `7777` available on the host.

Verify Docker:

```powershell
docker version
docker compose version
```

## Build And Run

From the repository root:

```powershell
docker compose up --build
```

Run in the background:

```powershell
docker compose up -d --build
```

The Compose service maps host port `7777` to container port `7777`.

## Logs

```powershell
docker compose logs -f battlegrid-server
```

Expected startup logs include server configuration and successful initialization.

## Stop

```powershell
docker compose down
```

## Verify Port 7777

Check the container:

```powershell
docker compose ps
```

You should see `0.0.0.0:7777->7777/tcp` or similar in the ports column.

On Windows PowerShell, you can also check:

```powershell
netstat -ano | findstr :7777
```

## Test With Browser WebSocket Page

1. Start the container with `docker compose up -d --build`.
2. Open `tools/websocket-test.html`.
3. Use `ws://127.0.0.1:7777`.
4. Click `Connect`.
5. Click `Send Ping` and verify `pong`.
6. Click `Send Join` and verify `join_ok`.
7. Click `Send Debug Room` and verify Room 1 targets.
8. Click `Send Fire Input` or `Start Moving Right` and watch snapshots.

## Test With Unreal

1. Start the Docker container.
2. Open Unreal.
3. Ensure the PlayerController `ServerUrl` is `ws://127.0.0.1:7777`.
4. Press Play.
5. Verify the HUD shows connected server state and snapshot tick.

## Common Errors

### Docker Desktop Not Running

Start Docker Desktop and wait until it reports that the engine is running.

### Port 7777 Already In Use

Another server process may already be listening. Stop the old native server or old container:

```powershell
docker compose down
```

If needed, find the process:

```powershell
netstat -ano | findstr :7777
```

### Build Dependency Errors

The Docker build installs Ubuntu packages inside the image. If package installation fails, check network access and Docker Desktop connectivity.

### Container Exits Immediately

Check logs:

```powershell
docker compose logs battlegrid-server
```

Common causes are bind/listen failures, missing runtime libraries, or command-line argument errors.

### Browser Cannot Connect

Check:

- Container is running.
- Port mapping is visible in `docker compose ps`.
- The page uses `ws://127.0.0.1:7777`.
- Firewall or security software is not blocking Docker networking.
