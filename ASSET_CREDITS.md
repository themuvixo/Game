# Asset Credits

The build environment used to create this project had **no internet access**, so no third-party
assets were downloaded from OpenGameArt, Kenney, itch.io, CraftPix, Freesound, Pixabay, Wikimedia or Poly Haven.
Per the brief's fallback rule, every visual and audio asset was **generated originally** by the scripts in `tools/`.
No Rayman or Crash Bandicoot files are used.

| Asset group | Files | Source / Author | License |
|---|---|---|---|
| Hero "Lumo" spritesheet (11 animations) + portrait | assets/characters/* | tools/gen_hero.py (original) | CC0 1.0 |
| 30 enemies (6 behaviours x 5 worlds) | assets/enemies/* | tools/gen_creatures.py (original) | CC0 1.0 |
| 5 bosses | assets/bosses/* | tools/gen_creatures.py (original) | CC0 1.0 |
| Tilesets, parallax backgrounds (4 layers x 5 worlds), fog, decorations | assets/tilesets, backgrounds, objects/deco_* | tools/gen_world.py (original) | CC0 1.0 |
| Collectibles, objects, projectiles, particles, world map, UI, logo, icon | assets/collectibles, objects, worldmap, ui | tools/gen_objects.py (original) | CC0 1.0 |
| 11 music tracks, 31 SFX, 5 ambience loops | assets/music, assets/sounds | tools/gen_audio.py (original synthesis) | CC0 1.0 |
| 35 stage layouts | assets/levels | tools/level_chunks.py + build_levels.py (original) | CC0 1.0 |
| Post-processing shader | assets/shaders/post.frag | original | CC0 1.0 |
| Noto Sans Regular/Bold, Noto Sans Display Bold | assets/fonts/* | Google LLC / Noto project | SIL Open Font License 1.1 (https://openfontlicense.org) |
| SFML 2.6.1 (runtime DLLs in the release ZIP) | - | Laurent Gomila & contributors | zlib/png |
| OpenAL Soft (openal32.dll shipped with SFML) | - | OpenAL Soft authors | LGPL 2.1 (dynamically linked) |

To swap in downloaded packs later, keep file names/frame layouts (see `*.anim` files) and add a row here.
