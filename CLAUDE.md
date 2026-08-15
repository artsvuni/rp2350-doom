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

## Status: control-input layer validated, engine not yet integrated
Milestone 0 (this far): confirmed we can reliably distinguish single/
double/long press on **both** buttons before writing any game logic
against that assumption. Full detail and the actual register-level
reasoning in `docs/DECISIONS.md`.

## Immediate next steps
1. Design the actual control scheme (which action maps to which
   press-pattern/touch-zone) — mostly discussed already, not yet written
   down as a spec.
2. Pull in `rp2040-doom`, get it building for RP2350 at all (verify on the
   `chocolate-doom` desktop target first, matching upstream's own
   verification approach, before touching RP2350-specific code).
3. Adapt its display output to our AMOLED (`../mp3player/firmware/lib/`
   has proven, working `AMOLED_1IN8_Display()` code to build on).
4. Wire in the validated button/touch input from this milestone.
5. Get a WAD loading and rendering at all before worrying about
   correctness/completeness.
