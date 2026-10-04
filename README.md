# Top Tennis — compatibility engine

🇬🇧 English · 🇮🇹 [Italiano](README.it.md)

**Top Tennis** is an unofficial C/SDL2 compatibility engine for Linux, **PS Vita** and **PSP**,
using files supplied from your own copy of the DOS game Top Tennis (The Game Factory, 1997; John Dolph).
The project was developed through reverse engineering. Original data is read locally from your own files.
Version 1.4 restores the version 1.1 control, shot and physics behaviour. Version 1.3 had different controller and shot behaviour.

## What you need (important)

You need a copy of the original game that you have the right to use. Original executables, graphics, sounds,
music and extracted data tables are **not included** in the current source or release packages.

Choose either setup:

- **One DAT file on the console (recommended):** on your PC run the included Python script against your original
  `TENNIS.EXE` and `TENNIS.DAT`. Copy the new DAT to your console; the EXE is no longer needed there.
- **Original files directly:** put your original `TENNIS.EXE` and `TENNIS.DAT` together in the game's data folder.
  The engine reads the EXE's data, without executing its code.

Download `TopTennis-Data-Tools.zip` from the [latest release](https://github.com/figarocool/Top-Tennis-game-Factory/releases/latest),
extract it and open a terminal in the `TopTennis` folder. Install Python 3 on your PC, then run:

```sh
python3 tools/prepare_data.py "/path/to/your/original/game" --output "prepared/TENNIS.DAT"
```

On Windows use `py -3` instead of `python3`. The input folder must contain **both original `TENNIS.EXE` and
`TENNIS.DAT`**. The script creates one new `prepared/TENNIS.DAT` and preserves the originals;
it refuses to overwrite an existing output. Copy the generated file, without the EXE, to your platform's folder:

| Platform | Prepared DAT destination |
|---|---|
| Native Vita | `ux0:/data/TopTennis/TENNIS.DAT` |
| PSP on Vita (Adrenaline) | `ux0:/pspemu/PSP/GAME/TopTennis/TENNIS.DAT`, next to `EBOOT.PBP` |
| PSP | `ms0:/PSP/GAME/TopTennis/TENNIS.DAT`, next to `EBOOT.PBP` |
| Linux | `prepared/TENNIS.DAT`; run `./toptennis prepared` |

The prepared DAT contains data from your copy: keep it private and out of Git and public packages.
The readers support the verified 1997 full-version layout; other versions are rejected with an error.
`TENNIS.OPT` is optional. **The original DAT alone is insufficient**: prepare the new DAT or copy EXE and DAT together.
See the [step-by-step guide, Windows examples and troubleshooting](DATA_SETUP.md).

## What is in the game

- Friendly matches, singles or doubles for up to 4 players (2 keyboards + 2 joysticks).
- **Tournaments**: 64-player knockout draws in 12 cities, and a full **season** with a world ranking.
- Training: serve practice and a ball machine with 12 different shots.
- Replay of the last point (F3), savable replays, Hall of Fame, saving and loading of tournaments and seasons.
- 4 court types (clay, hard, grass, indoor), 4 CPU levels, best-of-3 or best-of-5 matches.
- Sound effects and FM music (an OPL2 synthesizer written for this project).
- Menus and dialogs using data from your copy, with adjustable game speed.

## Building and running (Linux)

    sudo apt install build-essential libsdl2-dev
    make
    ./toptennis prepared         # prepared/TENNIS.DAT is enough; no EXE needed here
    ./toptennis game_folder      # alternatively: original TENNIS.EXE + TENNIS.DAT

Options, saves, replays and the Hall of Fame are written to the same folder. F11 = fullscreen, F12 = screen format
4:3 / 16:9 (on the consoles it is chosen in the OPTIONS menu, default 16:9). Without an argument the engine uses
`./orig`. Missing files, unsupported versions and invalid prepared DATs are explained on screen.
During a match: ESC quits, F3 replay, F5 pause, F10 "boss" screen.

## Network play (LAN / Wi-Fi)

From the main menu choose **NETWORK**: one player picks **HOST** (plays the near side and sets the match options from the
PLAY menu: court, number of sets, speed), the other picks **JOIN** (plays the far side). It works between PC, PS Vita and
PSP in any combination, as long as they are on the same local network (same router / Wi-Fi): the guest finds open games
by itself, or you can type the IP address the host shows on screen. The PSP uses its first saved Wi-Fi profile.

How it works: UDP on port 5757; both machines simulate the same match and swap only the buttons pressed every frame
(2 frames of delay to hide latency). A periodic hash of the game state warns you if the two simulations diverge. ESC
ends the match for both players. On a PC you may need to open UDP port 5757 in the firewall. For now it is friendly
one-on-one matches only.

Both players must use the same engine build and compatible original data. Version 1.4 uses network protocol 3; it is incompatible with version 1.3.

## PS Vita

Download `TopTennis.vpk` from the [Releases](https://github.com/figarocool/Top-Tennis-game-Factory/releases) (or build it, see below).

1. Install the VPK with VitaShell (HENkaku's "unsafe homebrew" mode must be enabled).
2. Copy your **prepared** `TENNIS.DAT` to `ux0:data/TopTennis/`. Alternatively copy both original files there.
3. Start "Top Tennis" from the LiveArea.

Controls: d-pad or stick = move, cross = fire / confirm, circle = ESC, triangle = replay (F3) and shows / hides the
on-screen keyboard, square = S (erases in name fields, "no" in yes/no questions), Start = pause, L/R = Y/N. In the menus
the left stick moves a pointer (cross = click) and you can tap the screen; a touch keyboard appears for names. If
something goes wrong the game writes `ux0:data/TopTennis/log.txt`.

To build it you need [VitaSDK](https://vitasdk.org):

    export VITASDK=/usr/local/vitasdk
    cd vita && mkdir build && cd build && cmake .. && make      # produces TopTennis.vpk

## PSP

Download `TopTennis-PSP.zip` from the [Releases](https://github.com/figarocool/Top-Tennis-game-Factory/releases) and extract it, or build with PSPDEV (project in `psp/`):

    export PSPDEV=/usr/local/pspdev && export PATH=$PSPDEV/bin:$PATH
    cd psp && mkdir build && cd build && psp-cmake .. && make      # produces EBOOT.PBP

Put `EBOOT.PBP` and your **prepared** `TENNIS.DAT` in the same folder, for example `ms0:/PSP/GAME/TopTennis/` (on a Vita, in the PSP
emulator: `ux0:pspemu/PSP/GAME/TopTennis/`). Controls are the same as on the Vita; there is no touch screen, so in the
menus the analog stick moves the pointer (cross = click) and names are typed with the on-screen keyboard. The PSP build
draws straight into the frame buffer. Version 1.4 has been cross-compiled for Vita and PSP and tested on Linux. A physical-console test of this release has not been performed.

Alternatively use your original `TENNIS.EXE` and `TENNIS.DAT` together. Console builds use artwork made for this project
by default (`TT_PUBLIC=ON`). Private artwork is an explicit local build option, not part of release packages.

## How it is organised

| Folder | Contents |
|---|---|
| `src/` | the game: ball physics (`ball.c`), shots (`shot.c`), AI and controls (`ctrl.c`), match (`match.c`), scoring, menus and dialogs, tournament and season (`tournament.c`), replay, Hall of Fame, audio and the OPL2 synthesizer, network play (`net.c`, `netplay.c`) |
| `vita/` | CMake project for VitaSDK |
| `psp/` | CMake project for PSPDEV, with the public XMB artwork in `art_public/` |
| `tools/` | local DAT preparation, `TENNIS.DAT` extractor, sprite decoder, release audit and DOSBox comparison script |
| `re/` | technical tools for studying file formats |

Environment variables for testing: `TT_SHOT`/`TT_SHOT_FRAMES` (screenshots), `TT_INPUT` (scripted keys), `TT_MENU`,
`TT_CTRL` (force the controls, 5 = CPU), `TT_VJOY` (virtual joystick), `TT_TURBO` (faster clock), `TT_TRACE`, `TT_DEBUG`.
On the consoles, where there are no environment variables, you can put `tt_input.txt` (key script) and `tt_env.txt`
(`NAME=value` lines) in the game folder.

## Known differences from the original

- Development controllers, shot selection and ball physics target version 1.1 behaviour. Automated comparisons
  cover individual states; complete equivalence with every DOS game situation has not been established.
- The music uses a new OPL2 synthesizer: notes and timing are the original ones, the timbre is approximate.
- The joystick uses axes already calibrated by SDL: the calibration screens exist but store nothing.
- The "LOADING" bar has a fixed duration (in the original it depended on file reading).
- Loading a saved tournament or season restarts from the saved round, as the original did.

## Licensing and credits

Unofficial project, not affiliated with or approved by the original game's rightsholders. Use a copy you have the
right to use; original files and prepared DATs are not included in distributions. The project licence does not
grant rights in the original game content. See [LICENSE.md](LICENSE.md) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for licensing terms and component notices.

## Checks

`make test` runs parser safety and replacement-controller tests without original data. For local compatibility tests,
run `TT_ORIGINAL_DIR=/path/to/your/game make test`. Original files and generated DATs are never test fixtures in this repo.
