# ⚔️ Battle Grid

### C++20 WebSocket 권위 서버 기반 멀티플레이 PvPvE 슈팅 게임

*Unreal Engine 5 클라이언트와 C++20 WebSocket 서버를 연동하여 로비, 룸, 매치, 전투, 리스폰까지 직접 구현한 실시간 멀티플레이 프로젝트*

<img width="800" height="500" alt="2026-05-21 20 48 56 (1)" src="https://github.com/user-attachments/assets/1a9c4168-dda8-4919-bae4-b9aef0c31fdf" />


<br />

<div align="center">

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5-0E1128?style=for-the-badge&logo=unrealengine&logoColor=white)
![C++20](https://img.shields.io/badge/C++20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![WebSocket](https://img.shields.io/badge/WebSocket-JSON-6A5ACD?style=for-the-badge)
![Docker](https://img.shields.io/badge/Docker%20Compose-2496ED?style=for-the-badge&logo=docker&logoColor=white)
![GCP](https://img.shields.io/badge/GCP%20Compute%20Engine-4285F4?style=for-the-badge&logo=googlecloud&logoColor=white)

</div>

---

## 📋 목차

- [프로젝트 소개](#-프로젝트-소개)
- [핵심 특징](#-핵심-특징)
- [주요 기능](#-주요-기능)
- [기술 스택](#-기술-스택)
- [시스템 아키텍처](#-시스템-아키텍처)
- [네트워크 프로토콜](#-네트워크-프로토콜)
- [프로젝트 구조](#-프로젝트-구조)
- [트러블슈팅](#-트러블슈팅)
- [프로젝트 통계](#-프로젝트-통계)
- [역할 및 기여](#-역할-및-기여)

---

## 🎯 프로젝트 소개

### 왜 Battle Grid?

> "Unreal 기본 Replication 없이도 직접 만든 C++ 서버로 멀티플레이 게임을 구성할 수 있을까?"

**Battle Grid**는 Unreal Engine 5 클라이언트와 독립 실행형 **C++20 WebSocket 권위 서버**를 연동한 멀티플레이 PvPvE 슈팅 게임입니다.

클라이언트는 WebSocket을 통해 닉네임 설정, 로비, 방 생성/입장, 준비, 매치 시작, 이동, 사격 정보를 서버에 전송합니다. 서버는 GameRoom 단위로 플레이어, 봇, 체력팩, 전투 판정, 점수, 사망/리스폰 상태를 관리하고, 모든 클라이언트에게 `snapshot`과 `event`를 브로드캐스트합니다.

Unreal의 기본 네트워크 Replication을 사용하지 않고, **직접 설계한 WebSocket + JSON 프로토콜**로 멀티플레이 흐름을 구성한 것이 핵심입니다.

---

## 🚀 핵심 특징

- 🧠 **C++20 권위 서버**: 서버가 HP, 킬, 점수, 사망, 리스폰 등 핵심 상태를 최종 판정
- 🔌 **WebSocket + JSON 통신**: Unreal 클라이언트와 서버가 실시간으로 메시지 송수신
- 🏠 **로비/룸 시스템**: 닉네임 입력, 방 생성, 방 입장, 준비, 시작 흐름 구현
- 🎮 **PvPvE 전투**: 플레이어 간 전투와 봇 전투를 하나의 전투 흐름으로 통합
- 🤖 **봇 AI / 체력팩 / 리스폰**: 서버 Tick 루프 기반 봇, 체력팩, 리스폰 상태 관리
- 🛰️ **Snapshot/Event 동기화**: 위치, HP, 전투 이벤트, HUD를 서버 상태 기준으로 동기화
- 🗺️ **Runtime Map Marker Sync**: Unreal 레벨에 배치한 스폰/힐팩 마커를 서버 룸 상태로 전송
- ☁️ **GCP + Docker 배포**: Docker Compose로 C++ 서버를 컨테이너화하여 GCP VM에 배포

---

## ✨ 주요 기능

### 1. 🧑‍💻 닉네임 입력 및 서버 접속

<img width="800" height="386" alt="2026-05-22 18 24 00 (1)" src="https://github.com/user-attachments/assets/e15e8917-cac9-4804-bf28-a67c09ead3c0" />

- 클라이언트 실행 시 GCP 또는 로컬 서버로 WebSocket 자동 접속
- 닉네임 입력 후 `set_nickname` 메시지 전송
- 서버 응답을 기준으로 로비 화면 진입
- 로비 상태에서는 캐릭터/무기/HUD를 숨겨 UI 흐름과 게임플레이 흐름을 분리

---

### 2. 🏠 로비 / 방 생성 / 방 입장 / 준비 / 시작

<img width="800" height="386" alt="2026-05-22 18 24 00 (2)" src="https://github.com/user-attachments/assets/4d9da769-43df-46c2-b605-daf7d98b4839" />


- 방 생성, 방 목록 조회, 방 입장 기능 구현
- 방장/비방장 상태에 따라 시작/준비 버튼 표시 제어
- 플레이어 목록, 준비 상태, 방 상태를 서버 `room_state` 기준으로 갱신
- ESC 메뉴의 나가기 확인/취소 흐름 등 UI 상태 관리

---

### 3. 🎮 인게임 멀티플레이 전투

<img width="800" height="386" alt="2026-05-22 18 24 00 (3)" src="https://github.com/user-attachments/assets/1f82bc39-41d6-4786-b0e1-1fe40a5ea86c" />


- 서버 snapshot을 기반으로 원격 플레이어 위치/닉네임/체력 상태 동기화
- 로컬 클라이언트는 사격 입력과 시각 효과를 즉시 처리
- 서버는 `client_hit_claim`을 검증하여 최종 피해/킬/점수를 확정
- 확정된 전투 이벤트를 기준으로 피격 이펙트와 HUD 업데이트 수행

---

### 4. 🤖 PvE 봇 전투 및 체력팩

- 서버에서 봇 위치, HP, 공격 쿨다운, 타겟 플레이어 관리
- 봇 공격 이벤트를 클라이언트가 받아 트레이서/VFX로 표현
- 체력팩 획득 시 서버에서 회복 처리 후 클라이언트 HUD 갱신
- 봇/플레이어 피격 이펙트를 공통 impact VFX 흐름으로 통합

---

### 5. 🗺️ 런타임 맵 마커 기반 스폰/바운더리 동기화

<img width="2567" height="352" alt="image" src="https://github.com/user-attachments/assets/5911c4cd-6376-45c5-8f8c-5e4046c7c3de" />

- Unreal 레벨에 배치한 `BG_Spawn_*`, `BG_HealSpawn_*` 액터를 런타임에 스캔
- 마커 위치를 `debug_set_map_markers` 메시지로 서버에 전송
- 서버는 해당 Room의 player spawn, heal spawn, arena bounds를 갱신
- 클라이언트는 서버 bounds를 기준으로 이동 가능 범위와 바운더리 처리

---



## 🛠 기술 스택

### Client

| 기술 | 용도 |
|------|------|
| Unreal Engine 5 | 클라이언트 개발, 레벨 구성, UMG UI, 캐릭터/전투 표현 |
| C++ | PlayerController, WebSocket 연동, 전투/VFX/HUD 동기화 |
| UMG | 닉네임, 로비, 방, 인게임 HUD, 일시정지 UI |
| Niagara / Spline Mesh | 총알 트레이서, 피격 이펙트, 시각 피드백 |
| Animation Montage | 사격, 피격, 사망 연출 |

### Server

| 기술 | 용도 |
|------|------|
| C++20 | 독립 게임 서버 구현 |
| Boost.Asio / Beast | WebSocket 세션 및 비동기 통신 |
| nlohmann/json | JSON 프로토콜 직렬화/역직렬화 |
| GameRoom Tick Loop | 방 단위 매치 상태 갱신 |
| CMake | 서버 빌드 구성 |

### Network / Protocol

| 기술 | 용도 |
|------|------|
| WebSocket | 클라이언트-서버 실시간 양방향 통신 |
| JSON Protocol | `join`, `room_state`, `snapshot`, `client_hit_claim` 등 메시지 표현 |
| Snapshot/Event | 위치, HP, 점수, 전투 결과, 리스폰 동기화 |
| Server Authority | 서버 기준 피해/킬/리스폰 확정 |

### Infrastructure

| 기술 | 용도 |
|------|------|
| Docker | C++ 서버 컨테이너화 |
| Docker Compose | 서버 실행 및 포트 매핑 관리 |
| GCP Compute Engine | 원격 게임 서버 배포 |
| VPC Firewall | 외부 TCP 포트 접속 허용 |
| Static External IP | 클라이언트 접속 주소 고정 |

### Development Tools

```text
- IDE: Visual Studio 2022, Unreal Editor
- Version Control: Git, GitHub
- Build: Unreal Build Tool, CMake, Docker Compose
- Test: Packaged EXE, PIE, Browser WebSocket Test Tool
- Platform: Windows Client, Linux Docker Server
```

---

## 🏗 시스템 아키텍처

<img width="1672" height="941" alt="image (25)" src="https://github.com/user-attachments/assets/497325f9-4307-4407-9a66-167e46da05f8" />

### 전체 구조

```text
┌────────────────────────────┐
│ Unreal Engine 5 Client      │
│ - Lobby / Room / HUD UI     │
│ - Input / Fire / VFX        │
│ - Snapshot 기반 동기화       │
└──────────────┬─────────────┘
               │ WebSocket / JSON
               ▼
┌────────────────────────────┐
│ GCP VM + Docker             │
│ BattleGrid C++20 Server     │
│ - Boost.Asio / Beast        │
│ - MessageDispatcher         │
│ - GameRoom Tick Loop        │
│ - JsonProtocol Snapshot     │
└──────────────┬─────────────┘
               │ Snapshot / Event Broadcast
               ▼
┌────────────────────────────┐
│ Client Visual Sync          │
│ - Remote Player             │
│ - Bot / HealthPack          │
│ - HP / Score / Respawn      │
│ - Impact VFX / HUD          │
└────────────────────────────┘
```

### 서버 모듈 흐름

```text
WebSocket Session
      ↓
MessageDispatcher
      ↓
GameRoom
      ↓
Authoritative Game State
(Player / Bot / HealthPack / Combat / Score / Respawn / Bounds)
      ↓
JsonProtocol
      ↓
Snapshot + Events Broadcast
```

---

## 🔌 네트워크 프로토콜

### 클라이언트 → 서버

#### 닉네임 설정

```json
{
  "type": "set_nickname",
  "nickname": "player1"
}
```

#### 방 생성

```json
{
  "type": "create_room"
}
```

#### 매치 시작

```json
{
  "type": "start_match"
}
```

#### 입력 전송

```json
{
  "type": "input",
  "seq": 42,
  "player_id": 1,
  "move_x": 1.0,
  "move_y": 0.0,
  "fire": true,
  "reload": false,
  "jump": false,
  "shot_dir_x": 0.9,
  "shot_dir_y": 0.1,
  "shot_dir_z": 0.0
}
```

#### 피격 요청

```json
{
  "type": "client_hit_claim",
  "shot_id": 23,
  "target_player_id": 2,
  "hit_zone": "head",
  "headshot": true,
  "damage": 40,
  "hit_x": 2519.29,
  "hit_y": -2735.18,
  "hit_z": 61.59
}
```

### 서버 → 클라이언트

#### 룸 상태

```json
{
  "type": "room_state",
  "room_id": 12,
  "state": "waiting",
  "players": 2,
  "host": 1
}
```

#### 스냅샷

```json
{
  "type": "snapshot",
  "room_id": 12,
  "tick": 1024,
  "players": [],
  "bots": [],
  "health_packs": [],
  "scoreboard": [],
  "events": []
}
```

#### 전투 이벤트

```json
{
  "event_id": 31,
  "type": "player_hit_player",
  "actor_player_id": 1,
  "target_player_id": 2,
  "damage": 20,
  "headshot": false
}
```

---

## 📁 프로젝트 구조

```text
BattleGrid/
├── server/
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp
│       ├── game/
│       │   ├── GameRoom.h
│       │   └── GameRoom.cpp
│       └── protocol/
│           ├── MessageDispatcher.h
│           ├── MessageDispatcher.cpp
│           ├── JsonProtocol.h
│           └── JsonProtocol.cpp
│
├── client-unreal/
│   └── BattleGridClient/
│       ├── BattleGridClient.uproject
│       ├── Config/
│       ├── Content/
│       │   └── BattleGrid/
│       │       ├── Map/
│       │       ├── UI/
│       │       ├── Blueprints/
│       │       └── FX/
│       └── Source/
│           └── BattleGridClient/
│               ├── BattleGridClientPlayerController.cpp
│               ├── BattleGridNetworkSubsystem.cpp
│               ├── BattleGridWeaponComponent.cpp
│               ├── BattleGridServerPlayerGhostActor.cpp
│               ├── BattleGridServerBotGhostActor.cpp
│               └── BattleGridArenaBoundaryActor.cpp
│
├── docs/
│   ├── protocol.md
│   ├── server-architecture.md
│   ├── troubleshooting.md
│   └── demo-checklist.md
│
├── compose.yml
├── Dockerfile
└── websocket-test.html
```

---


### GCP 서버 구성

| 항목 | 내용 |
|------|------|
| Cloud | GCP Compute Engine |
| Server Runtime | Docker Container |
| External Port | 8080 |
| Internal Port | 7777 |
| Protocol | WebSocket / TCP |
| Client URL | `ws://<GCP_EXTERNAL_IP>:8080` |


## 🧯 트러블슈팅

### 1. GCP 서버 접속 실패

```text
증상: WebSocket connection failed
원인: 서버 미실행, 방화벽 포트 미개방, 포트 매핑 오류
확인: docker ps, docker port battlegrid-server, Test-NetConnection
```

### 2. 패키징 EXE에서 검은 화면

```text
증상: 게임 시작 후 검은 화면에서 멈춤
원인: 패키징 맵 목록 누락, 기본 맵 설정 오류, UI 상태 전환 실패
확인: Project Settings → Maps & Modes / Packaging
```

### 3. 플레이어 이동 불가

```text
증상: 총/점프는 되지만 WASD 이동 불가
원인: 입력 모드가 UIOnly로 남음, 죽음 상태 Lock, 숨은 충돌체
확인: InputMode, ServerDeathLocksInput, Player Collision View
```

### 4. 멀티에서 이동 가능 범위가 줄어듦

```text
증상: 솔로에서는 정상인데 멀티에서 바운더리가 작아짐
원인: 비호스트가 demo_arena_v1 기본 bounds를 적용
해결: level_runtime bounds만 accepted bounds로 사용하고 stale bounds 무시
```

### 5. 디버그 오브젝트가 보임

```text
증상: 스폰 큐브, 힐팩 큐브, 서버 디버그 텍스트가 인게임에 노출
해결: Runtime marker visuals hidden, code-generated debug UI disabled
```

---

## 📊 프로젝트 통계

| 항목 | 내용 |
|------|------|
| 개발 기간 | 2026.04 ~ 2026.05 |
| 참여 인원 | 1명 |
| 장르 | 멀티플레이 PvPvE 슈팅 |
| 클라이언트 | Unreal Engine 5 |
| 서버 | C++20 WebSocket Server |
| 배포 | GCP Compute Engine + Docker Compose |
| 통신 방식 | WebSocket + JSON |
| 동기화 방식 | Snapshot / Event |
| 기본 봇 수 | 8개 |
| 체력팩 | Runtime Marker 기반 배치 |
| 주요 UI | Nickname, Lobby, Room, Combat HUD, Pause |

---

## 🙋 역할 및 기여

**박지원 / 1인 개발**

- C++20 WebSocket 권위 서버 구현
- GameRoom 기반 매치 상태, 봇, 체력팩, 전투 판정 관리
- Unreal Engine 5 클라이언트 C++ 연동
- 닉네임, 로비, 방 생성/입장, 준비/시작 UI 흐름 구현
- 서버 snapshot/event 기반 플레이어, 봇, 체력, HUD 동기화
- 원격 플레이어 비주얼, 닉네임, 히트박스, 사망/리스폰 표현 구현
- 총알 트레이서, 피격 VFX, 카메라 반동 등 전투 피드백 구현
- Docker Compose 기반 서버 컨테이너화
- GCP VM 배포, 포트 매핑, 방화벽 설정, 원격 접속 테스트
- 패키징 EXE 실행 이슈 및 디버그 오브젝트 노출 문제 해결

---

<div align="center">

### ⚔️ Battle Grid

*C++20 WebSocket Authoritative Server × Unreal Engine 5 Multiplayer Shooter*

</div>
