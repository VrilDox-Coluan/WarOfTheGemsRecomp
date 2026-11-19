# Marvel Super Heroes: War of the Gems — Native Static Recompilation

[![Status: Up For Grabs](https://img.shields.io/badge/status-up_for_grabs-brightgreen.svg)](https://github.com/VrilDox-Coluan/WarOfTheGemsRecomp)
[![Engine: snesrecomp](https://img.shields.io/badge/engine-snesrecomp-blue.svg)](https://github.com/VrilDox-Coluan/WarOfTheGemsRecomp)
[![Stack: C11 | SDL3](https://img.shields.io/badge/stack-C11%20%7C%20SDL3-orange.svg)](https://github.com/VrilDox-Coluan/WarOfTheGemsRecomp)
[![Sister Project: X-Men Recomp](https://img.shields.io/badge/sister_project-X--Men%20Recomp-purple.svg)](https://github.com/VrilDox-Coluan/XMenMutantApocalypseRecomp)

> [!WARNING]
> ### ⚠️ Legal Disclaimer: Bring Your Own Dump
> **No game data is in this repository. No ROM, no assets, no proprietary data. Bring your own dump** (*Marvel Super Heroes - War of the Gems (U).smc*, 2 MiB headerless, SHA-256 `49dd77b310b476c875633335243553be59ecfb0bffae62e46f2e53ff05c20fcd`).
> The original game assets (graphics, sound, palettes, music sequences) are loaded directly from your own legally obtained ROM file at runtime. This repository contains only reverse-engineered runner code, menu hooks, and static recompilation tooling.

> [!NOTE]
> ### 🛑 Project Status: Archived / Up for Grabs
> **This project is on hold and will NOT be actively maintained or updated by the author.**
> Please note that the author has ceased active development on **both** Capcom SNES recompilation projects:
> - **Marvel Super Heroes: War of the Gems Recomp** (this repository)
> - [X-Men: Mutant Apocalypse Recomp](https://github.com/VrilDox-Coluan/XMenMutantApocalypseRecomp) (sister project sharing the Capcom fiber-scheduler engine)
>
> Both repositories are released as public research foundations for the emulation, decompilation, and speedrunning communities. They are fully functional up through active gameplay at 60 FPS and are **up for grabs** for anyone who wants to fork, continue development, implement true 16:9 widescreen rendering, or port ROM hacks.

A native PC port of Capcom's 1996 Super Nintendo classic **Marvel Super Heroes in War of the Gems**, created via static binary recompilation (`snesrecomp`).

This project translates the original SNES 65816 machine code into native C11, running alongside an SDL3-based hardware runner with cycle-accurate SNES PPU/APU emulation.

---

## Estimated Progress & Status

| Subsystem | Progress | Status | Notes |
| :--- | :--- | :--- | :--- |
| **Boot & Intro Parity** | `██████████` 100% | Complete | Parity verified matching LLE reference on boot (frame SHA-256 match at 300 frames) |
| **Audio (SPC700 / DSP)** | `██████████` 100% | Complete | Music & sound effects running natively via SPC700/DSP core |
| **In-Game Debug Menu & Launcher** | `██████████` 100% | Complete | Debug menu (`M`), Stage select (`00..0E`), real-time pause & frame step (`K`) |
| **Input & Savestates** | `██████████` 100% | Complete | Full gamepad/keyboard mapping, 10 save/load slots, visual thumbnail browser (`F7`) |
| **Character Physics & Combat** | `█████████░` 90% | Playable | Iron Man, Hulk, Spider-Man, Wolverine, Captain America playable at 60 FPS |
| **AOT Code Coverage** | `████████░░` 80% | High | 429 roots mapped, 388 AOT compiled, 190 dynamic targets safe in LLE fallback |
| **16:9 Widescreen Groundwork** | `██████░░░░` 60% | Mapped | Camera registers (`$7E:3F9B`/`$7E:3F9F`) & horizontal clipping (`0xC29D90`, `0xC27A03`) mapped |
| **ROM Hack & Modding Support** | `█████░░░░░` 50% | Analyzed | Memory layout analyzed for Redux v3.0, Color Palettes, and Spanish translation |

---

## 📖 Developer Handover & Technical Notes

> **Notice for Contributors:** If you are interested in continuing this project, comprehensive reverse-engineering findings, hardware memory addresses, Capcom fiber-scheduler traps, and Widescreen 16:9 implementation details have been compiled in **[docs/DEVELOPER_NOTES.md](docs/DEVELOPER_NOTES.md)**.

---

## Current Status (Work in Progress)

- **Native Execution**: Runs at a rock-solid 60 FPS natively on Windows and Linux without an emulator wrapper.
- **Parity**: Boot and intro sequence match SNES LLE hardware emulation.
- **AOT Coverage**:
  - **429** discovered code roots mapped.
  - **388** routines compiled Ahead-Of-Time (AOT) to native C.
  - **190** dynamic indirect jump targets routed safely through the LLE interpreter fallback (`runner/wotg_interp_targets.txt`) to protect Capcom's cooperative fiber scheduler and stack frames.
- **In-Game Debug & Launcher Menu**:
  - Hotkey: **`M`**
  - **Stage Select**: Warp directly to any level (`00` through `0E`), including *Iron Man (Space/Asteroid Belt)*, *Hulk (Alaska)*, *Spider-Man (Amazon)*, *Wolverine (Boston)*, *Captain America (Egypt)*, *Dr. Doom*, *Magus*, and *Thanos*.
  - **Savestates**: Real-time state saving and loading across 10 slots.
  - **Savestate Browser**: Visual thumbnail slot browser via **`F7`**.
  - **Session Control**: Pause (**`P`**), Frame Advance (**`K`**), and Reset (**`F5`**).

---

## Known Issues & Research Notes

### 1. Intro Screen Fade Timing Divergence
When running with full AOT enabled, screen transition fades (between the Capcom logo, Marvel intro, and title screen) run slightly faster than the pure LLE reference (~98 frame difference over 300 frames).
- **Culprit subroutines**: `$C0:B4DE` and `$C5:0000` (Capcom cooperative task/fiber scheduler).
- **Behavior**: The scheduler yields on VBlank timers; differences in unthrottled branch cycle counts can advance the fade counter earlier.
- **Status**: Non-fatal during gameplay; documented for anyone refining the cycle-budget timers in `beam_frame_driver.c`.

### 2. Audio Subsystem
Audio playback uses the native SPC700/DSP emulation core provided by the `snesrecomp` runner. All music and sound effects play accurately.

---

## Roadmap & Areas for Contribution

We welcome PRs and research from the community! Here are the next major milestones:

### 🌟 True 16:9 Widescreen Implementation
The reverse-engineered game geometry is already mapped:
- **Camera Registers**:
  - Camera X: `$7E:3F9B` (2 bytes, 16-bit word)
  - Camera Y: `$7E:3F9F` (2 bytes, 16-bit word)
- **Background Tilemaps**:
  - `BG1`: Tile base `$5000` (64x32 tiles / 512x256 px)
  - `BG2`: Tile base `$5800` (64x32 tiles / 512x256 px)
  - `BG3`: Tile base `$0800` (64x32 tiles / 512x256 px)
- **Object / Sprite Horizontal Culling Routines**:
  - Screen boundary clipping logic resides at `0xC29D90` and `0xC27A03`.
- **Next Step**: Expand the SDL3 viewport aspect ratio to 16:9 (426x240 or 384x216), widen the PPU scanline rasterizer buffer, and patch the horizontal clipping limits so enemies and objects render outside the original 256px boundary.

### 🎮 Enhancements & Modding
- **IPS Patch Integration**:
  - *Redux v3.0*: Uses bank `$DF` free space and patches into `$C0` and `$C1`.
  - *Palette / Color Fixes*: Modifies tables located in banks `$C5` through `$CF`.
  - *Translations (Spanish, Japanese)*: Modifies dialogue pointers and 8x8 font tables in `$C0` and `$C5`.
- **Controls & Input**:
  - Native SDL3 Gamepad support with remappable button layouts.
  - High-DPI UI scaling and fullscreen toggle (**`Alt+Enter`** / **`F11`**).

---

## Prerequisites

### Windows
- [CMake](https://cmake.org/download/) (v3.20 or newer)
- [Ninja](https://github.com/ninja-build/ninja/releases)
- GCC / MinGW-w64 (GCC 14.2+ recommended) or Clang
  *(e.g., via WinLibs or MSYS2)*

### Linux (Ubuntu / Debian / Fedora / Arch)
- CMake, Ninja, GCC / Clang
- SDL3 development headers and libraries (or allow CMake to fetch them automatically)
```bash
sudo apt-get install build-essential cmake ninja-build libsdl3-dev
```

---

## Building

### Quick Build (Windows PowerShell)
```powershell
.\build.ps1
```

### Quick Build (Linux / macOS)
```bash
sh build.sh
```

### Manual CMake Build
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The resulting binaries will be placed in the `build/` directory:
- `build/wotg_desktop.exe` (Interactive desktop application with SDL3 window)
- `build/wotg_headless.exe` (Headless regression tester for parity benchmarking)

---

## Running the Game

### Requirements
You must supply a legally obtained ROM of **Marvel Super Heroes: War of the Gems (USA)**:
- **File**: `Marvel Super Heroes - War of the Gems (U).smc`
- **Format**: SNES Headerless (2,097,152 bytes)
- **SHA-256**: `49dd77b310b476c875633335243553be59ecfb0bffa62e46f2e53ff05c20fcd`

> **Note**: The ROM file is **NEVER** distributed with this project. The runner loads the ROM at runtime solely to read original graphics, audio samples, and level tilemaps.

### Launching
Run the desktop executable passing your ROM path:
```bash
# Windows
.\build\wotg_desktop.exe "path\to\Marvel Super Heroes - War of the Gems (U).smc"

# Linux
./build/wotg_desktop "path/to/Marvel Super Heroes - War of the Gems (U).smc"
```

### Controls (Default Keyboard Mapping)

| SNES Button | PC Keyboard |
| :--- | :--- |
| **D-Pad Up / Down / Left / Right** | Arrow Keys / `W`, `S`, `A`, `D` |
| **B (Jump)** | `Z` / `J` |
| **Y (Attack)** | `X` / `K` |
| **A (Dash / Special)** | `C` / `L` |
| **X (Gem Power)** | `V` / `I` |
| **L (Select Gem Left)** | `A` / `U` |
| **R (Select Gem Right)** | `S` / `O` |
| **Start** | `Enter` |
| **Select** | `Right Shift` |

### In-Game Menu & Debug Hotkeys

| Hotkey | Action |
| :--- | :--- |
| **`M`** | Toggle In-Game Menu (Stage select, savestates, pause) |
| **`F1`** | Quick Save (Slot 1) |
| **`F3`** | Quick Load (Slot 1) |
| **`F7`** | Savestate Gallery Browser |
| **`F5`** | Reset Game |
| **`P`** | Pause / Unpause |
| **`K`** | Step 1 Frame (when paused) |

---

## Project Structure

```text
WarOfTheGemsRecomp/
├── config/              # snesrecomp configurations and function signatures
├── generated/           # Recompiled C sources from the original 65816 code
├── runner/              # Custom game runner, menu hooks, and warp routines
│   ├── wotg_menu.c      # In-game debug and stage select UI
│   ├── wotg_menu.h
│   ├── wotg_game.c      # Game lifecycle callbacks & environment init
│   └── wotg_interp_targets.txt # Critical routines delegated to interpreter
├── snesrecomp/          # Vendored snesrecomp runtime, PPU/APU driver, and framework
├── tools/               # Scripts for converting movie dumps (.smv) to test tapes
├── CMakeLists.txt       # Unified CMake build configuration
├── build.ps1            # Windows one-click build script
├── build.sh             # Linux one-click build script
└── README.md
```

---

## Legal & Disclaimers

1. This project is a reverse engineering and static recompilation experiment developed for preservation, educational, and interoperability purposes.
2. No proprietary Nintendo or Capcom code, ROM binaries, copyrighted art, or sound assets are included in this repository.
3. Users must own and provide their own legally acquired copy of the original Super Nintendo cartridge.
4. *Marvel Super Heroes*, *War of the Gems*, and associated characters are trademarks of Marvel Entertainment and/or Capcom Co., Ltd. This project is not affiliated with, endorsed by, or sponsored by Marvel, Capcom, or Nintendo.

---

## License

The runner and custom reimplementation files are licensed under the MIT / GPL-3.0 License. The `snesrecomp` framework is subject to its respective upstream license (see `snesrecomp/LICENSE` and `snesrecomp/THIRD_PARTY_ATTRIBUTION.md`).
