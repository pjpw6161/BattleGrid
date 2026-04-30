\# BattleGrid Project Brief



\## Project Name



BattleGrid



\## One-line Description



BattleGrid는 Unreal Engine C++로 구현한 탑다운 멀티플레이 아레나 게임입니다.  

멀티플레이 동기화와 주요 게임 판정은 GCP에 배포한 Custom C++20 Authoritative Server가 처리합니다.



\## Target Roles



1\. Unreal Game Client Programmer

2\. Game Server Programmer

3\. General C++ Software Engineer



\## Game Genre



Top-down multiplayer arena shooter



\## Core Gameplay



\- WASD 이동

\- 마우스 조준

\- 좌클릭 투사체 공격

\- HP

\- 피격

\- 사망

\- 3초 뒤 리스폰

\- 처치 시 점수 +1

\- 2\~8명 동시 플레이



\## Client Responsibilities



Unreal Engine C++ Client는 다음을 담당합니다.



\- 입력 처리

\- 카메라

\- 캐릭터 표시

\- 투사체 표시

\- HP UI

\- 점수판

\- 피격/사망 이펙트

\- 서버 snapshot 기반 위치 보간



\## Server Responsibilities



Custom C++20 Server는 다음을 담당합니다.



\- WebSocket 연결

\- Session 관리

\- Room 관리

\- 30 TPS game loop

\- 플레이어 이동 계산

\- 투사체 이동 계산

\- 충돌 판정

\- 데미지 계산

\- 점수 계산

\- 리스폰 처리

\- snapshot broadcast



\## Initial Tech Stack



\### Unreal Client



\- Unreal Engine

\- C++

\- Gameplay Framework

\- Enhanced Input

\- UMG

\- WebSockets



\### Server



\- C++20

\- CMake

\- Boost.Asio

\- Boost.Beast

\- JSON

\- GoogleTest



\### Deploy



\- Docker

\- Docker Compose

\- GCP Compute Engine

\- Ubuntu



\## MVP Scope



\### Must Have



\- Unreal에서 캐릭터 이동

\- 투사체 공격

\- HP UI

\- 서버 접속

\- 서버 권한 이동/공격/충돌

\- snapshot 수신 후 캐릭터 갱신

\- GCP 서버 배포

\- bot client 10\~50개 테스트



\### Not Now



\- 로그인

\- DB

\- 랭킹

\- 채팅

\- 복잡한 스킬

\- 아이템

\- 인벤토리

\- Kubernetes

\- 대규모 분산 서버

\- 고퀄리티 그래픽



\## Final Portfolio Message



이 프로젝트는 Unreal Engine C++ 게임 클라이언트 구현 역량과 직접 만든 C++20 실시간 멀티플레이 서버 구현 역량을 함께 보여주는 것을 목표로 합니다.

