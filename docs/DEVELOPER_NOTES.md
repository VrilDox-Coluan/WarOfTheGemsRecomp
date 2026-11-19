# Developer Handover & Reverse-Engineering Notes
**Project:** Marvel Super Heroes: War of the Gems — Native Static Recompilation  
**Author Status:** Archived / Up for Grabs  
**Target:** SNES 65816 HiROM FastROM (16 Mbit, No SRAM)

---

## 1. Engine Architecture & Capcom Fiber Scheduler

*War of the Gems* (1996) runs on an evolved revision of the Capcom SNES engine first developed for *X-Men: Mutant Apocalypse* (1994). Understanding this engine is the single most important factor for anyone extending this recompilation.

### Cooperative Fiber/Task Scheduler
The game does not use a flat execution flow. Instead, it implements a cooperative fiber scheduler primarily located in Bank `$C0` (`$C0:0000`–`$C0:1FFF`):
- **Mechanism**: Tasks (player physics, enemy AI, particle systems, HUD animations) are assigned independent stack frames. Context switching is performed by swapping the stack pointer (`TSC; STA dp,X` and `LDA dp,X; TCS`).
- **AOT Trap**: When compiled directly into native C via `snesrecomp`, standard C call stacks cannot tolerate arbitrary stack pointer rewrites across function boundaries. Naive promotion of scheduler routines leads to stack frame corruption or infinite loops (`interp_cap`).
- **Solution**: The critical scheduler entry points and context-switchers must remain in the LLE interpreter fallback. They are declared in [`runner/wotg_interp_targets.txt`](../runner/wotg_interp_targets.txt).

### Inline Parameter Subroutines (`JSL` with inline data)
Several utility functions in Bank `$C5` read arguments stored *immediately after the `JSL` instruction in ROM*:
- **Signature**: `PLY; PLA; PHA; PHY` (`7A 68 48 5A`).
- **Examples**:
  - `0xC50B58` (in `wotg_interp_targets.txt`)
  - `0xC52985` (in `wotg_interp_targets.txt`)
  - `0xC52A2A`: Reads 6 inline bytes (`$0001,Y`..`$0006,Y`) after `JSL`. Must remain interpreted; if compiled to AOT, the native `RTL` will return into the middle of the inline ROM parameters, executing data as code (`BRK` at `$C5:5625`).
  - `0xC52AE4`: Shares identical inline parameter signature.

### VBlank Wait Loops
- `0xC50AB4`: Runs `while ($F0 != 0) JSR $0A96;`, where `$C5:0A96` issues a `WAI` (`0xCB`) opcode. In AOT mode, this loop must yield properly to the frame driver or remain in LLE fallback to avoid freezing.

### Open-Bus Anti-Piracy Check (`$C0:FF4E`)
- At boot, the game checks for open-bus behavior on unmapped SRAM space (`STA $6000,X` / `CMP $6000,X` with `DB=$30`).
- Ensure `snesrecomp/runner/src/cpu_state.c` handles `(bank & 0x7f) < 0x40 && addr >= 0x6000 && addr < 0x8000` as open-bus when `cart->ramSize == 0` for `CART_HIROM`. This prevents spurious `[off-rails-romptr]` crashes.

---

## 2. Bank Layout: Code vs. Data

Using instruction profiling (`REP`/`SEP` opcode density), we mapped the exact boundaries between executable 65816 logic and passive data tables:

| Bank | REP/SEP Count | Role & Contents |
| :--- | :--- | :--- |
| **`$C0`** | 407 | Task scheduler, input handler (`$C0:DC22`), text engine, memory initializers |
| **`$C1`** | 269 | Level state machines, camera update logic, input processing (`$C1:1AB9`) |
| **`$C2`** | 169 | Player physics, combo tables, damage calculation, sprite culling |
| **`$C3`** | 79  | SPC700/DSP APU audio communication driver |
| **`$C5`** | 320 | Decompression helpers, inline parameter unpackers, DMA queue handlers |
| **`$CE`** | 46  | Enemy object handlers & stage scripting |
| **`$D1`** | 36  | Boss fight logic (Thanos, Magus, Dr. Doom) |

*Note: Banks `$C4`, `$C6`–`$CD`, `$C8`–`$DF` consist almost entirely of graphics, compressed tilemaps, palette tables, and audio samples.*

---

## 3. Widescreen (16:9) Implementation Guide

The game canvas was natively 256×224 pixels. The reverse-engineered geometry is already mapped for anyone wishing to complete true 16:9 widescreen:

### Camera Registers in WRAM
- **Camera X**: `$7E:3F9B` (16-bit word, little-endian)
- **Camera Y**: `$7E:3F9F` (16-bit word, little-endian)

### Background Tilemaps
All three background layers allocate full 512×256 buffers:
- `BG1`: Tile base `$5000` (64×32 tiles / 512×256 px)
- `BG2`: Tile base `$5800` (64×32 tiles / 512×256 px)
- `BG3`: Tile base `$0800` (64×32 tiles / 512×256 px)

Because the background tilemaps are 512 pixels wide, horizontal screen scrolling already keeps extra tile columns in VRAM.

### Horizontal Culling & Clipping Routines (Action Item)
Capcom's sprite engine aggressively despawns and clips objects outside the 256px viewport:
- **`0xC29D90`**: Primary sprite bounding-box visibility test.
- **`0xC27A03`**: Secondary horizontal clipping test.
- **How to patch**: Modify these two subroutines in `config/` or `runner/` to widen the clipping window by `+85` pixels on each side (or check against the expanded viewport aspect ratio), allowing enemies and projectiles to render seamlessly on the edges of 16:9 screens.

---

## 4. In-Game Debug Menu & State Warping Architecture

The in-game menu is implemented in [`runner/wotg_menu.c`](../runner/wotg_menu.c) and [`runner/wotg_menu.h`](../runner/wotg_menu.h):
- **Activation**: Toggled with key **`M`** during active gameplay.
- **Stage Select**: Warps the game state to any level (`0x00` through `0x0E`):
  - `0x00`: Character Select / Asteroid Belt (Iron Man)
  - `0x01`: Alaska (Hulk)
  - `0x02`: Amazon (Spider-Man)
  - `0x03`: Boston (Wolverine)
  - `0x04`: Egypt (Captain America)
  - `0x05`–`0x0E`: Boss stages (Dr. Doom, Magus, Thanos, Space Station)
- **Warp Implementation**:
  When warping to a new stage or loading a state, `snes_beam_frame_driver_warp_pc` is called in `beam_frame_driver.c`. Crucially, you must also synchronize the interpreter bridge:
  ```c
  interp_bridge_set_lle_resume_pc(pc24);
  ```
  Failing to update `s_lle_resume_pc24` causes the interpreter to resume at the old program counter, triggering a `BRK` assertion.

---

## 5. ROM Hack & Modding Layout

For developers interested in adding modding or patch support, the following community patches were analyzed during reverse-engineering and serve as primary reference architectures:

- **Redux v3.0 Patch** (by **BillyTime! Games**):
  - Hooks gameplay logic, alternate character forms, and special moves inside Banks `$C0` and `$C1`.
  - Places expanded code and routines into Bank `$DF` (free space in the original HiROM).
  - Uses `$C5:FDB1` / `$C5:FE00` hooks to implement automatic SRAM saving for passwords.
- **Palette Patch** (by **Kertra**):
  - Modifies pure color lookup tables in Banks `$C5` through `$CF` (Hulk purple shorts, MCU Infinity Stone colors).
  - Can be hooked dynamically via `snes_beam_frame_driver` or patched into ROM palettes at startup.
- **Translations (Spanish 1.1 by Max1323)**:
  - Dialogue strings and 8×8 font tables reside in `$C0` and `$C5`.

---

## 6. Community Credits & Acknowledgments

Special recognition to the ROM hackers and researchers whose work helped map the Capcom SNES engine:
- **BillyTime! Games**: Author of *War of the Gems Redux*.
- **Kertra**: Author of the *War of The Gems Palette Patch*.
- **Max1323**: Author of the *Spanish Translation 1.1*.
- **spinda**: Author of `snesrecomp`.

---

## 6. How to Continue Development (Quick Checklist)

1. **Clone & Build**:
   ```bash
   git clone https://github.com/VrilDox-Coluan/WarOfTheGemsRecomp.git
   cd WarOfTheGemsRecomp
   .\build.ps1   # or sh build.sh on Linux
   ```
2. **Test Gameplay**:
   Provide your legally dumped ROM `Marvel Super Heroes - War of the Gems (U).smc` and run `.\build\wotg_desktop.exe`.
3. **Inspect Logs**:
   Use `wotg_headless.exe --rom "..." --frames 300` to run deterministic parity regressions.
4. **Next Big Feature**:
   Implement expanded scanline rendering in `beam_frame_driver.c` and patch `0xC29D90` / `0xC27A03` for true 16:9 widescreen!
