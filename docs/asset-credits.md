# BattleGrid Asset Credits

BattleGrid currently uses placeholder visuals for prototype readability.

## Current Placeholder Assets

- Built-in Unreal Engine basic shapes are used for server ghost placeholders.
- Server player ghosts use basic sphere placeholders.
- Server projectile ghosts use basic sphere placeholders.
- Server CORE ghosts use basic cube placeholders.
- Server bot ghosts use basic sphere placeholders.
- Server health pack ghosts use basic cube placeholders.
- Custom placeholder materials may be created locally in Unreal Editor for readability.
- No external asset packs are required for the current portfolio demo.

## Local Project Materials

Simple color materials may be created manually in Unreal Editor and assigned to the ghost Blueprint class defaults.

Recommended local placeholder material roles:

- `M_ServerEcho_Alive`
- `M_ServerEcho_Invincible`
- `M_ServerEcho_Down`
- `M_ServerBullet`
- `M_Core_Alive`
- `M_Core_Destroyed`
- `M_Bot_Alive`
- `M_Bot_Invincible`
- `M_Bot_Down`
- `M_HPack_Active`
- `M_HPack_Inactive`

These material names are recommendations only. The C++ classes expose editable material slots and do not require any material asset to exist.

## External Assets

No external art, audio, animation, model, icon, or marketplace assets are imported yet.

## Paragon / Fab Asset Template

BattleGrid may later use Epic/Fab humanoid character assets for player and bot visuals. If those assets are added, keep the game branded as BattleGrid and do not use the Paragon trademark as the game title, logo, or advertising identity.

Record each imported character, weapon, animation, or material here:

- Asset name:
- Creator/vendor:
- Source URL:
- Marketplace/Fab listing:
- License or usage terms:
- Imported path:
- Used for:
- Modified locally:
- Redistribution notes:

Suggested BattleGrid import paths:

- `Content/BattleGrid/Art/Characters`
- `Content/BattleGrid/Art/Bots`
- `Content/BattleGrid/Art/Weapons`
- `Content/BattleGrid/Art/Animations`
- `Content/BattleGrid/Art/ThirdParty/Paragon`

When external assets are added later, record:

- Asset name:
- Creator/vendor:
- Source:
- License:
- Imported path:
- Usage:
- Redistribution notes:

Suggested future asset source categories:

| Source | Likely Use | Notes |
| --- | --- | --- |
| Unreal Marketplace / Fab | characters, weapons, arena props, materials, VFX | Check license and redistribution terms before committing assets. |
| Kenney | prototype icons, UI, low-poly props | Preserve license text and source URL. |
| Mixamo | temporary character animations | Check Adobe/Mixamo usage terms and document animation names. |
| Freesound | temporary audio feedback | Use only assets with compatible licenses and credit requirements. |

Do not redistribute marketplace or third-party assets outside the permissions of their license. If an asset license allows project use but not source redistribution, document it here and keep the asset out of public repository commits as needed.

## Current Limitation

The current visual layer is still a prototype ghost visualization layer. It is intended to make server-authoritative state readable during demo recording, not to represent final production art.
