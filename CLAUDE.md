# DOOM on RP2350-Touch-AMOLED-1.8 — Project Context

Read this first. Sibling context:
`../rp2350-touch-amoled-1.8-knowledge-base/project-ideas.md` has the original
feasibility research and reusable development workflows, and `../mp3player/`
is the first project on this board—its proven driver code is reused directly
here rather than re-derived.

## The goal
Create a self-contained handheld Doom experience that is enjoyable and
controllable enough to finish the game—not merely a technical port that boots.
Use the touchscreen, PWR button, and QMI8658 motion sensors without a keyboard
or external controller. Full-width video is the ambition, but smooth frame
pacing, readable motion controls, effects audio, memory stability, and combat
response take priority. See `docs/ROADMAP.md` for the staged experience plan.

## Engine
[kilograham/rp2040-doom](https://github.com/kilograham/rp2040-doom) — a
fully-featured Doom port for RP2040/RP2350 by Graham Sanderson (one of the
actual RP2040/RP2350 chip designers at Raspberry Pi), derived from
Chocolate Doom. The embedded subset is vendored and adapted under
`firmware/engine/`.
Reference display port for small SPI/I2C screens (not our exact QSPI
AMOLED, but proves the VGA-output default is forkable):
[rsheldiii/rp2040-doom-LCD](https://github.com/rsheldiii/rp2040-doom-LCD).

## Status: the game runs (2026-08-18)
Doom1.wad's first level loads fully and is playable on hardware — title
screen, menu navigation, level load, and in-game rendering all working.
Touch uses a floating two-axis pointing-finger control and PWR fires/selects.
The current presentation is full-width 448x280.
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
Alexander's latest play run progressed into E1M2 without freezing. Treat this
as strong evidence for the effects-only baseline, not yet proof of unlimited
long-session stability. Full-width 448x280 now sustains 35.2 presented FPS with
the asynchronous presenter and paired-row packer. Three pitch-locomotion
experiments were rejected because reliable starting and stopping remained too
difficult. The current control candidate uses simultaneous touch movement and
turning with the IMU completely disabled after roll strafing caused uncommanded
lateral movement. The strongest early touch-only test was playable but required
too much horizontal finger travel; F3 compressed turning into a 56px
bottom-left floating control. Its cubic curve then under-responded to short
combat swipes, so F4 uses a 6px dead zone and bounded quadratic response. Its
hardware test improved turning but exposed vertical movement as a delayed
fixed-speed switch. The current F5 candidate keeps F4 turning and diagonal
composition, starts forward/back after 4px at a gentle 8 units, and scales
linearly to Doom's 50-unit run bound at 44px. Its test accepted progressive
movement but rejected the remaining dead-zone delay. F6 reduces both axes to a
one-pixel jitter guard and lowers initial output to 4 movement/48 turn units,
while preserving the accepted curves, ranges, and maximum speeds. F6 still
required roughly a centimetre of physical travel, exposing a driver mismatch:
point tracking was initialized in Monitor mode and finger/X/Y were read through
separate transactions. F7 keeps the mapping unchanged, selects continuous
Active mode, and reads one coherent five-byte touch sample per tic. Its first
test confirmed subtle pointing-finger response but exposed
excessive gain and a remaining broad-thumb centroid limitation. F8 keeps all
response values and doubles movement/turn full scale to 88/112px for precision.
Its small pointing-finger turns are accepted but large turns are too laborious;
F9 raises only the quadratic outer turn maximum from 640 to 960. Its hardware
test accepted the pointing-finger touch model. F10's touch-gated roll strafe
worked technically, but touching after changing grip could arm an unexpected
sidestep; always-on tilt would restore the earlier drift problem. F11 compiles
the IMU out, maps bottom-corner double taps to short deterministic strafe
bursts, keeps double-tap Use elsewhere, and changes forward/back to a gentler
quadratic ramp reaching full run only after 140px. It is flashed and the
strafe is a modest improvement, but detailed tuning is deferred.
Runtime BOOT input was
tried and removed after two abnormal hardware tests; BOOT is reserved for
entering the ROM loader.

## Immediate next steps
1. Audit the remaining essential actions, led by weapon cycle and Escape/menu.
2. If desired, measure a 448x336 traditional 4:3 display candidate against the
   locked 448x280/35.2 FPS presentation.
3. Run a longer combat session, or begin a separate Wolf3D/Spear feasibility
   project and later investigate a two-game launcher.
4. Keep the locked full-width pipeline and effects-only audio as defaults.
5. Never read BOOT during gameplay.
