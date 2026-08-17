# DOOM on the RP2350-Touch-AMOLED-1.8

## Goal

Run real, playable DOOM on the tiny
[Waveshare RP2350-Touch-AMOLED-1.8](https://www.waveshare.com/rp2350-touch-amoled-1.8.htm)
using only the board's touchscreen and PWR button—no keyboard, external
controller, or display.

This project adapts Graham Sanderson's
[RP2040/RP2350 DOOM port](https://github.com/kilograham/rp2040-doom) to the
board's 368×448 SH8601 AMOLED, FT3168 touch controller, AXP2101 power
management IC, and ES8311 audio codec. The result boots, renders, loads E1M1,
and is playable on the physical device.

> **Current status (17 August 2026):** the effects-only build is verified on
> hardware with working touch controls, menu navigation, combat, and sound
> effects. Music also works, but is disabled by default because the lightweight
> synthesized timbre is not a good fit for the small speaker. Longer sustained
> combat testing is still in progress.

## What works

- DOOM Shareware boots from flash and loads the first episode.
- Pixel-exact 320×200 gameplay is centered in landscape orientation.
- One-finger movement works anywhere on the screen through a floating,
  swipe-and-hold digital joystick.
- The PWR button handles fire, use, menu selection, and menu back.
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
| Touch, then swipe up and hold | Move forward | Move selection up |
| Touch, then swipe down and hold | Move backward | Move selection down |
| Touch, then swipe left/right and hold | Turn left/right | Move selection left/right |
| PWR single press | Fire | Select/confirm |
| PWR double press | Use/open | Back |
| PWR long press | Board-managed power action | Board-managed power action |
| BOOT | Reserved for entering BOOTSEL while powering/resetting | — |

The gesture starts wherever the finger lands. Movement begins after a
24-pixel dead zone, and an axis bias plus a two-tic stability filter prevents
touch jitter from rapidly changing directions. The original fixed touch-zone
model is preserved behind a compile-time selector for future experiments.

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
| Display | SH8601 QSPI output with a 40-row software-transpose tile; five packed DMA transfers per 320×200 frame |
| Input | FT3168 floating four-way gesture and AXP2101 PWR events; runtime BOOT access intentionally disabled |
| Audio | ES8311 + PIO I2S with two static 512-frame DMA buffers and IRQ refill |
| Game data | WAD preprocessed to compressed WHX and read directly from XIP flash at `0x10200000` |
| Failure diagnostics | On-screen boot checkpoints and watchdog-scratch OOM reports |

The SH8601 cannot exchange display axes in hardware, so landscape presentation
is performed as a bounded tiled transpose rather than with a full rotated
framebuffer. Native 320×200 output sends 128,000 display bytes per frame and
avoids the CPU and QSPI cost of the earlier 448×280 scaler.

The effects-only build currently links with `__end__=0x20046ae8`, leaving
234,776 bytes for DOOM's short-pointer zone up to `0x20080000`. Enabling music
leaves 233,448 bytes. The zone's exact address matters: upstream compresses
pointers into a fixed 256 KiB window, so ordinary heap allocation cannot be
substituted casually.

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

- Continue long-duration combat testing. An earlier silent freeze was proven
  not to be ordinary zone exhaustion, but has not yet been conclusively
  isolated.
- Tune the floating gesture's dead zone and axis bias through longer play.
- Design an Escape/menu gesture using PWR or touch; do not read BOOT in the
  playable firmware.
- Evaluate the planned tilt and hybrid tilt-plus-touch control models against
  the current floating swipe-and-hold baseline.
- Improve the optional music timbre before reconsidering music as the default.
- Revisit the WHX configuration before supporting retail multi-episode WADs.

## Credits and game data

- Engine foundation: [kilograham/rp2040-doom](https://github.com/kilograham/rp2040-doom)
- Desktop compatibility foundation: [Chocolate DOOM](https://github.com/chocolate-doom/chocolate-doom)
- Board and peripheral references: [Waveshare](https://www.waveshare.com/)

DOOM and its game data belong to their respective rights holders. This
repository contains porting work and source code, not a WAD. Supply only game
data you are legally entitled to use and retain the applicable upstream and
per-component license notices when redistributing binaries or source.
