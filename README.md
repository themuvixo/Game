# Lumo and the Shattered Dream
Original 2D fantasy platformer (C++17, SFML 2.6.1). 5 worlds x 7 stages (5 normal incl. a challenge stage, 1 secret, 1 boss) = 35 stages,
an explorable island world map, 30 enemy types, 5 multi-phase bosses, collectibles, achievements, gallery and versioned saves.

## Controls (rebindable in Options > Keyboard Controls)
Arrows/WASD move | Space/Z jump (hold while falling = helicopter glide; double jump after beating World 1's boss or finding the orb in 1-5)
X/J magic fist (hold to charge) | L-Shift walk | Esc/P pause | Enter confirm | Mouse works in menus and on the map.

## Build (Windows, MSVC v143, x64)
CI builds it: push to GitHub and the `Build Windows Portable` workflow produces artifact **windows-portable** containing
`AdventurePlatformer-Windows.zip` (exe + SFML DLLs + VC++ runtime DLLs + assets + README.txt). Manual:
`cmake -S . -B build -A x64 -DSFML_DIR=C:/SFML-2.6.1/lib/cmake/SFML && cmake --build build --config Release`

## Saves
`%APPDATA%\LumoShatteredDream\` (save.dat, save.dat.bak, settings.cfg, game.log). Writes go to a temp file, are re-validated
(checksum), then replace the old file; a .bak is kept; corrupted files are preserved as `save.corrupt.*`, never deleted.

## Tools
`tools/*.py` regenerate every asset (Python 3, numpy, Pillow, scipy, ffmpeg). `tools/build_levels.py` assembles the 35 stages and
statically validates that the exit, cages, gems, keys and relics are reachable within the player's jump envelope.

## Compatibility
Targets Windows 10/11 x64. Windows 7 is NOT claimed: the MSVC v143 runtime and SFML 2.6.1 build were not verified there.
