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
- [ ] Design Escape/menu without BOOT or any PWR hold gesture; defer until movement feels good.
- [ ] Later, if still useful, isolate BOOT-as-input in a single-core audio-disabled test using the SDK's `flash_safe_execute()` API; not a current priority.
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
- [ ] Audit essential Doom actions before closing controls: weapon change first, then Escape/menu; treat automap and direct weapon numbers as optional.
- [ ] Prototype the smallest deliberate weapon-cycle gesture; compare top-corner previous/next double taps with a stationary long-touch action.
- [ ] If broad-thumb support remains important after pointing-finger tuning, prototype a fixed bottom-left eight-way touch-and-hold D-pad with initial-touch activation.
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
- [ ] Experiment with higher-quality optional music synthesis before reconsidering the default.
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
- [ ] Add a selectable 448x336 4:3-corrected output candidate and measure FPS, audio pacing, and memory against locked 448x280 before considering it permanent.

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
