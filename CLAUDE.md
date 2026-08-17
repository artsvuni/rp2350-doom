# DOOM on RP2350-Touch-AMOLED-1.8 — Project Context

Read this first. Sibling context: `../knowledge-base/project-ideas.md` has
the original feasibility research (why this is realistic at all), and
`../mp3player/` is the first project on this board — its proven driver
code (display, buttons, toolchain/flashing workflow) is reused directly
here rather than re-derived. See `../CLAUDE.md` for the wider workspace.

## The goal
Get a real, playable port of Doom running on this board, controlled with
just the 2 physical buttons (BOOT, PWR) and the touchscreen — no keyboard.
Alexander's own retail WAD is the target; `DOOM1.WAD` (free shareware,
legal to redistribute) is the fallback/bring-up target.

## Engine
[kilograham/rp2040-doom](https://github.com/kilograham/rp2040-doom) — a
fully-featured Doom port for RP2040/RP2350 by Graham Sanderson (one of the
actual RP2040/RP2350 chip designers at Raspberry Pi), derived from
Chocolate Doom. The embedded subset is vendored and adapted under
`firmware/engine/`.
Reference display port for small SPI/I2C screens (not our exact QSPI
AMOLED, but proves the VGA-output default is forkable):
[rsheldiii/rp2040-doom-LCD](https://github.com/rsheldiii/rp2040-doom-LCD).

## Status: the game runs (2026-08-17)
Doom1.wad's first level loads fully and is playable on hardware — title
screen, menu navigation, level load, and in-game rendering all working.
Touch uses a floating swipe-and-hold four-way control and PWR fires/selects.
The current performance-test presentation is centered native 320x200.
Got here by root-causing and fixing two serious bugs in the same session
(a zone-memory-corruption freeze caused by a stray `calloc()` colliding
with the DOOM_TINY zone allocator's manually-claimed RAM, and a genuine
zone-exhaustion freeze caused by re-enabling the AMOLED display shrinking
the zone's available capacity below what the level needed) — see
`docs/DECISIONS.md`'s 2026-08-16 entries for the full bisection chain.
Milestone 0 (control-input layer validated) is also done — see the same
file for the register-level detail. A later silent freeze during sustained
combat is not ordinary zone OOM; persistent OOM reporting is in place.
Sound effects use a hardware-verified, non-blocking two-buffer DMA/IRQ
backend. The fixed-memory MUSX music synthesizer also works on hardware, but
its lightweight chiptune timbre is not enjoyable through this board's small
speaker. The normal build therefore defaults to effects only; the complete
music experiment remains available with `-DDOOM_ENABLE_MUSIC=ON`.
Runtime BOOT input was tried and removed after two abnormal hardware tests;
the current safe firmware never samples BOOT and is byte-identical to the
confirmed-good pre-BOOT build. BOOT is reserved for entering the ROM loader.

## Immediate next steps
1. Continue extended combat testing of the asynchronous SFX-only build.
2. If the combat freeze remains, add persistent core/render/game-tic stage
   heartbeats to identify the exact stalled subsystem.
3. Design an Escape/menu gesture using PWR or touch, and tune the floating
   touch dead zone/axis bias. Do not read BOOT in the playable firmware.
4. Optionally improve the music synthesizer's timbre before reconsidering it
   as a default, then strip the remaining temporary bootlog instrumentation
   once the port feels stable.
