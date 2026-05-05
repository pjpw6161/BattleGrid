# GCP Deployment

This document describes the current manual GCP VM deployment flow for the BattleGrid C++ server. It assumes Docker Compose is already used locally and that the Unreal client connects over WebSocket.

Do not commit a real external IP address to source code or Blueprint assets. Use the placeholder `ws://<GCP_EXTERNAL_IP>:7777` in documentation.

## Recommended VM Settings

- OS: Ubuntu 24.04 LTS
- Machine type: `e2-medium` for a lightweight demo, or `e2-standard-2` for more headroom
- Boot disk: 30 GB standard or balanced persistent disk
- Network tag: `battlegrid-server`
- Firewall: allow TCP `7777` from the client test network

## Firewall Rule

Create an ingress firewall rule:

- Target tag: `battlegrid-server`
- Protocol/port: `tcp:7777`
- Source range: use your trusted client IP range for demos, or `0.0.0.0/0` only for temporary public testing

## VM Setup

SSH into the VM, then install Docker and Git:

```bash
sudo apt-get update
sudo apt-get install -y ca-certificates curl git
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
sudo usermod -aG docker "$USER"
```

Log out and back in so the Docker group membership applies, or prefix Docker commands with `sudo`.

Verify:

```bash
docker version
docker compose version
```

## Clone Repository

```bash
git clone https://github.com/pjpw6161/BattleGrid.git
cd BattleGrid
```

## Run Server

```bash
docker compose up -d --build
```

The Compose file runs:

```text
./battlegrid-server --host 0.0.0.0 --port 7777 --tick-rate 30
```

## Logs

```bash
docker compose logs -f battlegrid-server
```

Expected logs include startup, listening, accepted sessions, join, input, and snapshot messages.

## Stop

```bash
docker compose down
```

## Update Deployment

```bash
git pull
docker compose up -d --build
docker compose logs -f battlegrid-server
```

## Test From Local Browser

1. Open `tools/websocket-test.html` on your local machine.
2. Set the URL to:

   ```text
   ws://<GCP_EXTERNAL_IP>:7777
   ```

3. Click `Connect`.
4. Send `Ping`, `Join`, and `Debug Room`.
5. Verify snapshots include players, projectiles, and targets.

## Test From Unreal

In Unreal Editor, open the PlayerController Blueprint defaults:

1. Set `bUseRemoteServer` to true.
2. Set `RemoteServerUrl` to:

   ```text
   ws://<GCP_EXTERNAL_IP>:7777
   ```

3. Leave `LocalServerUrl` as `ws://127.0.0.1:7777`.
4. Press Play.
5. Verify the HUD shows `Profile=Remote` and receives snapshots.

## Current Operational Limits

- This is a manual VM deployment.
- There is no CI/CD pipeline yet.
- There is no TLS termination yet.
- There is no authentication.
- There is no production monitoring.
- Firewall exposure should be restricted for demos whenever possible.
