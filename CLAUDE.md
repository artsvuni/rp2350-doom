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
Touch uses fixed asymmetric thumb zones and PWR fires/selects.
F18 448x368 is installed and hardware-accepted as the core presentation;
448x336 remains the exact-4:3 rollback and 448x280 the measured 35.2 FPS
rollback. F18 deliberately stretches exact 4:3 by 9.5% vertically, reuses the
existing two 20-row buffers for an overlapping final transaction, and adds no
static SRAM. Alexander loves the full-panel result and observed no smoothness
regression during its acceptance run. A later checksum-valid one-minute combat
capture measured 34.3 presented FPS (2,059 frames in 60.021 seconds), only 2.5%
slower in cadence than the 448x280/35.2 FPS rollback, with zero DMA timeouts.
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
backend. The fixed-memory MUSX music synthesizer is now included in the normal
firmware but Music Volume starts at zero, preserving the preferred effects-only
experience. Raising Music Volume in Options enables the accepted lightweight
score with percussion at 50% of its original gain. At zero, the generator is
detached so music retains about 1.3 KiB fixed SRAM but consumes no continuous
synthesis or silent-buffer CPU time.
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
F14 supersedes that relative navigation direction as the leading hardware
candidate. It uses contiguous asymmetric thumb zones anchored to the left and
bottom screen edges, with larger forward/right targets, narrow forward-turn
transition bands, release-to-stop, and visible guide outlines. Alexander judged
it the best control experience so far and found the guides useful. F14.1 keeps
movement immediate while allowing the existing stationary double tap inside
any control zone to emit Use/Open on the second release.
F15 safely reintroduced one release-only BOOT action after an isolated probe,
silent two-core Doom, and effects-enabled Doom all passed: a short release
cycles to the next owned weapon in local play. A separate default-off F15.1
candidate interprets a deliberate BOOT hold as a touch strafe modifier. Its
hardware test passed and it is accepted/installed as F16. F15 remains the
release-only rollback and F14.1 the no-runtime-BOOT rollback.
F17 is the exact-4:3 rollback and its control routing remains part of F18. It
fixes the state-routing bug that returned menus to legacy swipes and left
intermission without an Attack/Use command, and keeps DOWN's visible image
overlap at 20 pixels after the taller 448x336 presentation. The accidental menu
swipe felt good, so a deliberately menu-only swipe option remains a future A/B
test rather than a rejected idea.

## Immediate next steps
1. Later A/B test fixed-zone menu navigation against a deliberately menu-only
   swipe mode without changing gameplay or intermission input.
2. Run a longer combat session, or begin a separate Wolf3D/Spear feasibility
   project and later investigate a two-game launcher.
3. Keep the locked full-panel pipeline and runtime-muted optional music as the
   default; do not reopen music work without a concrete regression or goal.
4. Do not expand BOOT beyond the accepted F15 short release or the explicitly
   gated F15.1 hold candidate without a new safety and interaction review.
