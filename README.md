\# BattleGrid



BattleGrid는 Unreal Engine C++로 구현한 탑다운 멀티플레이 아레나 게임입니다.



Unreal 클라이언트는 입력, 카메라, 캐릭터 표현, UI, 이펙트, snapshot interpolation을 담당하고, GCP에 배포된 Custom C++20 Authoritative Server는 세션, 룸, 게임 루프, 이동 검증, 투사체, 충돌, 데미지, 점수, 리스폰을 처리합니다.



\## Project Structure



```text

battlegrid/

&#x20; client-unreal/   # Unreal Engine C++ client

&#x20; server/          # Custom C++20 authoritative server

&#x20; tools/           # Bot client and test tools

&#x20; docs/            # Architecture, protocol, deployment, benchmark docs

