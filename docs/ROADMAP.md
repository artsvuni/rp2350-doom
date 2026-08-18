# Experience and Engineering Roadmap

## North star

This project is no longer only a demonstration that Doom can run on the
RP2350-Touch-AMOLED-1.8. The goal is a self-contained handheld version that is
enjoyable and controllable enough that someone would choose to keep playing
and could realistically finish the game on this device.

That changes the success criteria. Feature parity is less important than the
quality of the experience:

- movement and aiming must feel intentional during combat;
- the game view should be as large as the hardware can sustain smoothly;
- sound effects stay because they work well on the small speaker;
- music stays optional unless a version sounds genuinely good;
- memory headroom and long-session stability cannot be traded away for a
  screenshot that merely looks better;
- port-specific UI should be quiet and useful, not obscure Doom.

Full-width 448x280 is the visual ambition, not a promise. The outcome should be
the largest view that maintains good frame pacing, input response, effects
audio, and enough memory to progress through real levels.

## Confirmed baseline — 17 August 2026

- The effects-only build boots, renders, plays, and produces good sound effects.
- Alexander progressed from E1M1 into E1M2 in the latest run without a freeze.
  This is strong baseline evidence, but longer repeated runs are still needed
  before declaring the earlier combat freeze closed.
- Centered native 320x200 presentation sends 128,000 bytes per frame in five
  packed display transfers.
- The effects-only image links with `__end__=0x20046ae8`, leaving 234,776 bytes
  for Doom's short-pointer zone through `0x20080000`.
- The known-good firmware can be rebuilt byte-for-byte and flashed from macOS
  through `picotool -f` without a physical BOOT press. Its UF2 SHA-256 is
  `0fa5d343884f25a2c9a99aeea84177eb2014417d5b4cdb0f8f26dd2e27a4f1e2`.
- The verified 16 MiB recovery image is stored outside the repository at
  `../device-backups/rp2350-doom-working-2026-08-17.bin`; SHA-256
  `58aef7f97a624a378cd3a6edd0ba47377852113c1351f16ff25dae90b152cb43`.
- Floating swipe-and-hold movement is playable, but not yet enjoyable enough
  for sustained combat.
- At this 17 August baseline runtime BOOT remained forbidden. The later F15
  milestone admitted one release-only next-weapon action after three staged
  hardware gates and elimination of the flash-backed audio-DMA source.

## Order of work

Performance comes before detailed motion-control tuning. Control experiments
need a stable visual and timing baseline; otherwise display latency can be
mistaken for poor sensor mapping and every video change invalidates the feel
test. This does not mean completing an unlimited driver rewrite first. Measure,
make the smallest useful video changes, then lock a presentation baseline and
move to controls.

## Phase 1 — establish a measured baseline

Add bounded, low-overhead instrumentation that can be compiled out. Record:

- game-tic time and whether the engine sustains its 35 Hz simulation target;
- render time, presentation packing time, DMA wait time, and complete presented
  frame cadence;
- dropped/skipped presentations and display timeout recovery;
- audio queue underflow/overflow pressure;
- linker/static-memory totals, Doom zone headroom, and stack high-water marks;
- input-to-command latency at the game-tic boundary.

Use a repeatable hardware route: boot, start E1M1, fight through representative
rooms, and reach E1M2. Keep the current UF2 and complete-flash backup as the
recovery baseline.

Gate to proceed: metrics are repeatable, diagnostics do not perturb play
materially, and the current effects-only build completes repeated E1M1-to-E1M2
runs without a new persistent fault.

### Phase 1 implementation checkpoint — 17 August 2026

The first measurement scaffold is now implemented behind
`DOOM_ENABLE_PROFILING=ON`. It waits through a 3-second user-controlled-level
warm-up, captures 60 seconds using hardware time, then checksum-saves one
aggregate report in reset-retained SRAM and watchdog-reboots. The next boot
stops before graphics, core1, audio, and Doom start and persists the 100-byte
report to reserved flash sector `0x101ff000`; the WHD starts at `0x10200000`.
The host reads and verifies that record through `picotool`. This separates game
work, render work,
display-frame wait, core1 rendezvous, presentation total, CPU preparation,
AMOLED transfer, complete frame cadence, and recovered DMA timeouts without
requiring a live terminal during play. Gameplay never writes flash; the one
report-sector write occurs only after reboot in a single-core diagnostic state
and cannot overlap WHD data. The normal build pays no profiler state and keeps
the proven linker boundary.

`DOOM_DISPLAY_WIDTH` now selects 320, 384, 416, or 448 at configure time. The
presenter keeps palette/overlay composition, exact nearest-neighbour scaling,
RGB565 byte order, and portrait transpose fused without a full scaled
framebuffer. All four Release modes compile. Their static-memory result is:

| Mode | `__end__` | Zone bytes to `0x20080000` |
|---|---:|---:|
| 320x200 | `0x20046ae8` | 234,776 |
| 384x240 | `0x20047ee8` | 229,656 |
| 416x260 | `0x200447e8` | 243,736 |
| 448x280 | `0x200492e8` | 224,536 |

The unusual 416x260 improvement is intentional: it uses one 20-row tile while
the other modes use 40 rows. Instrumented 320 and 448 images are ready for the
same-route hardware comparison. Do not select a display-driver rewrite until
those logs show whether CPU preparation, transfer, or frame synchronization is
the actual limit.

The first attempted live capture exposed a separate USB lifecycle bug. The
reused Waveshare module called `stdio_init_all()` during bootlog hardware setup,
then Doom's entry point called it again. macOS retained the enumerated CDC/reset
interfaces while TinyUSB's application state stopped servicing them. Stdio is
now owned only by executable entry points. Hardware confirms autonomous reset
works again, although CDC text remains silent. The persistent-log profilers
leave 233,648 zone bytes at 320x200 and 223,408 at 448x280. Their first short
320 capture averaged 22,917us cadence, 15,439us presentation (11,663us CPU
preparation and 3,775us transfer), with zero DMA timeouts. The checksum-valid
one-minute combat baseline then captured 2,507 frames in 60.015 seconds: 41.8
presented FPS, 23,948/42,958us average/max cadence, 15,460/20,351us
presentation, 11,664us CPU preparation, 3,795/4,197us transfer, 7,091us core1
work, 1,367us frame wait, at most 8us display wait, and zero DMA timeouts.
CPU preparation is 75.4% of presentation time and 48.7% of the whole average
frame. The first optimisation therefore targets transpose/compose locality and
tile size; neither the AMOLED transfer nor DMA reliability currently justifies
a wholesale display-driver rewrite.

The first bounded candidate replaces the fixed 40-row transpose tile with a
build-selectable height and chooses eight rows automatically (ten for
416x260). This shortens each strided write from 80 bytes to 16 bytes at the
three divisible modes. It also shrinks the 320 tile from 25,600 to 5,120 bytes
and the 448 tile from 35,840 to 7,168 bytes. Exact normal-build zone headroom
increases to 255,256 bytes at 320 and 253,208 bytes at 448. Both normal and
instrumented comparison images build; hardware profiling must decide whether
the more local writes outweigh the additional transfer setup.

The first 8-row 320x200 image was flashed and byte-verified but produced a
black panel while its application USB interface returned. It was rejected
before performance testing. Forty rows is restored as the playable default;
smaller tiles remain explicit experiments only. The next user-facing test is
the full-width 448x280 40-row profiler, which retains 223,408 bytes of exact
instrumented zone headroom.

That full-width capture is now complete and checksum-valid. It measured 1,724
frames in 60.026 seconds, or 28.7 presented FPS. Average/max cadence was
34,838/46,520us; presentation 28,934/35,225us; CPU preparation 21,803us;
transfer 7,130/7,600us; and render last/max 22,254/31,316us. Display wait
peaked at 9us and no DMA timeout occurred. Full width therefore needs only
about 1.5ms, or 4.3% of average cadence, removed to cross 30 FPS. Presentation
preparation remains the primary optimisation target; the measured DMA/panel
path remains healthy.

Alexander judged that build playable to slightly sluggish, but heard lagged,
stretched effects in the menu. The cause is consistent with the measured
schedule: two 512-sample buffers cover 23.2ms at 44.1kHz, while one full-width
presentation averages 28.9ms and can take 35.2ms. The first audio candidate
does not enlarge the queue. It services the existing non-blocking mixer after
each completed 40-row panel transfer, about seven opportunities per full-width
frame. All normal and profiled comparison builds pass with unchanged static
memory; a short hardware listening test is the remaining gate.

The first presentation-pipeline candidate is now build-complete behind
`DOOM_ASYNC_AMOLED=ON`. It replaces the one 40-row tile with two 20-row tiles:
DMA reads one while core1 packs the other. Their combined 35,840-byte footprint
is identical, so normal and profiled 448x280 linker endpoints remain
`0x200492e8` and `0x20049750`. The driver holds its mutex and chip-select from
asynchronous submission through bounded completion, keeping bootlog and other
display callers serialized. The synchronous path remains the default until a
short physical screen/audio check succeeds; only then should the one-minute
comparison run be requested.

That short hardware gate passed: the flashed/verified 448x280 candidate shows
the menu correctly, updates normally, and its menu effects sound good without
the previously reported stretching. This validates screen transaction order,
buffer lifetime, and the audio-service placement sufficiently to proceed to
the bounded one-minute combat comparison. It does not yet establish a frame-
rate improvement.

The subsequent checksum-valid combat report captured 2,024 frames in 60.012
seconds, or 33.7 FPS. Average/max cadence is 29,664/46,151us and presentation
23,940/29,114us. Blocking display service fell from 7,130us to 1,535us average,
so the pipeline hides about 78% of the old transfer cost. CPU preparation is
22,404us, display wait peaks at 9us, and DMA timeouts remain zero. Compared with
the synchronous 28.7 FPS baseline, cadence improves by 5,174us (14.9%) with no
static-memory penalty. Only about 1,093us more is needed to meet a 35 FPS
average cadence, so the next work targets CPU packing rather than another
driver rewrite.

The next CPU candidate targets the 80 source rows that 448x280 scaling emits
twice. When both copies remain in one 20-row tile, it writes them in one
strided x loop instead of loading and walking the 448-pixel scaled row twice.
No buffer is added. An exact host simulation matches the previous output-row
mapping for all three scaled modes, all builds pass, and linker endpoints are
unchanged. Profiler duration is now configurable per build, source-local to the
video file; intermediate hardware comparisons use 20 seconds while the final
accepted result will retain the one-minute route.

The checksum-valid 20-second hardware comparison then captured 727 frames in
20.009 seconds, or 36.3 FPS. Average/max cadence is 27,560/43,794us;
presentation 21,842/27,674us; CPU preparation 20,404us; and blocking display
service 1,437/4,193us. Display wait peaks at 9us and DMA timeouts remain zero.
Against the 33.7 FPS pipeline baseline, cadence improves by 2,104us and CPU
preparation by 2,000us, confirming that the paired-row loop produced the gain.
This clears the 35 FPS target directionally. One final 60-second combat capture
is the remaining gate before locking the video pipeline and moving to controls.

The final checksum-valid one-minute run captured 2,110 frames in 60.004
seconds, or 35.2 FPS. Average/max cadence is 28,451/46,503us; presentation
22,226/27,049us; CPU preparation 20,709us; and blocking display service
1,516/3,843us. Display wait peaks at 11us and DMA timeouts remain zero. Against
the original synchronous full-width baseline, cadence improves by 6,387us
(18.3%) and presentation by 6,708us (23.2%), while normal zone headroom remains
224,536 bytes. This meets the full-width performance gate. The normal
non-profile build is now installed, so regular gameplay does not auto-reboot;
video profiling is locked unless later experience testing exposes a concrete
regression.

## Phase 2 — maximise the game view

### 2A. Profile before rewriting

Separate the cost of Doom rendering from palette conversion, scaling,
transpose/packing, DMA waits, QSPI transfer, and multicore rendezvous. The old
448x280 path moved 250,880 display bytes per frame—almost twice the native
path—so the bus floor matters even if scaling arithmetic becomes free.

### 2B. Optimise the presentation path in small steps

Test one change at a time and record frame-time and SRAM deltas:

1. Keep palette lookup, scale, landscape transpose, and RGB565 packing fused so
   no full intermediate framebuffer is created.
2. Precompute the fixed horizontal/vertical source-index maps or use bounded
   accumulators; never divide once per output pixel.
3. Remove repeated window/command setup and redundant panel work from the hot
   frame path.
4. Measure safe QSPI/PIO clock headroom rather than assuming the current rate is
   either maximal or stable.
5. Evaluate smaller ping-pong tiles so CPU packing can overlap display DMA
   without consuming the memory of a second large frame tile.
6. Decouple game tics from display presentation where possible: preserve game
   responsiveness even when an occasional rendered frame is skipped.

Evaluate a quality ladder rather than jumping directly between two extremes:

| Mode | Output bytes/frame | Purpose |
|---|---:|---|
| 320x200 | 128,000 | Proven performance and memory baseline |
| 384x240 | 184,320 | First larger-view checkpoint |
| 416x260 | 216,320 | Near-full-width checkpoint |
| 448x280 | 250,880 | Full-width ambition |

All modes preserve Doom's intended 16:10 image shape.

### 2C. Rewrite only the constrained boundary

If profiling shows the Waveshare-derived display layer or its synchronous API
is the limiting factor, replace the Doom presentation boundary—not the engine
wholesale—with a small driver that explicitly owns:

- panel window setup;
- tile packing and byte order;
- bounded DMA submission/completion;
- PIO state and timeout recovery; and
- frame/presentation scheduling across the two cores.

Retain the current driver behind a build option until the replacement is
hardware-proven. Preserve the current two-buffer DMA/IRQ SFX backend; the
measured issue is refill scheduling during long presentation, not a reason for
a wholesale audio-driver rewrite.

Phase 2 exit: select the largest mode that feels smooth in combat, keeps input
responsive and SFX clean, survives the repeatable route, and leaves defensible
zone/stack headroom. If that is 416x260 rather than 448x280, it is a successful
experience decision rather than a failed optimisation.

## Phase 3 — design enjoyable controls

First add a selectable input-model boundary that produces the same bounded
`ticcmd_t` intentions regardless of source. Preserve the current model as the
comparison baseline and avoid changing game logic for every experiment.

### How the motion sensors can control Doom

The QMI8658 contains both an accelerometer and a gyro:

- The accelerometer can estimate pitch and roll from gravity. It gives a stable
  long-term neutral direction, but hand movement and vibration add noise.
- The gyro measures rotation rate. It responds quickly, but integrated angles
  drift and therefore need recentering or correction from gravity.
- A complementary filter can combine slow accelerometer attitude with fast gyro
  response using fixed-size, fixed-point state.

Yaw cannot be inferred from gravity alone. Left/right *tilt* is roll; a gyro can
measure deliberate yaw rotation, but treating yaw as an absolute angle requires
drift management. Small pitch/roll angles are preferable because the display
must remain readable while the player controls it.

For every motion model:

1. Average 0.5–1.0 seconds of a comfortable grip to set neutral.
2. Apply a dead zone, asymmetric enter/exit thresholds, and low-pass filtering.
3. Use a nonlinear response: gentle near neutral, faster near the edge.
4. Clamp output and feed proportional `ticcmd_t.forwardmove`/`angleturn` rather
   than rapidly toggling virtual keys once the simple prototype is stable.
5. Provide an explicit recenter gesture and never allocate memory while polling.
6. Burst-read at roughly the game-tic rate on core 0 and keep shared I2C1 access
   bounded and serialized with touch, codec, RTC, and power management.

### Models to compare

- **A — floating touch baseline:** current four-way swipe-and-hold plus PWR
  actions. Preserve unchanged for comparison and fallback.
- **B — full tilt (not pursued):** pitch would control forward/back and roll
  would turn. Hardware evidence from the simpler pitch-only candidates now
  makes this too risky for screen readability and dependable stopping.
- **C — horizontal touch + pitch movement (rejected):** swipe/drag left-right
  turned while pitch walked or reversed. Three mappings all failed the
  enjoyable/reliable stopping requirement.
- **D — tilt steering + touch movement:** roll or gyro steers; vertical touch
  swipe controls forward/back. This may make speed/stopping more explicit while
  testing whether motion is better suited to aiming than movement.
- **E — gyro-assisted variant:** add gyro rate to C or D for quicker turns while
  the accelerometer stabilises neutral. Explore only after a simpler hybrid has
  usable calibration and recentering.
- **F — touch navigation + optional roll strafe:** vertical touch moves and
  horizontal touch turns. The first combined test preferred touch but rejected
  roll because it caused uncommanded lateral movement.
- **F2 — touch-only tuning (active):** keep simultaneous touch move/turn and
  disable IMU initialisation/polling until touch sensitivity is locked.
- **F3 — compact floating touch (active):** keep the same floating anchor but
  fit full horizontal response inside the preferred bottom-left grip area with
  a cubic precision curve.
- **F4 — responsive compact touch (active):** retain the 56px range and 640
  maximum, but use a smaller dead zone and quadratic curve so short combat
  swipes turn reliably.
- **F5 — progressive compact movement (active):** retain F4 turning and
  diagonal composition, but replace the delayed fixed-speed vertical switch
  with a 4px dead zone and an 8-to-50 movement ramp reaching full scale at
  44px.
- **F6 — near-zero dead zones (active):** use a one-pixel jitter guard on both
  axes and gentler minimum outputs, preserving F5's curves, full-scale
  distances, maximum speeds, and diagonal composition.
- **F7 — active coherent touch reporting (active):** hold the F6 mapping
  constant, switch FT3168 point tracking from Monitor to Active mode, and read
  finger/X/Y as one register burst before judging another control model.
- **F8 — pointing-finger precision (active):** retain F7's immediate tracking
  and all response bounds, but double both full-scale distances so a few
  millimetres no longer reaches aggressive movement or turning.
- **F9 — faster outer turn (active):** retain F8's precise quadratic start and
  112px range, but raise only the outer turn maximum from 640 to 960 so large
  pointing-finger gestures reorient quickly.
- **F10 — deliberate touch-gated roll strafe (rejected):** keep F9 touch intact;
  allow proportional accelerometer roll strafing only while touch is held,
  learn neutral only while released, start near 10 degrees after two tics,
  stop inside 5 degrees, and scale 8-to-32 through roughly 22 degrees.
- **F11 — corner dodge and slower travel (active):** compile motion out; map
  bottom-corner double taps to bounded left/right strafe bursts, retain Use on
  double taps elsewhere, and use a 140px quadratic forward/back ramp.

Judge each model on the same route and score:

- ability to move, turn, fire, and use doors intentionally;
- combat accuracy and time to recover from over-turning;
- accidental movement and ease of stopping;
- ability to see the screen while controlling it;
- hand fatigue after 10–15 minutes;
- learnability without instructions; and
- whether E1M1-to-E1M2 progression feels easier than Model A.

The hybrid intentionally changes the action mapping as part of the UX
experiment: a PWR tap fires immediately and touchscreen double-tap owns
Use/Open. F12 cautiously revisits PWR hold with a 450ms software timer, one-shot
fire, and suppression through release, well before the PMIC's documented
1–2.5-second long IRQ and 4–10-second power-off ranges. A continued physical
hold can still power off the board. Menus retain their existing swipe
navigation and short/double PWR semantics, while the same 450ms hold acts as
Escape/Back without emitting Fire. Runtime BOOT input remains excluded pending
a separate safety investigation.

### Phase 3 implementation checkpoint — 17 August 2026

The first three pitch-movement candidates are rejected. Proportional pitch
needed too much tilt, the stateful gearbox rebased neutral and behaved
unpredictably, and fixed-neutral zones still made stopping difficult enough to
feel like controlling water. The next candidate remains behind
`DOOM_HYBRID_CONTROLS=ON`, while the floating-touch model stays available as
the fallback. It adds no heap allocation and samples one six-byte XYZ
accelerometer burst per game tic.

The first interaction contract is:

| Input | In-level action |
|---|---|
| Hold vertical touch displacement | Fixed-speed forward/back; release stops |
| Hold horizontal touch displacement | Reduced proportional left/right turn |
| Hold device roll outside neutral | Optional fixed-speed left/right strafe |
| Double-tap touchscreen | Use/Open once |
| Tap PWR | Fire once immediately |

The first hardware test confirmed responsive/enjoyable horizontal turning and
working double-tap door use, but rejected proportional tilt and PWR hold. The
second test rejected the stateful gearbox: direction was inverted and rebasing
settled poses produced delayed, unpredictable starts and poor stopping.

The F1 hardware test preferred the touch-first direction but rejected roll:
uncommanded left/right strafing continued with no finger down, while touch
turning remained too fast. F2 disables IMU initialisation and polling, keeps
vertical movement unchanged, and slows turning from a 960 to 640 maximum while
widening the dead zone and full-scale drag distance. It adds 32 bytes of static
SRAM over the locked full-width baseline, leaving 224,504 zone bytes. The next
gate is touch-only direction, stopping, diagonal control, and aiming precision.

F2 then produced the easiest physical playthrough so far, but useful turning
still required enough finger travel to obscure the view. F3 keeps touch-only
semantics and the 640 maximum, but compresses horizontal full scale from 120 to
56 pixels and uses an 8-pixel dead zone plus cubic response. The player can land
in the bottom-left and operate within a compact area; no fixed region or overlay
is introduced until the response curve itself is validated.

F3 remained playable through E1M1, but its cubic curve under-responded to short
left/right combat swipes in E1M2. F4 keeps forward/back and the compact maximum
unchanged, reduces horizontal dead zone from 8 to 6 pixels, raises minimum turn
from 64 to 80, and restores a quadratic response. This isolates responsiveness
without raising the maximum speed.

F4 improved left/right responsiveness, but the physical test exposed the
unchanged vertical path as a delayed binary switch: no output through 10 pixels,
then fixed 25-unit walking. F5 keeps horizontal response and diagonal
composition unchanged while starting vertical movement after 4 pixels at 8
units and scaling linearly to the 50-unit run bound at 44 pixels.

F5 made vertical response progressive, but its 4-pixel vertical and 6-pixel
horizontal dead zones still felt like delay, followed by an aggressive first
response. F6 reduces both to a one-pixel jitter guard and lowers initial output
to 4 movement units and 48 turn units. Mid-range and maximum response remain
nearly unchanged.

F6 still required roughly a centimetre of reported physical travel, which a
one-pixel software guard cannot cause. F7 keeps the mapping unchanged and
repairs the FT3168 path: continuous Active point mode and one coherent
five-byte finger/X/Y read per tic. If a 1–2mm motion still does not register,
the next model is a fixed bottom-left eight-way D-pad whose initial touch
location commands direction without establishing a relative neutral anchor.

F7's physical test then established a finger-size split. A smaller pointing
finger now produces subtle response, validating the driver direction, but
reaches full movement/turn speed too easily. A broad thumb still shifts the
reported centroid less reliably. F8 doubles movement full scale from 44 to 88
pixels and turn full scale from 56 to 112 pixels without changing initial or
maximum output. Thumb-oriented D-pad work remains separate.

F8's small turns feel precise, and the pointing finger is now the intended
relative-control contact, but large turns require too much effort. F9 leaves
movement and the entire horizontal near/mid mapping intact while raising the
quadratic curve's edge maximum from 640 to 960.

F9 is accepted as a usable touch baseline. F10 proved that touch-gated roll
could strafe but exposed an irreducible UX conflict: touch was unexpectedly
required, and changing grip before touch could arm an immediate move. Enabling
roll continuously would restore the earlier drift. F11 therefore uses a no-IMU
build, uses explicit bottom-corner double-tap dodge bursts, and makes the
forward/back ramp calmer through the middle while preserving F9 turning.

F12 leaves F11 navigation unchanged and tests the remaining Escape action. It
fires once on the immediate PWR press edge, opens Escape once after a 450ms
continuous hold, then discards PWR events through release so the release cannot
select a menu item. The build adds eight static bytes and leaves 224,492 bytes
of short-pointer headroom. Hardware validation is pending.

The in-level F12 interaction passed, but press-time Fire necessarily occurred
before every hold. F12.1 resolves the gesture on release: a short release emits
Fire in gameplay or Enter/Select in menus; reaching 450ms emits Escape/Back and
suppresses release. The hybrid PWR double-click path is bypassed because hold
now owns Back.

F12.1 felt very comfortable on hardware, but adjacent rapid taps could merge
into one continuously sampled Fire state. F12.2 adds a four-pulse fixed queue
and inserts one explicit released tic between queued shots. It preserves the
first release's latency and the 450ms Escape/Back hold.

F12.2's physical test still lost Alexander's fastest click-click. F12.3 handles
the additional PMIC ordering case: if the release of an active tap and the next
press arrive in one latched status mask, complete the first tap and preserve the
second press rather than clearing both. This remains a bounded polling design;
two complete clicks entirely between Doom tics cannot be counted by the
AXP2101's occurrence flags and do not justify high-rate shared-I2C IRQ work.

F12.3 did not materially change the pistol test. Doom's 14-tic pistol recovery
explains the middle-speed loss: a one-tic Fire pulse during recoil expires
before `A_ReFire` checks input. Keep vanilla cadence and avoid invisible
delayed-shot buffering. F13 instead compares navigation models: a default-off
fixed 160x160 bottom-left eight-way D-pad with a 12px neutral centre and
normal/fast radial response, while preserving the accepted pointing-finger
build unchanged.

F13's first physical comparison selected its interaction direction: fixed
touch-and-hold feels more traditional, controllable, and thumb-friendly than
the relative mapping. F14 now tests Alexander's asymmetric edge-based layout:
narrow LEFT and DOWN targets anchored to tactile panel edges, larger UP and
RIGHT targets, no inter-zone dead zones, release-to-stop, and only two narrow
forward-turn transition bands. A compile-time boundary-only overlay makes the
actual mapping visible during calibration without allocating another buffer.
The physical result was the best control experience so far: cardinal zones,
release-to-stop, thumb use, and both forward diagonals passed, while the guides
made the mapping easier to use. Keep the overlay during longer testing and
design its permanent treatment only after the interactions are locked.

F14.1 restores the one missing action by allowing a bounded stationary double
tap inside any control zone to emit Use/Open. It preserves immediate movement,
so the door gesture produces two tiny direction pulses instead of delaying all
normal touch holds. Hardware-confirm this compromise, then treat F14 as the
navigation baseline and proceed to the BOOT safety audit/weapon-cycle phase.

The subsequent BOOT investigation completed. The exact schematic and SDK audit
found the earlier missed hazard: empty-queue audio DMA read a `const` silence
buffer from XIP while BOOT temporarily floated flash CS. The new path moves all
active DMA sources to SRAM, uses SDK multicore flash coordination, and acts only
after release in local level play. An isolated probe, silent Doom, and normal
effects-enabled Doom all passed. F15 therefore maps one short BOOT release to
native next-weapon cycling; hold/double/menu/network actions remain excluded.

The next isolated control candidate is F15.1. It leaves the F15 binary exact
when disabled and reuses the same 25 ms flash-safe sampler when enabled. A
short release remains next weapon; after roughly 300 ms of physical hold, BOOT
becomes a modifier that maps the F14 LEFT/RIGHT zones to sustained strafe and
the forward transition bands to forward-strafe. Release restores turning and
does not cycle weapon. The candidate is built and statically audited but must
pass one bounded hardware hold test before it can replace F15.

That test passed. Short release retained weapon cycling, held LEFT/RIGHT
produced strafe, release restored turning, and Alexander reported the combined
model worked great. Accept it as F16 and treat essential controls as closed.
Future sensitivity work is refinement rather than a prerequisite for moving to
the taller-display experiment.

The first taller-display candidate is deliberately 448x320, not 448x336. It
reduces the top and bottom bands from 44 to 24 pixels and adds 14.3% output
pixels while keeping the proven two-buffer 20-row asynchronous transaction
path unchanged. 448x336 would add 20% pixels but is not divisible by 20, so it
also requires a partial final transfer or a newly validated tile size. The
448x320 Release image builds with unchanged static memory and the same 220,176
zone bytes as F16; its first gate is a short physical image, guide, audio, and
in-level correctness check before any timed combat capture.

That short gate passed. Alexander judged 448x320 excellent and visually much
better, so the experiment advances to 448x336. This is the exact square-pixel
equivalent of Doom's intended 4:3 CRT presentation: 320x200 content corrected
to 320x240, then scaled 1.4x. The final 16 image rows are carried in the proven
20-row transaction with four cleared border rows, avoiding the rejected 8-row
path. The candidate adds no SRAM and awaits the same short physical gate before
performance capture or any full-panel experiment.

The 448x336 physical gate also passed. The image looked correct and good, and
Alexander observed no loss of smoothness versus 448x320. Accept exact 4:3 as
the preferred visual baseline while retaining 448x320 and measured 448x280 as
rollback points. Because that smoothness judgment is qualitative, capture one
bounded combat report before deciding whether display work is complete. Do not
treat 448x368 as the automatic next scale: it exceeds the intended 4:3 image
shape and needs a separate experience decision.

Alexander made that separate experience decision in favour of testing game
content across the complete panel rather than reserving the last 16-pixel
bands for permanent port UI. F18 is the bounded 448x368 comparison: it stretches
the accepted 4:3 frame by 9.5% vertically, preserves the existing two 20-row
buffers, and assembles the final eight image rows into an overlapping full-size
transaction using the preceding buffer. It adds no static memory and keeps
220,176 zone bytes. The panel clear uses the same full-size-tail principle to
test whether its old 8-row final stripe caused the photographed coloured edge
remnant. F18 must pass a short physical correctness/feel gate before profiling
or replacing 448x336.

That gate passed decisively. Alexander loves the 448x368 result, described it
as feeling really good, and selected it as the core experience. Accept F18
despite its explicit 9.5% vertical stretch beyond exact 4:3; on this device the
benefit of using the complete panel outweighs geometric purity. Preserve F17
448x336 as the aspect-correct rollback and measured 448x280/35.2 FPS as the
performance rollback. A bounded F18 capture is useful baseline documentation,
but subjective acceptance is complete and measurement must not automatically
restart optimisation.

The optional capture is now complete: 2,059 frames in 60.021 seconds, or 34.3
presented FPS, with 29,164/45,612us average/max cadence and zero DMA timeouts.
That is only 2.5% slower in average cadence than the measured 448x280 rollback
while emitting 31.4% more pixels. F18 remains the locked core presentation;
there is no evidence-led reason to reopen the video pipeline now.

## Phase 4 — port-specific experience polish

Only after video and controls are good enough for sustained play:

- test a minimal battery indicator sourced from the AXP2101, shown only where
  it does not obscure critical game information;
- add a small port settings menu for control model, sensitivity, recentering,
  video size/performance mode, audio, and diagnostics;
- experiment with hiding or simplifying the vanilla status bar, while checking
  whether health, armour, ammo, keys, and weapon feedback become worse;
- revisit music only if a higher-quality low-cost rendering sounds enjoyable on
  the physical speaker; effects-only remains the product default otherwise;
- revisit retail multi-episode WAD layout and save/load only after the core
  handheld experience is dependable.

## Phase 5 — optional mini retro console

After Doom's essential controls are closed, treat Wolfenstein 3D or Spear of
Destiny as a separate port that reuses the proven board support rather than
mixing another engine into Doom prematurely. Before implementation:

1. audit the 16 MiB flash budget for two firmware images and both user-supplied
   game-data sets;
2. identify a maintained RP2350-suitable Wolf3D/Spear engine and its licensing
   constraints;
3. extract or share the proven AMOLED, touch, PWR, audio, and flash-layout
   interfaces without coupling the game engines;
4. compare independent bootable flash slots against one monolithic firmware;
   independent images are the safer starting architecture; and
5. add a minimal launcher only after both games boot independently.

The product idea is a tiny two-game retro console with a first-boot game menu.
It is technically plausible on this hardware, but flash partitioning, asset
placement, reset/selection flow, and legal user-supplied game data must be
designed explicitly rather than assumed.

## Decision rules

- Optimise measured bottlenecks, not code that merely looks old.
- Preserve a known-good build and change one performance variable at a time.
- Prefer removing a low-value feature over degrading the whole experience.
- Do not spend memory headroom without recording the exact cost.
- Hardware verification includes how it feels, not only whether it boots.
- Full width is desirable; smooth, controllable, stable Doom is mandatory.
