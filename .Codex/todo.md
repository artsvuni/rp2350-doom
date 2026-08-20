# TODO

## Controls

- [x] Research established one-finger directional controls for small touchscreens.
- [x] Document the current fixed hold-zone model.
- [x] Implement swipe-direction-and-hold movement anywhere on screen.
- [ ] Hardware-test and tune swipe dead zone/axis bias.
- [ ] If swipe-and-hold is unsatisfactory, redesign the fixed touch-zone sizes and placement.
- [x] Map BOOT long-press to Escape using bounded cross-core flash lockout.
- [x] Replace BOOT long-press with a debounced single press that emits Escape once per hold.
- [x] Abandon and remove all runtime BOOT polling after both BOOT-enabled tests behaved abnormally.
- [x] Prototype long PWR press as Escape/menu without reading BOOT at runtime (rejected after it conflicted with firing and preceded an apparent physical restart).
- [x] Implement F12 short PWR hold: fire once immediately, emit Escape once at 450ms, and suppress the completed press cycle through release.
- [x] Hardware-test F12 in-level tap/hold behaviour (worked: immediate one-shot fire and short hold opened Escape/menu).
- [x] Hardware-test release-resolved F12.1 (Fire and Escape feel very comfortable in gameplay and menus; very fast click-click can collapse into one shot).
- [x] Implement F12.2 bounded PWR pulse queue with one sampled low tic between rapid shots and queue flush before Escape.
- [x] Hardware-test F12.2 rapid Fire (paced pairs work, but the fastest click-click still collapses into one shot).
- [x] Implement F12.3 preservation of a coalesced active-tap release plus next press.
- [x] Hardware-test F12.3 fastest click-click (no practical improvement; middle-speed pulses can expire during the pistol's 14-tic recovery, while extreme complete cycles may still coalesce at the PMIC).
- [x] Complete the BOOT safety audit covering the exact schematic, flash-CS electrical path, SDK contract, CPU/XIP/DMA interactions, and staged physical protocol.
- [x] Build the isolated single-core BOOT safety probe with amplifier/display disabled and no DMA/I2C/PIO/audio/core1/USB stdio.
- [x] Hardware-test exactly one short BOOT press/release with the isolated probe (passed: entered ROM BOOTSEL cleanly), then restore and reboot F14.1.
- [x] After the probe passed, move audio DMA's flash-backed silence source to SRAM, register Doom core1 with `flash_safe_execute_core_init()`, and build/audit an audio-disabled default-off next-weapon candidate.
- [x] Hardware-test one release-based next-weapon event in audio-disabled Doom, then restore exact F14.1 (passed; one weapon change and no abnormal behaviour).
- [x] Build and statically audit a separately gated effects-enabled BOOT candidate with every active DMA source in SRAM.
- [x] Hardware-test one release-based next-weapon event with normal sound effects (passed; weapon cycling worked well and everything appeared normal).
- [x] Build and statically audit a default-off BOOT-hold strafe modifier while
  preserving byte-identical F15 when disabled.
- [x] Hardware-test one bounded F15.1 sequence: short BOOT next-weapon, held
  BOOT plus LEFT/RIGHT strafe, release-to-turn, normal SFX, and no abnormality
  (passed; accepted as F16 and reported to work great).
- [x] Preserve the floating swipe-and-hold controls as documented Control Model A.
- [x] Introduce a compile-selectable hybrid control model without changing Model A's fallback behavior.
- [x] Prototype QMI8658 accelerometer tilt with neutral calibration, dead zone, fixed-point filtering, and proportional output (rejected: useful movement required too much tilt).
- [x] Replace absolute proportional tilt with a small-gesture reverse/stopped/forward movement state machine (rejected: wrong direction, delayed/unpredictable transitions, unreliable stop).
- [x] Replace stateful rebasing with fixed-neutral forward/stop/back zones and start/stop hysteresis (rejected: stopping remained difficult and accidental movement persisted).
- [x] Decide whether to continue full tilt or pitch locomotion (no: three hardware candidates failed dependable start/stop control).
- [x] Prototype horizontal touch/drag for turning plus pitch tilt for forward/back, returning to neutral to stop.
- [x] Reassign in-game Use/Open from delayed PWR double-click to a bounded touchscreen double-tap.
- [x] Prototype immediate PWR press/release edge handling for responsive held fire (rejected: PMIC long press opened menu and a longer hold appeared to restart).
- [x] Hardware-confirm responsive horizontal touch turning and double-tap Use/Open.
- [x] Hardware-test fixed-neutral pitch zones (rejected as too difficult to start/stop dependably).
- [x] Implement simultaneous touch forward/back plus reduced-sensitivity touch turning, with release as a guaranteed stop.
- [x] Demote motion to direct hold-to-strafe device roll with a wider 6-degree start zone and 3-degree stop hysteresis.
- [x] Hardware-test touch-plus-roll F1 (touch direction felt better; turn remained too fast; roll caused uncommanded lateral movement and was rejected).
- [x] Split roll strafing behind a separate default-off build switch so the active touch build never initialises or polls the IMU.
- [x] Hardware-test touch-only F2 (easiest playthrough so far; movement controllable, but useful turning required too much finger travel and obscured the view).
- [x] Compress horizontal response into a 56px floating bottom-left control range with a cubic precision curve and unchanged 640 maximum.
- [x] Hardware-test compact F3 (playable through E1M1, but short combat swipes under-responded and turning remained difficult).
- [x] Increase compact horizontal response with a 6px dead zone, 80 minimum, quadratic curve, and unchanged 640 maximum.
- [x] Hardware-test F4 short-swipe turning (left/right responsiveness improved; unchanged binary vertical response became the limiting imbalance).
- [x] Replace fixed-speed vertical touch with a compact progressive mapping while retaining F4 turning and independent diagonal composition.
- [x] Hardware-test F5 (progressive movement works, but the remaining 4px/6px dead zones still feel delayed and activation feels too abrupt).
- [x] Reduce both axes to a one-pixel jitter guard and lower their initial outputs while preserving mid/high response.
- [x] Hardware-test F6 (a one-pixel software guard still required roughly a centimetre of physical travel, pointing below the mapping layer).
- [x] Put FT3168 point tracking in Active mode and replace separate finger/X/Y reads with one coherent register burst.
- [x] Hardware-test F7 (pointing finger now responds to subtle motion; broad thumb remains less responsive; existing compact ranges are too fast).
- [x] Double movement and turning full-scale distances for pointing-finger precision without changing initial or maximum output.
- [x] Hardware-test F8 (pointing finger selected; small turns feel good, but large turns require too much effort).
- [x] Raise only the outer quadratic turn maximum from 640 to 960 while preserving F8 movement and small-turn mapping.
- [x] Hardware-test F9 for precise small aiming plus easier deliberate 90/180-degree turns (accepted as the touch baseline).
- [x] Implement F10 deliberate proportional roll strafe gated by active touch, with no-touch neutral learning and release-to-stop.
- [x] Hardware-test F10 (mechanism worked, but touch gating was unintuitive and touching after a grip change could arm unexpected strafe; rejected).
- [x] Implement F11 bottom-corner double-tap strafe bursts with Use/Open retained elsewhere and the IMU compiled out.
- [x] Replace the 88px linear forward/back response with a calmer 140px quadratic ramp.
- [x] Perform an initial F11 hardware check (corner strafe works and is better than no strafe; detailed tuning deferred).
- [ ] Hardware-test F11 corner direction/burst length, door Use outside corners, and low/mid/far forward speed.
- [x] Audit essential Doom actions: Escape/menu and next-weapon are now covered; automap and direct weapon numbers remain optional.
- [x] Select the smallest deliberate weapon-cycle action: one release-only BOOT press, avoiding more overlapping touch vocabulary.
- [x] Implement F13 fixed bottom-left eight-way touch-and-hold D-pad with immediate absolute-position activation and normal/fast radial response.
- [x] Hardware-compare F13 D-pad against the accepted pointing-finger model (directionally accepted: more traditional, controllable, and thumb-friendly).
- [x] Implement F14 asymmetric contiguous thumb zones with edge-sized LEFT/DOWN, larger UP/RIGHT, deliberate forward-turn transition bands, and a compile-time diagnostic overlay.
- [x] Hardware-tune F14 geometry (best control result so far; thumb reach, cardinals, release-to-stop, overlay, and both forward diagonals passed; diagonals remain refinable).
- [x] Allow bounded stationary Use/Open double taps inside all F14 control zones without delaying held movement.
- [x] Hardware-test F14.1 in-zone Use (works well; accepted for continued play).
- [x] Hardware-test F17 state-consistent input: fixed D-pad in menus, a fresh
  D-pad/PWR press advances the E1M1 score screen into E1M2, and DOWN again
  occupies only 20 visible game-image pixels at 448x336 (passed; issue fixed).
- [ ] A/B test an explicitly menu-only swipe mode against F17 fixed-zone menu
  navigation; do not change gameplay or intermission routing.
- [ ] Replace Doom's keyboard-specific Quit Game prompt with `TAP PWR TO
  QUIT`, and translate a short PWR/menu-confirm tap to `key_menu_confirm` only
  while a Y/N dialog is active. Preserve PWR hold/Escape as cancel.
- [ ] Explore a polished permanent visual treatment for the helpful control guides after interaction mapping is locked.
- [x] After touch-only controls are locked, decide whether motion still earns a role (no continuous tilt after F10; use explicit touch dodge gestures).
- [x] Do not add a recenter action after continuous roll control was rejected.

## Stability and performance

- [x] Reconnect, rebuild, flash, and confirm the board operates normally after the earlier abnormal test.
- [x] Confirm that the pre-BOOT-polling effects-only firmware boots and operates normally again.
- [ ] Repeat the successful E1M1-to-E1M2 run and confirm extended-play stability with the current 234,776-byte effects-only zone.
- [x] Add a persistent exact-allocation OOM diagnostic.
- [x] Hardware-test centered pixel-exact 320x200 video during active combat.
- [ ] If 320x200 still freezes, add persistent core/render/game-tic heartbeat stages.
- [x] Make SFX output DMA/IRQ-driven and non-blocking, then re-enable sound effects.
- [x] Hardware-test menu/gameplay SFX and immediate frame pacing in the effects-only build.
- [x] Implement a fixed-memory MUSX music backend and compatible named-lump playback path.
- [x] Hardware-test music playback and confirm the MUSX/WAD/backend path works.
- [x] Research the speaker/codec, current synthesis failure, upstream OPL2,
  prerecorded-audio fallback, and performance/memory trade-offs.
- [x] Build a reversible speaker-mastered fixed-memory music candidate with
  smoother voices, note envelopes, music-only filtering, peak control, lower
  default level, and SFX ducking.
- [x] Hardware-listen to the mastered title/menu and E1M1 (rejected: first
  build was inaudible; corrected gain emitted distorted intermittent bursts).
- [x] Restore exact effects-only F18 after manually entering BOOTSEL; flash
  verification passed and the board rebooted into application mode.
- [ ] If music is revisited, port upstream OPL2 with fixed static state and no
  `calloc`, instrumenting refill cost/underflow before physical listening.
- [x] Add a default-off keyboard-free save mode that immediately generates
  slot/map/elapsed-time labels without requiring an RTC.
- [ ] Hardware-test creating and overwriting an automatic save, then loading it
  after a normal reboot. Two attempts froze before any save-area byte changed;
  the corrected game-tick/core1 pause candidate successfully created one save.
  Load, overwrite, and post-reboot persistence remain.
- [x] Replace slot/map/time labels with the simpler Doom level title plus slot
  number (`HANGAR 1`, `HANGAR 2`, and so on), without RTC integration.
- [x] Refine the final label order to `SAVED GAME <slot> - <level>`, with the
  number dictated by the selected save slot.
- [x] Add compile-out frame timing for game, render, core rendezvous, presentation packing, AMOLED transfer, cadence, and DMA timeout recovery.
- [x] Make 320x200, 384x240, 416x260, and 448x280 presentation modes selectable from CMake and verify all four Release builds.
- [ ] Capture comparable on-device 320x200 and 448x280 profiler logs using the same gameplay route.
- [ ] Establish a repeatable baseline for zone headroom, stack high-water marks, frame/tic timing, display waits, and audio queue pressure.
- [ ] Benchmark 320x200, 384x240, 416x260, and 448x280 presentation modes using the same gameplay route.
- [x] Instrument render, palette/scale/transpose packing, DMA/QSPI transfer, and multicore rendezvous separately.
- [x] Replace live-only USB profiling with a short real-level flight recorder and reset-retained report mode.
- [x] Hardware-confirm that single-owner TinyUSB initialization restores autonomous reset; CDC text remains unavailable and is bypassed by the persistent report record.
- [x] Capture the one-minute 320x200 combat baseline after the 3-second warm-up.
- [x] Hardware-check the selectable 8-row 320x200 transpose candidate (rejected: verified firmware booted to a black panel).
- [x] Hardware-profile the full-width 448x280 40-row build using the one-minute combat route (28.7 presented FPS; zero DMA timeouts).
- [x] Add a zero-buffer audio refill opportunity between completed full-width display chunks.
- [x] Hardware-listen to menu SFX in the interleaved-audio full-width build (clean, no reported stretching).
- [x] Implement a compile-selectable, memory-neutral 20-row ping-pong AMOLED presenter.
- [x] Hardware-check the 448x280 asynchronous presenter for correct image, audio, and menu stability before starting its profiler.
- [x] Capture and compare the asynchronous 448x280 one-minute combat report against the 28.7 FPS synchronous baseline (33.7 FPS, zero DMA timeouts).
- [x] Remove about 1.1ms from full-width CPU packing to target a sustained 35 presented FPS (20-second capture reached 36.3 FPS).
- [x] Build a memory-neutral paired-row packing candidate with exact scaled-row equivalence.
- [x] Hardware-check and capture the 20-second paired-row full-width profiler (36.3 FPS; zero DMA timeouts).
- [x] Run one final 60-second paired-row combat capture before locking the video pipeline (35.2 FPS; zero DMA timeouts).
- [x] Decide whether a wholesale display rewrite is needed (no: targeted pipelining reached 35.2 FPS full-width).
- [ ] After instrumentation isolates the remaining freeze, perform a phased refactor of display, input, audio, and diagnostics behind unchanged interfaces.
- [ ] Audit linker-map/static buffers and remove measured duplication or dead code one change at a time, recording SRAM and frame-time deltas.
- [x] Add a selectable memory-neutral 448x320 stepping-stone that preserves the
  proven 20-row asynchronous presenter and exact F16 AUTO rollback.
- [x] Hardware-check 448x320 image, guides, effects audio, and a short E1M1 run
  (passed; Alexander judged it excellent and visually much better).
- [x] Add bounded partial-final-tile support for an exact-4:3 448x336 candidate
  while preserving the 20-row transaction and all static-memory headroom.
- [x] Hardware-check 448x336 image, guides, effects audio, and a short E1M1 run
  (passed; looks good, feels very smooth, no perceptible loss versus 448x320).
- [x] Retire the planned 448x336 gate after F18 won the direct experience test;
  retain 448x336 as the exact-4:3 rollback.
- [x] Build a memory-neutral asynchronous 448x368 experiment with a full
  overlapping 20-row tail instead of an unproven 8-row transaction.
- [x] Make the one-time panel clear end with a full overlapping stripe instead
  of a short 8-row transfer.
- [x] Hardware-check and accept F18 full-panel presentation as the core
  experience; Alexander loves the result and reported it feels really good.
- [x] Capture one bounded 448x368 combat performance report for the permanent
  baseline record (34.3 FPS over 60.021 seconds; zero DMA timeouts); retain F18.
- [ ] If any coloured edge remnant reappears, photograph it before changing
  panel initialisation or addressing; none was reported during F18 acceptance.

## Future ports and launcher

- [ ] Audit Wolfenstein 3D/Spear of Destiny engine candidates, licensing, controls, and user-supplied data formats for RP2350.
- [ ] Measure the 16 MiB flash budget for Doom firmware/data, a Wolf/Spear firmware/data slot, and a launcher/recovery reserve.
- [ ] Design independent bootable application slots and a minimal game-selection menu; avoid a monolithic two-engine binary unless it proves simpler and safe.
- [ ] Port Wolf3D/Spear independently using the shared AMOLED, touch, PWR, audio, and flash knowledge, then integrate the launcher only after both games boot alone.

## Completed

- [x] Verify autonomous `picotool -f` firmware flashing on the current Mac without a physical BOOT press.
- [x] Save a complete 16 MiB flash backup and verify a source-unchanged build byte-for-byte before reflashing.
- [x] Progress from E1M1 into E1M2 in a normal effects-only gameplay run without freezing.
- [x] Filter touch boundary chatter and remove blocking per-transition logs.
- [x] Implement both aspect-correct 448x280 scaling and native 320x200 presentation.
- [x] Replace the 128KB rotated framebuffer with a tiled transpose buffer.
- [x] Clear noisy letterbox panel RAM.
- [x] Expand the RP2350 short-pointer zone by 64KB.
- [x] Disable on-screen boot diagnostics after graphics init and clear their panel remnants.
