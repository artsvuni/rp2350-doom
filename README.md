# DOOM on the RP2350-Touch-AMOLED-1.8

## Goal

Create an enjoyable, self-contained handheld version of DOOM on the tiny
[Waveshare RP2350-Touch-AMOLED-1.8](https://www.waveshare.com/rp2350-touch-amoled-1.8.htm)
using only the board's touchscreen and two onboard buttons—no keyboard,
external controller, or display. The ambition is not just to prove the port
runs, but to make combat controllable and the presentation smooth enough that
finishing the game on this device feels realistic.

This project adapts Graham Sanderson's
[RP2040/RP2350 DOOM port](https://github.com/kilograham/rp2040-doom) to the
board's 368×448 SH8601 AMOLED, FT3168 touch controller, AXP2101 power
management IC, and ES8311 audio codec. The result boots, renders, loads E1M1,
and is playable on the physical device.

> **Current status (20 August 2026):** the F18 effects-only build is verified on
> hardware with working touch controls, menu navigation, combat, and sound
> effects. The installed full-panel 448×368 presentation sustains **34.3 FPS**
> over a measured one-minute combat run with zero display-DMA timeouts and no
> additional static presentation memory. The 448×280 rollback measures 35.2
> FPS. The lightweight music baseline now works and sounds acceptable, but
> remains disabled by default while candidates are compared inside real Doom.
> The standalone player was retired after both music and a direct reference
> tone were silent outside Doom's complete runtime lifecycle. F18
> uses fixed asymmetric thumb zones, double-tap Use/Open, PWR tap/hold for
> Fire and Escape, a short BOOT release for next weapon, and BOOT hold as a
> strafe modifier for the LEFT/RIGHT zones. Alexander judged this combination
> comfortable and the strongest playable control experience so far. Tilt
> locomotion and continuous tilt strafing were tested and rejected as
> unreliable. The installed image fills the complete 448×368 panel. It
> deliberately stretches the exact-4:3 version by 9.5% vertically, but
> Alexander loves the result and selected it as the core experience after its
> hardware test. Exact-4:3 448×336 and measured 448×280 remain rollbacks.

The staged product and engineering plan is in
[`docs/ROADMAP.md`](docs/ROADMAP.md).

## Display geometry

The physical AMOLED is 368×448 in portrait, or **448×368** in the landscape
orientation used by Doom. The installed F18 game image is **448×368**, filling
the complete panel without permanent UI bands.

| Mode | Top/bottom bands | Pixels vs 448×280 | Status and trade-off |
|---|---:|---:|---|
| 448×280 measured rollback | 44px each | baseline | 35.2 FPS; raw square-pixel 16:10 image |
| 448×320 rollback | 24px each | +14.3% | Physical gate passed; visually excellent; performance not measured |
| 448×336 F17 rollback | 16px each | +20.0% | Exact traditional 4:3 correction; hardware passed |
| 448×368 F18 core | none | +31.4% | 34.3 FPS measured; hardware-accepted full panel; stretches exact 4:3 by 9.5% vertically |

The raw 320×200 framebuffer is 16:10 only when treated as square pixels.
Original Doom was normally shown on a 4:3 CRT using vertically taller pixels;
the equivalent square-pixel correction is 320×240, or exactly 448×336 here.
The successful 448×320 test moves partway toward that intended shape. F17
448×336 completes the exact 4:3 correction. F18 then scales the complete frame
another 9.5% vertically to fill the panel; that distortion is explicit, but its
physical experience is preferred. Its final eight rows are combined with 12
overlapping rows in one proven 20-row transaction using the existing two
buffers. Future battery state should appear only as a low-battery overlay,
rather than reclaiming permanent bands from the game.

## What works

- DOOM Shareware boots from flash and loads the first episode.
- Full-panel 448×368 gameplay is hardware-accepted as the core experience;
  exact-4:3 448×336 is the visual rollback and measured 448×280 remains the
  performance rollback.
- Fixed visible thumb zones control forward/back movement and left/right
  turning; two narrow boundaries combine forward movement with turning.
- A stationary double tap activates Use/Open, including inside the thumb zones;
  a bottom-right double tap produces a short right-strafe burst.
- A short PWR release fires in-game or confirms in menus; a 450 ms hold opens
  Escape/Back without firing first.
- A short BOOT press/release cycles to Doom's next selectable owned weapon in a
  local single-player level.
- Holding BOOT converts the LEFT/RIGHT thumb zones from turning to sustained
  strafing; releasing it restores turning without changing weapon.
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
| Hold LEFT / UP / RIGHT / DOWN touch zone | Turn or move at normal Doom speed | Fixed-zone navigation |
| Hold an UP boundary band | Move forward while turning | — |
| Stationary double tap | Use/open | — |
| Double-tap bottom-right outside the pad | Short strafe-right burst | — |
| PWR short release | Fire once | Select/confirm |
| PWR hold for 450 ms | Escape/menu | Back |
| BOOT short press/release | Next selectable owned weapon | No action |
| Hold BOOT + LEFT / RIGHT | Strafe left / right until released | No action |
| Continued long PWR hold | Board-managed power action | Board-managed power action |

On an intermission/score screen, release the touchscreen once after leaving the
level, then tap any touch zone or PWR to accelerate the statistics and continue
using Doom's native Fire action.

The asymmetric touch layout uses the physical left and bottom edges to make its
small LEFT and DOWN targets discoverable, while UP and RIGHT receive more area.
There is no neutral software zone: lifting the thumb stops movement immediately.
The visible boundary overlay materially improved control during hardware tests
and remains enabled. The previous floating pointing-finger and radial D-pad
models remain available through build switches.

BOOT shares the external-flash chip-select signal, so its runtime use is not a
normal GPIO read. F15's short release was admitted only after an isolated probe, a silent Doom
test, and a sound-enabled Doom test all passed. The sampler runs from SRAM via
the Pico SDK's multicore flash-safety coordinator, ignores failed lockouts,
and is disabled outside local level play. F16 reuses the same brief 25 ms
sampler—flash is not held suspended during the physical hold—and distinguishes
tap from hold in software. Every active display/audio DMA source is SRAM-backed.
Full evidence and the rollback
protocol are in [`docs/BOOT-RUNTIME-SAFETY.md`](docs/BOOT-RUNTIME-SAFETY.md).

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

The current full-panel F18 build is configured explicitly so the conservative
320×200 fallback remains the CMake default:

```sh
cmake -S firmware -B firmware/build-controls-448 \
  -DCMAKE_BUILD_TYPE=Release \
  -DDOOM_DISPLAY_WIDTH=448 \
  -DDOOM_DISPLAY_HEIGHT_OVERRIDE=368 \
  -DDOOM_PANEL_CHUNK_ROWS=20 \
  -DDOOM_ASYNC_AMOLED=ON \
  -DDOOM_HYBRID_CONTROLS=ON \
  -DDOOM_TOUCH_DPAD=ON \
  -DDOOM_TOUCH_DPAD_THUMB_ZONES=ON \
  -DDOOM_TOUCH_DPAD_OVERLAY=ON \
  -DDOOM_BOOT_NEXT_WEAPON=ON \
  -DDOOM_BOOT_HOLD_STRAFE=ON \
  -DDOOM_BOOT_WITH_SOUND_EFFECTS=ON \
  -DDOOM_ROLL_STRAFE=OFF
cmake --build firmware/build-controls-448 --target doom -j4
```

### Keyboard-free save names

The optional handheld save flow skips Doom's keyboard-only description editor
and saves immediately with a deterministic label such as
`SAVED GAME 1 - HANGAR`. The number comes from the selected slot and the title
comes from Doom's current level; no RTC or keyboard is required:

```sh
cmake -S firmware -B firmware/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DDOOM_HANDHELD_AUTO_SAVE_NAMES=ON \
  -DDOOM_FLASH_SAFE_SAVES=ON
cmake --build firmware/build --target doom -j4
```

The option defaults off so conventional keyboard builds keep vanilla text
entry. `DOOM_FLASH_SAFE_SAVES` is required on the multicore handheld. Save is
requested during Doom's game tick, before the next render wakes core 1; the
inherited display-oriented pause deadlocks at that boundary. This option uses
a dedicated core-1 pause rendezvous, keeps DMA sources in SRAM, and then uses
the Pico SDK lockout around each flash-sector erase/program operation.

### Music

The fixed-memory music backend is included in the normal handheld firmware.
Sound effects start enabled and music starts muted, matching the preferred
pocket experience. To opt in, open Doom's **Options** menu and raise
**Music Volume**; no alternative firmware is required. A fresh boot restores
the default effects-only balance. At volume zero the music generator is
detached, so the synthesiser and continuous audio-buffer production consume no
gameplay CPU time; the fixed music implementation still occupies about 1.3 KiB
of SRAM because it remains available for immediate opt-in.

The accepted optional score uses the original lightweight synthesiser, with
General MIDI percussion at 50% of its original level. The heavier mastered
candidate remains rejected because it produced intermittent distorted bursts.

```sh
cmake -S firmware -B firmware/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DDOOM_ENABLE_MUSIC=ON \
  -DDOOM_MUSIC_SPEAKER_MASTERING=OFF
cmake --build firmware/build --target doom -j4
```

For a smaller effects-only engineering build, configure
`-DDOOM_ENABLE_MUSIC=OFF`; that removes the music backend entirely rather than
merely muting it.

For the controlled in-game smooth-synth comparison, keep mastering off and add
`-DDOOM_MUSIC_SMOOTH_SYNTH=ON`. This changes only oscillator shapes,
instrument-family mapping, and short note-edge gain ramps; sample rate, buffer
size, codec configuration, volume, display, and controls remain unchanged.

`DOOM_MUSIC_PERCUSSION_PERCENT` independently controls General MIDI channel
10. Its accepted default is `50`, leaving tonal instruments and sound effects
untouched.

### Music listening builds

Use full Doom for physical listening tests. This retains the real renderer
load, core-1 servicing, sound effects, mixer, DMA queue and codec lifecycle,
so sound quality and stability results represent the shipped experience.
Start with `DOOM_ENABLE_MUSIC=ON`, `DOOM_MUSIC_SPEAKER_MASTERING=OFF`, and
`DOOM_MUSIC_LAB=OFF` as the accepted audible rollback.

The experimental lab code remains available for engineering reference and has
four selectable profiles:

- `0`: exact accepted 44.1kHz lightweight baseline;
- `1`: cheap smooth voices, instrument-family mapping, and note-edge ramps;
- `2`: profile 1 at 22.05kHz with 256-frame buffers;
- `3`: profile 1 at 44.1kHz plus experimental ES8311 DAC DRC.

Do not use those standalone images for product decisions: hardware testing
found even their direct output-path reference tone silent. See
[the music experiment](docs/MUSIC-QUALITY-EXPERIMENT.md) for the comparison
protocol and why music remains off by default.

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
| Input | FT3168 fixed thumb zones and double taps; AXP2101 PWR release/hold; flash-safe BOOT tap for weapons and hold modifier for strafe in local play |
| Audio | ES8311 + PIO I2S with two static 512-frame DMA buffers and IRQ refill |
| Game data | WAD preprocessed to compressed WHX and read directly from XIP flash at `0x10200000` |
| Failure diagnostics | On-screen boot checkpoints and watchdog-scratch OOM reports |

The SH8601 cannot exchange display axes in hardware, so landscape presentation
is performed as a bounded tiled transpose rather than with a full rotated
framebuffer. The presentation path uses two 20-row buffers: DMA reads one while
core 1 packs the other. Repeated vertical scale rows are packed together. At
448×368, the last eight rows are assembled with 12 rows from the preceding
buffer and sent as one overlapping full-size transaction. This fills the panel
without a full scaled framebuffer or a third tile buffer.

The F18 full-panel effects-only build links with `__end__=0x20049bf0`; after
its fixed 2 KiB heap it leaves 220,176 bytes before core 1's stack boundary at
`0x20080000`. Its installed UF2 SHA-256 is
`5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`. The
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
11. **Thumb-first complete controls** — adopted fixed asymmetric touch zones,
    restored Use and menu/intermission routing, and safely added PWR Escape,
    BOOT weapon cycling, and BOOT-hold strafing.
12. **Full-panel experience** — advanced through 448×320 and exact-4:3 448×336,
    then hardware-accepted 448×368 as the preferred core experience without
    adding static presentation memory; its final one-minute combat capture
    measured 34.3 FPS with zero display-DMA timeouts.

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
- Refine F18 sensitivity, guide styling, and zone geometry only through
  isolated comparisons; essential actions are covered.
- A/B test the promising menu-only swipe interaction against fixed-zone menu
  navigation without changing gameplay or intermission controls.
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
