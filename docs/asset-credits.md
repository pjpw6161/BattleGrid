# BattleGrid Asset Credits

BattleGrid currently uses placeholder visuals for prototype readability.

## Current Placeholder Assets

- Built-in Unreal Engine basic shapes are used for server ghost placeholders.
- Server player ghosts use basic sphere placeholders.
- Server projectile ghosts use basic sphere placeholders.
- Server CORE ghosts use basic cube placeholders.
- Server bot ghosts use basic sphere placeholders.
- Server health pack ghosts use basic cube placeholders.

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

When external assets are added later, record:

- asset name
- creator/vendor
- license
- source URL
- imported path
- usage notes

## Current Limitation

The current visual layer is still a prototype ghost visualization layer. It is intended to make server-authoritative state readable during demo recording, not to represent final production art.
