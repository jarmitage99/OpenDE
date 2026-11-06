# OpenDE

An open-source reimplementation of the Deus Ex engine and UnrealEd. It runs
the original game's UnrealScript bytecode from the shipped `.u` packages on a
new C++ engine built on SDL2 and OpenGL 3.3.

The original game data is required and is not included.

## Features

- Runs natively on Linux, Windows, macOS and in the browser
- Widescreen support with Hor+ field of view
- imgui-based editor
- Anisotropic filtering, trilinear filtering
- MSAA and FXAA
- Smooth lighting: bicubic filtering of lightmaps
- Detail textures visible at longer distances

## Requirements

Debian/Ubuntu with:

    $ sudo apt install build-essential cmake pkg-config libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxkbcommon-dev libgl-dev libegl-dev libasound2-dev libpulse-dev libwayland-dev wayland-protocols libdecor-0-dev

Every library dependency is vendored in `vendor/` and built with the project.

## Game data

Copy the contents of an installed Deus Ex (GOTY 1112fm) into `game-data/`, so
that `game-data/System`, `game-data/Maps`, `game-data/Textures`,
`game-data/Sounds` and `game-data/Music` exist. Another location can be given
at configure time with `-DDEUSEX_GAME_DATA=/path/to/deusex`.

The Deus Ex demo (version 1002f) works too, for example
`DEUSEX_GAME_DATA=demo-data build/bin/deusex`.

At runtime the game and the editor look for the data in this order: the
`DEUSEX_GAME_DATA` environment variable, a `game-data` directory next to the
executable (next to the AppImage file for AppImage builds), the Steam install
of Deus Ex (`steamapps/common/Deus Ex` in the default Steam directories and
their library folders), and finally the path given at configure time.

## Building

    $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    $ cmake --build build -j"$(nproc)"

Options:

- `-DSKIP_QA=ON` skips clang-format during the build.
- `-DSKIP_UT=ON` skips building the unit tests.

`scripts/check-debian.sh` builds the project in clean Debian 12 and Debian 13
containers with Docker.

### Windows

Windows builds are cross-compiled with MinGW:

    $ cmake -S . -B build-mingw -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake
    $ cmake --build build-mingw -j"$(nproc)"

## Browser builds

Requires Emscripten and WebGL 2. The game data given at configure time is
packaged into `index.data`, without the Windows binaries:

    $ emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DSKIP_UT=ON -DDEUSEX_GAME_DATA="$PWD/demo-data"
    $ cmake --build build-web --target deusex -j"$(nproc)"
    $ python3 -m http.server -d build-web/bin 8000

Then open `http://localhost:8000/`. The game data is LZ4 compressed
and cached in IndexedDB after the first download, together with settings
and saved games.

## Running

    $ build/bin/deusex

This starts at the title screen with the main menu. You can also specify a
map to be loaded `build/bin/deusex 01_NYC_UNATCOIsland`, or a path
to a `.dx` file.

Controls, settings and saves live in `~/.local/share/deusex` (`%APPDATA%\deusex`
on Windows): `DeusEx.ini` and `User.ini` are read from there when present
and otherwise from the game `System` directory, changed settings are
written there, and saved games are in `Save/`. The game data directory is
never written to. Key bindings are the original ones from `User.ini`, for
example keypad `+` for QuickSave and keypad `/` for QuickLoad.

## Mods: GMDX

Mods live in `mods/`, right next to `game-data`, one folder each. Your game
data never gets touched.

Download GMDX from ModDB, unpack it into its own mod folder:

    $ mkdir -p mods/GMDX && cd mods/GMDX
    $ 7z x -aou ../../GMDX_v9.0.3.exe

The first copy of each file is the English one. Delete the rest if you
dont want them:

    $ rm System/DeusEx_*.int System/*.est GMDXv9/System/gmdx_*.ini GMDXv9/System/*.est GMDXv9/System/DeusEx.rut
    $ rm -rf '$PLUGINSDIR' 'HDTP/$PLUGINSDIR' 'New Vision/Textures/$PLUGINSDIR'

HDTP is required. For New Vision, `-aos` keeps the `KZTextures.utx`:

    $ 7z x -oHDTP HDTP/HDTP.7z && rm HDTP/HDTP.7z
    $ 7z x -aos "-oNew Vision/Textures" "New Vision/Textures/NVTextures.7z" && rm "New Vision/Textures/NVTextures.7z"

You should now have `GMDXv9`, `HDTP`, `New Vision` and `System` inside
`mods/GMDX`. Now just launch the game:

    $ build/bin/deusex --mod GMDX

Settings and saves are in `~/.local/share/deusex/mods/GMDX`.

## Mods: The Nameless Mod

Similar setup:

    $ mkdir -p mods/TNM && cd mods/TNM
    $ 7z x ../../The_Nameless_Mod_Installer_2023-9-17_1132.exe

Check that `System` and `TNM2` exist in `mods/TNM`. Then:

    $ build/bin/deusex --mod TNM

## Editor

    $ build/bin/deused 01_NYC_UNATCOIsland

The editor is currently a work in progress. But it works!

## UnrealScript compiler

`ucc` exports and compiles UnrealScript, like the UCC that shipped with the
original Deus Ex SDK. Sources and compiled packages live in
`~/.local/share/deusex` and packages in `~/.local/share/deusex/System`
are loaded in place of the shipped ones with the same name.

To export the sources of a package:

    $ build/bin/ucc batchexport DeusEx.u class uc ~/.local/share/deusex/DeusEx/Classes

Compile:

    $ build/bin/ucc make

## Tests

    $ build/bin/unit-tests

Unit tests that need the game data are tagged `[data]`. You can run the ones
that dont need game data with:

    $ build/bin/unit-tests "~[data]"

## Thanks

This project would not be possible without the incredible work from these
legends:

- gildor
- dpJudas
- jmarshall
- AnthraX
- Marco888
