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
Chocolate Doom. Not yet integrated into this repo — next milestone.
Reference display port for small SPI/I2C screens (not our exact QSPI
AMOLED, but proves the VGA-output default is forkable):
[rsheldiii/rp2040-doom-LCD](https://github.com/rsheldiii/rp2040-doom-LCD).

## Status: the game runs (2026-08-16)
Doom1.wad's first level loads fully and is playable on hardware — title
screen, menu navigation, level load, and in-game rendering all working.
Got here by root-causing and fixing two serious bugs in the same session
(a zone-memory-corruption freeze caused by a stray `calloc()` colliding
with the DOOM_TINY zone allocator's manually-claimed RAM, and a genuine
zone-exhaustion freeze caused by re-enabling the AMOLED display shrinking
the zone's available capacity below what the level needed) — see
`docs/DECISIONS.md`'s 2026-08-16 entries for the full bisection chain.
Milestone 0 (control-input layer validated) is also done — see the same
file for the register-level detail.

## Immediate next steps
1. Wire the validated button/touch input (control-input milestone) into
   actual gameplay — currently the game runs but isn't controllable yet.
2. Design/finalize the actual control scheme (which action maps to which
   press-pattern/touch-zone) — mostly discussed already, not yet written
   down as a spec. Includes the still-open ESC/double-tap-BOOT mapping
   request.
3. Make the audio path non-blocking so real sound can be re-enabled
   (`DEBUG_NO_SOUND=1` is currently a stopgap — see `docs/DECISIONS.md`).
4. Strip the extensive temporary bootlog diagnostic instrumentation
   scattered across the engine once the port feels stable.
