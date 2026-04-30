\# AGENTS.md - BattleGrid



\## Project Summary



BattleGrid is an Unreal Engine C++ top-down multiplayer arena game with a custom C++20 authoritative server.



The Unreal client handles:

\- input

\- camera

\- character visuals

\- projectile visuals

\- UI

\- effects

\- snapshot interpolation



The custom C++20 server will handle:

\- sessions

\- rooms

\- game loop

\- movement validation

\- projectile simulation

\- collision

\- damage

\- score

\- respawn

\- snapshot broadcast



\## Role Split



The human developer edits Unreal Editor assets and settings.



Codex edits source code and text files only.



\## Allowed Files for Codex



Codex may edit:



\- client-unreal/BattleGridClient/Source/\*\*

\- client-unreal/BattleGridClient/\*.uproject only when necessary

\- client-unreal/BattleGridClient/Config/\*.ini only when explicitly requested

\- server/\*\*

\- tools/\*\*

\- docs/\*\*

\- README.md

\- PROJECT\_BRIEF.md

\- AGENTS.md

\- .gitignore



\## Files Codex Must Not Modify Unless Explicitly Requested



Do not modify, delete, or generate these files:



\- client-unreal/BattleGridClient/Content/\*\*/\*.uasset

\- client-unreal/BattleGridClient/Content/\*\*/\*.umap

\- client-unreal/BattleGridClient/Binaries/\*\*

\- client-unreal/BattleGridClient/Intermediate/\*\*

\- client-unreal/BattleGridClient/Saved/\*\*

\- client-unreal/BattleGridClient/DerivedDataCache/\*\*

\- .vs/\*\*

\- any generated Visual Studio solution/user files



Unreal assets such as Input Actions, Mapping Contexts, Blueprints, maps, widgets, and Niagara assets should be created or modified by the human developer inside Unreal Editor.



\## Unreal C++ Rules



\- Keep existing Unreal class names unless explicitly asked to rename them.

\- Prefer small focused changes.

\- Do not remove existing template code unless the task specifically says to replace it.

\- When changing Unreal C++ code, mention any required Unreal Editor steps separately.

\- Do not claim the project builds unless a build command was actually run.

\- If build verification is not possible, say exactly what the human should build in Visual Studio.



\## Current Client Direction



The Unreal client should become a top-down arena shooter.



Target input behavior:

\- WASD movement

\- mouse aim

\- left click fire

\- later: projectile, HP, score, respawn, server connection



\## Current Networking Direction



Initial networking will use:

\- WebSocket

\- JSON

\- custom protocol



Later improvements may include:

\- binary protocol

\- UDP movement synchronization

\- prediction/interpolation improvements



\## Communication Style



After each task, Codex should summarize:



1\. Files changed

2\. What changed

3\. What the human must do in Unreal Editor

4\. How to verify the change

5\. Any risks or follow-up tasks

