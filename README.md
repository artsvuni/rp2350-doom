# DOOM on the RP2350-Touch-AMOLED-1.8

## Goal

Create an enjoyable, self-contained handheld version of DOOM on the tiny
[Waveshare RP2350-Touch-AMOLED-1.8](https://www.waveshare.com/rp2350-touch-amoled-1.8.htm)
using only the board's touchscreen and PWR button—no keyboard,
external controller, or display. The ambition is not just to prove the port
runs, but to make combat controllable and the presentation smooth enough that
finishing the game on this device feels realistic.

This project adapts Graham Sanderson's
[RP2040/RP2350 DOOM port](https://github.com/kilograham/rp2040-doom) to the
board's 368×448 SH8601 AMOLED, FT3168 touch controller, AXP2101 power
management IC, and ES8311 audio codec. The result boots, renders, loads E1M1,
and is playable on the physical device.

> **Current status (18 August 2026):** the effects-only build is verified on
> hardware with working touch controls, menu navigation, combat, and sound
> effects. Full-width 448×280 presentation sustains 35.2 FPS over a measured
> one-minute combat run with zero display-DMA timeouts and no additional static
> presentation memory. Music also works, but is disabled by default because the
> lightweight synthesized timbre is not a good fit for the small speaker. The
> latest F11 control build uses pointing-finger move/turn, touchscreen
> double-tap Use, and short bottom-corner strafe bursts. Tilt locomotion and
> continuous tilt strafing were tested and rejected as too easy to trigger
> unintentionally.

The staged product and engineering plan is in
[`docs/ROADMAP.md`](docs/ROADMAP.md).

## Display geometry

The physical AMOLED is 368×448 in portrait, or **448×368** in the landscape
orientation used by Doom. The current game image is **448×280**: it fills every
horizontal pixel and preserves the raw 320×200 image's 16:10 shape, leaving
44-pixel black bands above and below.

| Candidate | Top/bottom bands | Output pixels vs current | Trade-off |
|---|---:|---:|---|
| 448×280 current | 44px each | baseline | Measured 35.2 FPS; raw 16:10 image |
| 448×336 | 16px each | +20.0% | Traditional 4:3 correction; best next experiment |
| 448×368 | none | +31.4% | Fills panel but needs stretch, crop, or renderer/UI redesign |

The 448×336 candidate is more interesting than blindly stretching to the full
panel: it uses 91% of the panel height and matches the traditional 4:3 display
shape of 320×200-era Doom. Its performance, audio pacing, and gameplay feel
must be measured on hardware before it replaces the locked 448×280 path. The
remaining bands could alternatively hold quiet port UI such as battery state.

## What works

- DOOM Shareware boots from flash and loads the first episode.
- Full-width 448×280 gameplay preserves Doom's 16:10 image in landscape.
- One pointing finger controls proportional forward/back movement and turning
  from a floating anchor anywhere on the screen.
- Bottom-corner double taps produce short left/right strafe bursts; double-tap
  elsewhere activates Use/Open.
- The PWR button fires in-game and handles selection/back in menus.
- Eight-channel sound effects play through the onboard ES8311 codec and
  speaker using non-blocking DMA/IRQ audio.
- An optional nine-voice fixed-memory MUSX music synthesizer plays the WAD's
  real music data.
- Persistent out-of-memory diagnostics survive a watchdog reboot, which makes
  hardware-only failures easier to distinguish from deadlocks.
- A separate calibration firmware remains available for testing the display,
  touch controller, and physical-button events without starting the game.

## Controls

| Input | In game | In menus |
|---|---|---|
| Touch, drag up/down, and hold | Proportional forward/back | Move selection up/down |
| Touch, drag left/right, and hold | Proportional turn left/right | Move selection left/right |
| Double-tap bottom-left/right corner | Short strafe burst left/right | — |
| Double-tap elsewhere | Use/open | — |
| PWR tap | Fire once | Select/confirm |
| PWR double press | — | Back |
| PWR long press | Board-managed power action | Board-managed power action |
| BOOT | Reserved for entering BOOTSEL while powering/resetting | — |

The in-game gesture starts wherever the finger lands and uses both axes at
once, so the player can move while turning. Both axes use a one-pixel jitter
guard. Turning follows a compact quadratic 48-to-960 response over 112 pixels.
Forward/back follows a gentler quadratic 4-to-50 response over 140 pixels, so
the middle of the gesture remains near ordinary walking and full run requires a
deliberate far reach. Lifting the finger stops move/turn immediately. The
original fixed touch-zone model remains behind a compile-time selector.

The current corner dodge is intentionally modest rather than a finished
solution. Weapon selection and Escape/menu entry are the main missing control
decisions. A future experiment should compare top-corner previous/next-weapon
gestures with other mappings before adding more overlapping touch vocabulary.

Escape/menu entry is not currently mapped. Runtime BOOT polling was tested and
then removed: this board's BOOT button shares the external-flash chip-select
signal, and both BOOT-enabled Doom builds behaved abnormally while the
otherwise identical pre-BOOT firmware remained stable. BOOT is therefore
reserved exclusively for entering the ROM loader. A future PWR or touch
gesture will provide Escape without disturbing flash access.

## Build and flash

### Requirements

- Waveshare RP2350-Touch-AMOLED-1.8
- Raspberry Pi Pico SDK and its ARM GNU toolchain
- CMake 3.13 or newer
- `picotool` for initially loading the converted game data
- A legally obtained DOOM WAD; the repository does not distribute game data

Set `PICO_SDK_PATH` to your Pico SDK checkout, then build the game target:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S firmware -B firmware/build -DCMAKE_BUILD_TYPE=Release
cmake --build firmware/build --target doom -j4
```

The resulting game firmware is `firmware/build/doom.uf2`. The default build
includes sound effects but no music.

The current full-width F11 candidate is built explicitly so the conservative
320×200 fallback remains the CMake default:

```sh
cmake -S firmware -B firmware/build-controls-448 \
  -DCMAKE_BUILD_TYPE=Release \
  -DDOOM_DISPLAY_WIDTH=448 \
  -DDOOM_PANEL_CHUNK_ROWS=20 \
  -DDOOM_ASYNC_AMOLED=ON \
  -DDOOM_HYBRID_CONTROLS=ON \
  -DDOOM_ROLL_STRAFE=OFF
cmake --build firmware/build-controls-448 --target doom -j4
```

### Optional music build

The experimental fixed-memory music backend can be enabled without changing
source code:

```sh
cmake -S firmware -B firmware/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DDOOM_ENABLE_MUSIC=ON
cmake --build firmware/build --target doom -j4
```

Return to the device default with `-DDOOM_ENABLE_MUSIC=OFF`.

### Prepare the WAD data

The firmware and game data are flashed separately. The build expects a
super-tiny `IWHX` image at flash address `0x10200000`; this keeps the WAD
memory-mapped and out of scarce SRAM.

Build `whd_gen` from the included `firmware/engine/whd_gen/` sources or the
[upstream port](https://github.com/kilograham/rp2040-doom), then convert your
WAD in a release build for the best sound-effect encoding quality:

```sh
whd_gen /path/to/DOOM1.WAD build/doom1.whx
```

With the board in BOOTSEL mode, load the converted data once:

```sh
picotool load -v -t bin build/doom1.whx -o 0x10200000
```

The current configuration targets the shareware/single-episode data layout.
Using Ultimate DOOM, DOOM II, or another multi-episode WAD requires revisiting
the `WHD_SUPER_TINY` and `DEMO1_ONLY` build choices together.

### Flash the firmware

If the running build still exposes its Pico SDK USB reset interface, picotool
can reboot it into ROM BOOTSEL, verify the write, and return to Doom without a
physical BOOT press:

```sh
picotool load -v firmware/build/doom.uf2 \
  -f --ser 05BCF1C1AA06AA58
```

Use the serial number of the intended board rather than copying this one for a
different device. This UF2 updates the low firmware range only; WHX data at
`0x10200000` remains untouched. The reusable fast, high-assurance, backup, and
recovery procedures are in the sibling hardware knowledge base's
[`development-workflows.md`](../rp2350-touch-amoled-1.8-knowledge-base/development-workflows.md).

If automatic reset is unavailable, use the manual fallback:

Put the board in BOOTSEL mode and copy `firmware/build/doom.uf2` to the
mounted `RP2350` drive. On macOS, for example:

```sh
cp -X firmware/build/doom.uf2 /Volumes/RP2350/doom.uf2
```

The board reboots automatically. During normal development only the UF2 needs
to be copied again; the WHX data at `0x10200000` remains in flash.

The other CMake target, `doom_firmware`, produces
`firmware/build/doom_firmware.uf2`. That is the hardware-control calibration
program, not the game.

## How it works

| Area | Implementation |
|---|---|
| Engine | Vendored `DOOM_TINY` RP2040/RP2350 port derived from Chocolate DOOM |
| CPU model | Game/tic work on core 0; rendering/presentation work on core 1 |
| Display | SH8601 QSPI output with two asynchronous 20-row software-transpose tiles; CPU packing overlaps panel DMA |
| Input | FT3168 coherent two-axis touch, double-tap actions, and AXP2101 PWR events; motion and runtime BOOT access disabled in F11 |
| Audio | ES8311 + PIO I2S with two static 512-frame DMA buffers and IRQ refill |
| Game data | WAD preprocessed to compressed WHX and read directly from XIP flash at `0x10200000` |
| Failure diagnostics | On-screen boot checkpoints and watchdog-scratch OOM reports |

The SH8601 cannot exchange display axes in hardware, so landscape presentation
is performed as a bounded tiled transpose rather than with a full rotated
framebuffer. The 448×280 path uses two 20-row buffers: DMA reads one while
core 1 packs the other. Repeated vertical scale rows are packed together, which
keeps the full-width path above Doom's 35 Hz simulation rate without a full
scaled framebuffer.

The F11 full-width effects-only build links with `__end__=0x2004930c`,
leaving 224,500 bytes for DOOM's short-pointer zone up to `0x20080000`. The
zone's exact address matters: upstream compresses pointers into a fixed 256 KiB
window, so ordinary heap allocation cannot be substituted casually.

## Project history

This port was brought up on real hardware through repeated build/flash/test
cycles. The important milestones were:

1. **Control calibration** — confirmed BOOT and AXP2101 PWR press patterns,
   calibrated touch rotation, and tuned the original asymmetric touch zones.
2. **Engine adaptation** — vendored the RP2040/RP2350 DOOM engine and replaced
   its VGA/I2S assumptions with this board's AMOLED, touch, and codec drivers.
3. **First rendered game** — converted the WAD to flash-mapped WHX, fixed board
   clock and QSPI initialization problems, and displayed the real title/menu.
4. **Memory debugging** — found a stray `calloc()` overwriting DOOM's manually
   claimed zone, then recovered enough SRAM for E1M1 by shrinking diagnostic
   and display buffers.
5. **Playable controls and video** — wired touch/PWR events into DOOM, replaced
   difficult fixed zones with swipe-and-hold movement, and returned to faster
   pixel-exact 320×200 presentation.
6. **Non-blocking audio** — replaced synchronous per-sample PIO writes with a
   bounded DMA/IRQ queue, restored sound effects, and verified them on-device.
7. **Music experiment** — proved the WAD contains working music and implemented
   a fixed-memory synthesizer; retained it as an opt-in experiment after
   hardware listening tests favored effects-only audio.
8. **Safe BOOT rollback** — tested runtime BOOT-as-Escape, observed repeatable
   abnormal hardware behavior, removed the integration, and restored a UF2
   byte-identical to the confirmed-safe pre-BOOT build.
9. **Full-width performance** — measured 28.7 FPS, then overlapped tile packing
   with DMA and packed duplicate scale rows together to reach 35.2 FPS over a
   full minute without increasing static presentation memory.
10. **Playable touch-control pass** — repaired FT3168 Active/coherent tracking,
    tuned pointing-finger move/turn curves, rejected unreliable tilt movement,
    and added deterministic corner double-tap strafe bursts.

The detailed investigation—including failed hypotheses, hardware gotchas,
memory addresses, and root-cause evidence—is preserved in
[`docs/DECISIONS.md`](docs/DECISIONS.md). It is intentionally more detailed
than this README so future work does not repeat already-resolved diagnoses.

## Repository structure

```text
.
├── firmware/
│   ├── CMakeLists.txt          # Game and calibration firmware targets
│   ├── main.c                 # Standalone hardware-control calibration app
│   ├── engine/                # Vendored/adapted DOOM engine
│   │   ├── doom/              # Game logic and renderer-facing engine code
│   │   ├── pico/              # RP2350 video, input, sound, timing, and linker port
│   │   └── whd_gen/           # Host-side WAD-to-WHX conversion source
│   └── lib/                   # AMOLED, QSPI, touch, audio, codec, and board drivers
├── docs/DECISIONS.md          # Complete chronological engineering history
├── .Codex/                    # Tracked project log, context, TODOs, and handoff
├── AGENTS.md                  # Repository workflow rules for coding agents
└── CLAUDE.md                  # Current project status and immediate next steps
```

Local upstream checkouts, original WADs, WHX files, and build outputs are
ignored deliberately. They are either reproducible dependencies or game data
that should not be committed.

## Current limitations and next steps

- Repeat the successful E1M1-to-E1M2 run; the earlier silent freeze has not
  appeared in the latest run but is not formally closed yet.
- Audit essential Doom actions, especially weapon selection, then choose the
  smallest gesture vocabulary that remains usable during combat.
- Tune the F11 corner-strafe burst and quadratic forward/back curve only if a
  short combat test identifies a clear problem.
- Design an Escape/menu gesture using PWR or touch; never read BOOT in the
  playable firmware.
- Measure a 448×336 traditional 4:3-corrected display candidate before deciding
  whether reducing the top/bottom bars is worth its additional pixel work.
- Improve the optional music timbre before reconsidering music as the default.
- Revisit the WHX configuration before supporting retail multi-episode WADs.
- Investigate a small launcher plus independent flash slots for DOOM and a
  future Wolfenstein 3D/Spear of Destiny port.

## Credits and game data

- Engine foundation: [kilograham/rp2040-doom](https://github.com/kilograham/rp2040-doom)
- Desktop compatibility foundation: [Chocolate DOOM](https://github.com/chocolate-doom/chocolate-doom)
- Board and peripheral references: [Waveshare](https://www.waveshare.com/)

DOOM and its game data belong to their respective rights holders. This
repository contains porting work and source code, not a WAD. Supply only game
data you are legally entitled to use and retain the applicable upstream and
per-component license notices when redistributing binaries or source.
