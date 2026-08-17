# Project Log

## 2026-08-18

### 00:31 — Wrap the playable full-width controls milestone

- Updated the publication README to reflect the measured 448x280/35.2 FPS
  pipeline, current F11 pointing-finger controls, rejected continuous tilt,
  effects-only audio decision, build flags, and exact 224,500-byte headroom.
- Recorded panel geometry: 448x368 landscape, current 44px top/bottom bands,
  and a future measured 448x336 4:3 candidate with 16px bands and 20% more
  output pixels.
- Added next-session work for essential control coverage, led by weapon change
  and Escape/menu, while deferring minor F11 tuning unless clearly valuable.
- Captured the longer-term Wolfenstein 3D/Spear of Destiny plus two-game
  launcher idea as a separate flash-slot/driver-reuse feasibility phase.
- Commit: included in this milestone-documentation commit.

### 00:15 — Replace tilt strafe with corner dodge bursts

- Recorded F10 as mechanically functional but UX-rejected: requiring touch was
  unexpected, and a grip change before touch could arm an unintended strafe.
- Compiled motion out and mapped bottom-left/right 96x72px double taps to
  six-tic, 32-unit directional strafe bursts; double-tap Use remains elsewhere.
- Changed forward/back from an 88px linear ramp to a 140px quadratic ramp while
  retaining the one-pixel guard and 4-to-50 output bounds; F9 turning is intact.
- The full-width asynchronous F11 build passes with `__end__=0x2004930c`,
  224,500 zone bytes, no QMI8658 input symbols, and UF2 SHA-256
  `ec00e1262a4a3f53f93c9cb09cd6fc49b4e39ef4d9fe2ae820f30d1f1eba2cd6`.
- Hardware validation remains pending.
- Commit: included in this corner-dodge-and-movement-curve commit.

### 00:01 — Add deliberate touch-gated proportional roll strafe

- Recorded F9 pointing-finger touch as the accepted navigation baseline and
  left every touch response value unchanged.
- Redesigned optional roll strafing so it can act only while touch is held;
  release guarantees zero sidemove and provides a stable neutral-learning
  window, while active play freezes the completed reference.
- Added a two-tic 10-degree entry, 5-degree stop hysteresis, and proportional
  8-to-32 output reaching full response around 22 degrees.
- The full-width asynchronous F10 build passes with `__end__=0x20049330`,
  224,464 zone bytes, and UF2 SHA-256
  `af30c8362cb6cd837bedb560e71998057f7f577a40e6fc28c5e9174c04777816`.
- Physical direction, stop reliability, and combat value remain to be tested.
- Commit: included in this touch-gated-roll-strafe commit.

## 2026-08-17

### 23:53 — Accelerate large pointing-finger turns

- Recorded the pointing finger as the intended relative-control contact and F8's small-turn precision as directionally accepted.
- Kept the one-pixel guard, 48-unit initial turn, quadratic curve, and 112px range; raised only outer maximum turn from 640 to 960.
- Forward/back, touch driver, diagonals, double-tap Use, tap-fire, and release-to-stop are unchanged.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `b8e94eebb5310750031883db351b14e019e67703e17c72d826726b6d19b97820`.
- Hardware testing remains pending.
- Commit: included in this faster-outer-turn commit.

### 23:40 — Expand touch ranges for pointing-finger precision

- Recorded that F7 enables subtle pointing-finger input, validating the Active/coherent driver direction, while a broad thumb remains less reliable; a stable reported contact point across a changing broad contact is the leading inference rather than a measured cause.
- Identified excessive gain as the new pointing-finger problem: maximum movement at 44px and maximum turning at 56px are reached within a few millimetres.
- Doubled movement full scale to 88px and turn full scale to 112px; one-pixel guards, initial outputs, curves, maximums, diagonals, and release-to-stop are unchanged.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `7f8f3e3df65668e23cbb0bf0579b42b4be83f933c6313d45fdf388188e9a8dfc`.
- Hardware testing remains pending.
- Commit: included in this pointing-finger-precision commit.

### 23:30 — Repair FT3168 point tracking before replacing controls

- Traced F6's roughly centimetre-scale physical delay below its one-pixel software guard: the inherited driver selected FT3168 Monitor mode for point tracking and sampled finger/X/Y separately.
- Point mode now selects continuous Active tracking; gesture mode retains Monitor mode.
- Replaced four touch I2C reads with one coherent five-byte `0x02..0x06` burst and updated every caller to consume its returned finger count.
- Both firmware targets build; Doom text shrinks by 104 bytes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols. UF2 SHA-256 is `7cf7565432d02df9643d05de79d98a57cb01eb3f08933e3082286522c1102579`.
- Hardware testing remains pending; F6 mapping is deliberately unchanged. Commit: included in this active-touch-driver commit.

### 22:53 — Minimise touch dead zones without adding an activation jump

- Recorded F5's progressive movement as directionally correct, but both axes still felt delayed around their 4px/6px dead zones and aggressive at activation.
- Reduced horizontal and vertical dead zones to a one-pixel jitter guard, allowing output from the next coordinate step.
- Lowered minimum movement from 8 to 4 and minimum turn from 80 to 48, preserving the established curves, full-scale distances, maximum speeds, diagonals, and release-to-stop.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `66f33d5c59e809047c9e8748184fa3a320d28fe33f3f34c99ca55508e40c5efe`.
- Hardware testing remains pending.
- Commit: included in this near-zero-dead-zone commit.

### 22:44 — Make compact vertical movement immediate and progressive

- Recorded that F4 improved left/right responsiveness, while forward/back still waited through a larger 10px dead zone and then jumped to fixed 25-unit walking.
- Kept F4's horizontal response and independent diagonal move-plus-turn composition unchanged.
- Reduced the vertical dead zone to 4px and added a linear 8-to-50 movement ramp reaching full scale at 44px, with release still guaranteeing stop.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `749e7fea1b0fea663d797b07135c2b89e543dfe0cec37f422f7cabee78c4b13f`.
- Hardware testing remains pending.
- Commit: included in this progressive-touch-movement commit.

### 22:30 — Strengthen short compact combat swipes

- Recorded F3 as playable through E1M1 with good forward/back control, but too unresponsive to short left/right swipes during E1M2 combat.
- Kept the compact 56px range and bounded 640 maximum; reduced horizontal dead zone from 8 to 6px, raised minimum from 64 to 80, and changed cubic response to quadratic.
- Forward/back, diagonal composition, release-to-stop, double-tap Use, tap-fire, and the compiled-out IMU are unchanged.
- Full-width build passes with unchanged `__end__=0x20049308` and 224,504 zone bytes; UF2 SHA-256 is `9c8783efa353573284afc8fec59f7241a927c7ee514aeef42e6e0e89d71a7186`.
- Commit: included in this responsive-compact-turning commit.

### 21:59 — Compress touch turning into a bottom-left grip

- Recorded F2 as the easiest playthrough so far; the remaining major issue was excessive horizontal finger travel obscuring the full-screen view.
- Kept the floating touch-down anchor so the player can choose the bottom-left without a fixed overlay or restricted screen region.
- Compressed full turn response from 120 to 56px, reduced dead zone from 10 to 8px and minimum from 80 to 64, kept the 640 maximum, and changed quadratic response to cubic for fine control near neutral.
- Forward/back, diagonals, release-to-stop, double-tap Use, tap-fire, and the compiled-out IMU remain unchanged.
- Full-width build passes with unchanged `__end__=0x20049308` and 224,504 zone bytes; UF2 SHA-256 is `c81069f72199bc11a5d251c3d8ef06d5ac8237db925625f778124f29b3c357f4`.
- Commit: included in this compact-touch-control commit.

### 21:35 — Disable roll and slow touch turning

- Recorded that touch navigation feels better, but the F1 roll path caused uncommanded left/right strafing with no finger down and is rejected for the active build.
- Split roll behind default-off `DOOM_ROLL_STRAFE`; the touch-only build performs no IMU initialisation, reads, or `sidemove` work.
- Reduced touch turn maximum from 960 to 640, minimum from 120 to 80, widened the dead zone from 8 to 10px, and moved full scale from 96 to 120px; forward/back is unchanged.
- Touch-only, opt-in roll, and fallback full-width builds pass. Touch-only `__end__=0x20049308`, leaving 224,504 zone bytes; UF2 SHA-256 is `c0a9fc35cc6ccdacb536d1842bbf8168eb079e4582fd5257a0b1d9d55dbe4a12`.
- Commit: included in this touch-only tuning commit.

### 21:16 — Make touch primary and demote motion to roll strafing

- Recorded the fixed-neutral pitch test as rejected: it remained difficult to start and stop reliably and could turn an attempted stop into reverse movement.
- Added simultaneous vertical touch movement and horizontal touch turning, with release as a guaranteed stop; reduced maximum turn response from 1600 to 960.
- Replaced pitch locomotion with optional direct-position roll strafing: stable neutral calibration, 6-degree two-tic start, 3-degree stop, and no delayed software low-pass.
- Both hybrid and fallback full-width builds pass. Hybrid `__end__=0x20049330`, leaving 224,464 zone bytes; fallback remains `0x200492e8`; candidate UF2 SHA-256 is `5a8c5498bf5258174d5dca86f12a9bdd3242b7b009d58601fb14d742f18104bf`.
- Commit: included in this touch-first roll-strafe control commit.

### 20:49 — Replace the rejected tilt gearbox with fixed-neutral zones

- Recorded the second hardware result: pitch direction was inverted and stateful rebasing caused delayed, unpredictable movement with unreliable stopping.
- Replaced latching/rebasing with one stable 18-tic neutral calibration and direct forward/stop/back position zones.
- Added hysteresis: fixed normal walking starts near 1.5 degrees and stops inside roughly 0.6 degrees; inverted the physically observed direction.
- Clarified that BOOTSEL remains unplug, hold BOOT, reconnect, release after about two seconds; holding through host detection was unnecessary.
- Both hybrid and fallback full-width builds pass. Hybrid `__end__=0x20049334`, leaving 224,460 zone bytes; UF2 SHA-256 is `03b9ec7f4bd00c8f375f217d765a71f3d6cf328eda4f76cbefab6767b2935866`.
- Commit: included in this fixed-neutral tilt-controls commit.

### 20:25 — Replace absolute tilt with a three-state movement gesture

- Recorded the first hardware result: touch turning felt responsive, double-tap opened doors, proportional tilt required too much angle, and held PWR conflicted with PMIC long-press/restart behavior.
- Replaced single-axis proportional tilt with XYZ sampling and signed X/Z gravity-vector rotation around the player's settled grip.
- Added a one-gesture-per-state reverse/stopped/forward controller at normal Doom walk speed; five stable tics rebase and prevent a continuous sweep from jumping through stop.
- Changed in-level PWR to immediate tap-fire only and removed all gameplay release/long-press behavior; Escape remains deliberately unresolved.
- Both hybrid and fallback full-width builds pass. Hybrid `__end__=0x20049338`, leaving 224,456 zone bytes; UF2 SHA-256 is `347c1484fe67ab3b338ed1b366fba25dfe4f49ffa8339e3f84ebef777c68eb35`.
- Commit: included in this stateful-tilt-controls commit.

### 20:02 — Build a hybrid motion-and-touch control prototype

- Added a compile-selectable hybrid model: QMI8658 pitch movement, proportional horizontal touch turning, touchscreen double-tap Use/Open, held PWR fire, and long-PWR Escape/menu.
- Added a minimal fixed-memory accelerometer driver at +/-2g and 62.5Hz, with 18-tic neutral calibration and fixed-point low-pass filtering; gyro remains intentionally disabled.
- Enabled AXP2101 PWRON press/release edge IRQs so fire no longer waits for the old 400ms single/double-click decision.
- Kept the floating swipe-and-hold model as the fallback and preserved its menu behavior. Both hybrid and fallback full-width Release builds pass.
- Measured the hybrid linker endpoint at `0x20049318`: 48 static bytes above baseline and 224,488 bytes of exact zone headroom. Hardware control validation is pending.
- Commit: included in this hybrid-control prototype commit.

### 19:47 — Lock full-width video at 35.2 FPS and install the normal build

- Recovered and checksum-validated the final one-minute report: 2,110 frames in 60.004 seconds, or 35.2 FPS.
- Measured average/max cadence 28,451/46,503us, presentation 22,226/27,049us, CPU preparation 20,709us, blocking display service 1,516/3,843us, display wait at most 11us, and zero DMA timeouts.
- Improved cadence by 18.3% and presentation by 23.2% versus the original 28.7 FPS synchronous full-width baseline, with unchanged static presentation memory.
- Replaced the profiler with the normal 448x280 asynchronous build, flashed and byte-verified as SHA-256 `c67fb48fc5ee27c5b348f5c532e316f06bc496b7981a09bd84460ef624d91920`; regular play no longer auto-reboots into reports.
- Commit: included in this locked-full-width-performance milestone commit.

### 19:38 — Clear 35 FPS in the paired-row hardware capture

- Recovered and checksum-validated the 20-second paired-row report: 727 frames in 20.009 seconds, or 36.3 FPS.
- Measured average/max cadence 27,560/43,794us and presentation 21,842/27,674us; CPU preparation fell to 20,404us while blocking display service stayed at 1,437us.
- Improved cadence by 2,104us and CPU preparation by 2,000us versus the 33.7 FPS pipeline baseline, with display wait at most 9us and zero DMA timeouts.
- Accepted the CPU optimisation directionally and selected one final 60-second capture as the video-performance lock gate.
- Commit: included in this paired-row hardware-result commit.

### 18:53 — Build a shorter paired-row CPU-packing experiment

- Added a selectable profiler duration so intermediate comparisons can take 20 seconds while final validation remains 60 seconds; changing it recompiles only the video source.
- Packed vertically duplicated output rows in one strided x loop when both remain in the same 20-row tile, removing repeated scaled-row loads and loop control without another buffer.
- Verified exact old/new row mapping at 384x240, 416x260, and 448x280; all normal/profiled synchronous and asynchronous configurations build with unchanged linker endpoints.
- Built the profiled 448x280 asynchronous candidate for a 20-second hardware comparison; UF2 SHA-256 is `d2f6ec21b94c2fbba57c4ff6a376c9e9bf8d56b21f462053bf59a96c0d374186`.
- Commit: included in this paired-row-packing candidate commit.

### 18:48 — Measure full-width asynchronous presentation at 33.7 FPS

- Recovered and checksum-validated the asynchronous one-minute combat report: 2,024 frames in 60.012 seconds, or 33.7 presented FPS.
- Improved average cadence from 34,838us to 29,664us (14.9%) and presentation from 28,934us to 23,940us, with zero DMA timeouts.
- Reduced blocking display service from 7,130us to 1,535us, hiding about 78% of the old transfer cost while retaining identical static-memory endpoints.
- Selected the remaining 22,404us CPU compose/scale/transpose work as the next target; about 1.1ms more reaches the 35 FPS simulation rate.
- Commit: included in this measured-asynchronous-performance commit.

### 18:43 — Hardware-validate the asynchronous screen and audio path

- Flashed and byte-verified the profiled 448x280 two-buffer candidate, then rebooted it into application mode.
- Alexander confirmed that the Doom menu is visible, updates normally, and sounds good; the previously stretched menu effects were not reported.
- Promoted the asynchronous presenter from build-only to hardware-valid for screen/menu/audio behavior. The one-minute combat timing comparison remains the next gate.
- Commit: included in this asynchronous-hardware-validation commit.

### 17:56 — Build a memory-neutral asynchronous full-width presenter

- Split the full-width presentation tile into two 20-row buffers when `DOOM_ASYNC_AMOLED=ON`; their combined 35,840 bytes exactly match the proven single 40-row buffer.
- Added paired packed-transfer start/wait APIs that retain display mutex and chip-select ownership while DMA runs, allowing core1 to pack the alternate buffer safely.
- Kept the synchronous 40-row presenter as the default and enforced the asynchronous candidate's memory-neutral 20-row configuration at CMake and compile time.
- Built normal and profiled 320x200/448x280 synchronous images plus normal and profiled 448x280 asynchronous images. Full-width linker endpoints remain exactly `0x200492e8` and `0x20049750`; hardware validation is pending.
- Commit: included in this asynchronous-presentation-pipeline commit.

### 17:43 — Interleave audio service with full-width display transfers

- Recorded Alexander's experience assessment: full-width gameplay was playable to slightly sluggish, while menu effects lagged and sounded stretched.
- Matched that symptom to the measured schedule: two 512-sample buffers cover 23.2ms, but full-width presentation averages 28.9ms and can take 35.2ms.
- Added a non-blocking mixer refill opportunity after each completed 40-row display transfer, without adding audio buffers, static memory, or queue latency.
- Built normal and profiled 320x200 and 448x280 images; linker endpoints are unchanged. Hardware listening remains pending.
- Commit: included in this interleaved-audio-service commit.

### 17:16 — Validate full-width combat at 28.7 FPS

- Flashed, byte-verified, and visually confirmed the hardware-proven 40-row presenter at full-width 448x280; Alexander confirmed the full-screen view rendered correctly.
- Recovered a checksum-valid one-minute combat report with 1,724 frames in 60.026 seconds, or 28.7 presented FPS.
- Measured average/max cadence 34,838/46,520us, presentation 28,934/35,225us, CPU preparation 21,803us, transfer 7,130/7,600us, render last/max 22,254/31,316us, display wait at most 9us, and zero DMA timeouts.
- Established that full width is already close to 30 FPS: removing about 1.5ms, or 4.3% of average cadence, reaches that target. CPU preparation remains the main target; DMA and panel reliability remain healthy.
- Commit: included in this measured-full-width-baseline documentation commit.

### 16:38 — Reject the black-screen small tile and move the next test to full width

- Flashed and byte-verified the instrumented 320x200 8-row candidate; its application USB returned but Alexander observed a black screen, so no performance capture was attempted.
- Restored 40 rows as the hardware-proven default while retaining other tile heights only as explicit driver experiments.
- Built the reliable 448x280 40-row profiler for the next hardware test; it links at `__end__=0x20049750` with 223,408 bytes of exact zone headroom.
- Clarified that 320x200 was used only to isolate one optimization against a measured baseline; subsequent experience testing moves to the full-width target.
- Commit: included in this failed-small-tile follow-up commit.

### 16:23 — Build the first cache-local transpose candidate

- Made transpose-tile height an explicit CMake comparison variable, retaining the original 40-row path while selecting 8 rows automatically for 320/384/448 and 10 for 416.
- Reduced the normal 320x200 tile from 25,600 to 5,120 bytes and moved `__end__` from `0x20046ae8` to `0x20041ae8`, increasing exact zone headroom from 234,776 to 255,256 bytes.
- Reduced the normal 448x280 tile from 35,840 to 7,168 bytes and moved `__end__` from `0x200492e8` to `0x200422e8`, increasing exact zone headroom from 224,536 to 253,208 bytes.
- Built normal and instrumented 320x200 and 448x280 Release images successfully. Hardware must still verify image correctness and whether shorter-stride writes outweigh the additional transfer setup.
- Commit: included in this selectable-small-transpose-tile commit.

### 16:18 — Capture the 320x200 one-minute combat baseline

- Recovered and checksum-validated the autonomous 100-byte report after Alexander completed the warm-up and one-minute combat run; the board returned to Doom without manual BOOTSEL handling.
- Measured 2,507 presented frames in 60.015 seconds (41.8 FPS), with average/max cadence 23,948/42,958us and average/max presentation 15,460/20,351us.
- Split average presentation into 11,664us CPU preparation and 3,795us panel transfer; core1 averaged 7,091us, frame wait averaged 1,367us, display wait peaked at 8us, and no DMA timeout occurred.
- Selected compose/scale/transpose locality and tile size as the first optimisation target. The healthy DMA/panel path does not justify a wholesale driver rewrite yet.
- Commit: included in this measured-combat-baseline documentation commit.

### 16:12 — Extend profiling to a true one-minute combat run

- Hardware-confirmed the first 320x200 flight recorder reboot/report path and recovered its 384-frame non-combat baseline from the reserved flash log.
- Replaced the frame-count cutoff with a 3-second level warm-up plus 60 seconds of wall-clock in-level capture so presentation rate cannot shorten the test.
- Persisted the checksum-validated report only after reboot, single-core and before USB/audio/Doom startup, in the reserved `0x101ff000..0x101fffff` sector immediately before the WHD at `0x10200000`.
- Built 320x200 and 448x280 Release profilers; their exact zone headroom is 233,648 and 223,408 bytes. The normal non-profile builds remain unchanged.
- Commit: included in this one-minute persistent-profiler commit.

### 15:55 — Replace live USB telemetry with a short flight recorder

- Identified duplicate TinyUSB initialisation as the concrete leading cause of the dead runtime CDC/reset interface: the reusable Waveshare module and Doom's entry point both called `stdio_init_all()` during one boot; hardware confirmation is pending.
- Moved stdio ownership to executable entry points and retained the profiling build's bounded 2ms USB output timeout.
- Replaced continuous gameplay serial dependence with a 384-frame real-level-only capture that checksum-saves full aggregate timing in reset-retained SRAM, watchdog-reboots, and repeats the report over quiet USB while cycling key values on the AMOLED.
- Built 320x200 and 448x280 flight-recorder images with ARM GNU 15.3. The profiler leaves 234,596 and 224,356 zone bytes respectively; hardware flashing and capture remain pending.
- Commit: included in this reset-persistent profiling-workflow commit.

### 15:30 — Build the first measured full-width video milestone

- Added compile-out timing instrumentation for game work, rendering, core rendezvous, presentation packing, AMOLED transfer time, presented-frame cadence, and display DMA timeout recovery.
- Reworked the presenter into one fused compose/scale/transpose path with selectable 320x200, 384x240, 416x260, and 448x280 outputs; scaled modes compose each Doom source row once and use exact division-free nearest-neighbour accumulators.
- Built every mode with the same Release toolchain. The normal 320x200 image retains the exact `__end__=0x20046ae8` baseline; full-width 448x280 leaves 224,536 bytes of short-pointer-zone headroom, only 10,240 bytes below baseline.
- Built instrumented 320x200 and 448x280 comparison images. Hardware flashing is pending because the long-running application currently enumerates its USB Reset interface but stalls both picotool and direct reset requests; the verified safe firmware remains installed.
- Commit: included in this performance-instrumentation and scalable-presentation commit.

### 14:50 — Set the enjoyable-handheld roadmap and verify autonomous flashing

- Reframed success from merely running Doom to an enjoyable, completable handheld experience.
- Added a measured performance-first roadmap toward full-width video, followed by horizontal-touch plus pitch-tilt and alternate motion-control experiments.
- Recorded the successful E1M1-to-E1M2 run, byte-identical SDK 2.3.0 rebuild, durable full-flash backup, verified `picotool -f` deployment, and normal post-flash operation.
- Documented later battery, port-settings, HUD, and optional music work without promoting them ahead of controls.
- Commit: included in this experience-roadmap documentation commit

### 08:03 — Prepare the safe milestone for GitHub

- Updated README and current project context to describe effects-only audio, the optional music backend, current memory figures, and removal of runtime BOOT input.
- Corrected the macOS flash command to suppress extended attributes and documented the safe pre-BOOT rollback as the published state.
- Reviewed the nine unpublished local commits for squashing into one coherent milestone before pushing `main`.
- Commit: included in the squashed playable-audio-and-controls milestone.

### 00:55 — Defer BOOT research and close on the safe firmware

- Documented that Raspberry Pi officially supports BOOTSEL runtime input, but found no exact-board Waveshare example and identified Doom's dual-core XIP workload as the important difference.
- Deferred any revisit to a standalone single-core, amplifier-disabled `flash_safe_execute()` experiment.
- Recorded the byte-identical safe UF2 hash and refreshed the session handoff; README remained unchanged.
- Commit: included in this BOOT-research documentation commit.

### 00:51 — Remove runtime BOOT input permanently

- Confirmed the pre-BOOT-polling effects-only build restored normal hardware behavior, while a minimal single-press BOOT variant again behaved abnormally.
- Removed BOOT sampling, Escape injection, and core1 lockout registration from the game firmware.
- Reserved BOOT exclusively for ROM BOOTSEL entry and moved Escape replacement back to the PWR/touch design backlog.
- Kept README unchanged under the publication-only documentation policy.
- Commit: included in this runtime-BOOT rollback commit.

### 00:36 — Simplify BOOT Escape to one press

- Replaced the 1.2-second BOOT hold with a two-sample-debounced press edge that emits one Escape pulse and never repeats while held.
- Recorded that this still uses runtime flash-CS sampling and does not eliminate that mechanism's risk.
- Paused deployment after the battery-free board emitted abnormal loud audio and a burning smell; physical inspection is required before it is powered again.
- Kept README unchanged under the publication-only documentation policy.
- Commit: included in this BOOT single-press control commit.

### 00:23 — Plan tilt controls and a measured performance refactor

- Confirmed the onboard QMI8658 makes calibrated accelerometer/gyro control feasible and reviewed comparative mobile-game input research.
- Preserved floating swipe-and-hold as Control Model A; planned full-tilt, hybrid tilt-plus-touch, and gyro-assisted variants with recentering and fixed-memory filtering.
- Planned instrumentation-first optimization followed by small, hardware-testable subsystem refactors with measured SRAM and timing deltas.
- Kept README unchanged under the publication-only documentation policy.
- Commit: included in this controls-and-refactor planning commit.

### 00:10 — Make README updates publication-oriented

- Recorded that routine local commits must not update README.
- Reserved README changes for major milestones or the final cleanup before an explicitly requested GitHub push.
- Kept granular local history in Scribe's project log/context/TODO and decision documents instead.
- Commit: included in this documentation-workflow commit.

### 00:04 — Map BOOT long-press to Escape safely

- Registered render core 1 with the Pico SDK's RAM-resident multicore lockout before allowing gameplay input.
- Sampled flash-CS-based BOOT at no more than 20Hz with a 2ms lockout timeout, then emitted one Escape pulse after a continuous 1.2-second hold.
- Updated controls, architecture notes, memory figures, project context, and follow-up hardware-test TODOs.
- Verified effects-only and optional-music builds; the final effects-only UF2 leaves 234,556 bytes in the short-pointer zone.
- Commit: included in this BOOT/Escape input commit.

## 2026-08-16

### 23:59 — Publish a complete project README locally

- Replaced the minimal README with a goal-first GitHub landing page following the Scribe documentation contract.
- Documented hardware-tested features and controls, WAD preparation, build/flash steps, architecture, memory constraints, repository structure, project history, limitations, and credits.
- Recorded the successful effects-only hardware check while keeping long-duration combat stability open.
- Commit: included in this README documentation commit.

### 23:51 — Record local-first Git workflow

- Added a permanent repository rule encouraging frequent recoverable local commits.
- Required explicit permission for every remote push and clarified that building or flashing does not imply pushing.
- Set squashing unpublished commits into one coherent milestone as the default pre-push workflow while protecting already-pushed history.
- Commit: included in this workflow-policy commit.

### 23:55 — Default to effects-only audio

- Confirmed on hardware that the shareware music data and fixed-memory MUSX playback path work end to end.
- Recorded that the lightweight synthesized timbre is not enjoyable through this device's small speaker.
- Kept the complete music backend behind `DOOM_ENABLE_MUSIC`, while making the normal build effects-only and excluding its music parser sources.
- Verified both configurations build; the final effects-only UF2 leaves 234,776 bytes in the short-pointer zone.
- Commit: included in this effects-only-default commit.

### 23:48 — Add fixed-memory MUSX music experiment

- Confirmed the shareware WAD and generated WHD contain the Doom music; it was not missing, only disabled behind the stub backend.
- Added a nine-voice integer MUSX synthesizer mixed through the non-blocking SFX DMA path.
- Replaced embedded MUSX file/iterator heap allocation with static storage and used a non-blocking cross-core music lock.
- Verified a release build with 233,448 bytes of short-pointer zone remaining; hardware listening and stability tests are pending.
- Commit: included in this music-experiment commit.

### 23:20 — Confirm asynchronous SFX on hardware

- Confirmed the sound-enabled firmware boots and Doom sound effects play correctly through the device speaker.
- Confirmed the absence of music is expected because `DEBUG_NO_MUSIC=1` still isolates the stub backend.
- Preserved this working SFX state as a stable milestone before music experiments.
- Commit: included in this hardware-confirmation commit.

### 23:12 — Re-enable sound effects through buffered DMA

- Replaced blocking per-sample PIO writes with a non-blocking two-buffer DMA/IRQ queue that emits silence on underflow.
- Serialized every ADPCM channel mutation across cores and removed obsolete sound bootlog traffic.
- Added saturating mixing, corrected pitch scaling, and enabled SFX while keeping unsupported music independently disabled.
- Verified a release build with 235,864 bytes of short-pointer zone remaining.
- Commit: included in this asynchronous-audio commit.

### 23:01 — Remove bootlog remnants from the game border

- Confirmed the white rectangle was the one-line on-screen boot diagnostic being repainted after the panel clear.
- Disabled normal bootlog rendering when graphics takes ownership, then clears the whole panel.
- Kept diagnostics enabled on every fresh boot so early failure and OOM reports still work.
- Commit: included in this display-cleanup commit.

### 22:52 — Restore pixel-exact video for an isolated freeze test

- Returned the centered game image from 448x280 scaling to native 320x200.
- Cut display traffic by 49%, reduced frame transfers from seven to five, and recovered 10,240 bytes of tile SRAM.
- Kept audio completely disabled; the blocking backend will be redesigned only after the combat freeze is isolated.
- Commit: included in this pixel-exact diagnostic commit.

### 22:41 — Harden display/audio drivers against stalls

- Confirmed the latest combat freeze was not normal zone OOM.
- Disabled all mixing/output work when `DEBUG_NO_SOUND` is active.
- Added bounded AMOLED DMA wait with abort/FIFO-clear/PIO restart recovery.
- Made shared module and QSPI PIO initialization idempotent.
- Commit: included in this driver-hardening commit.

### 22:31 — Optimize scaled output and persist OOM evidence

- Replaced per-output-pixel 5/7 integer division with an equivalent incremental scaler.
- Made zone OOM automatically reboot into a persistent allocation-size/free-byte report.
- Preserved the improved swipe controls; hardware tuning remains open.
- Commit: included in this diagnostics commit.

### 22:23 — Add floating swipe-and-hold movement

- Researched floating joysticks, anywhere-on-screen gestures, dead zones, and axial bias.
- Preserved and documented the original four fixed hold zones behind a compile-time selector.
- Implemented touch-anywhere anchoring, 24px dead zone, direction hold/change, and release-to-stop.
- Commit: included in this control-experiment commit.

### 22:20 — Stabilize touch, display memory, and Doom zone

- Debounced touch-zone transitions and removed blocking per-touch AMOLED diagnostics.
- Replaced the 128KB rotated presentation buffer with a 35KB tiled 448x280 scaler/transposer.
- Cleared the previously uninitialized letterbox bands and expanded the short-pointer zone by 64KB.
- Firmware builds and boots; extended gameplay lasts substantially longer, with memory stability still under test.
- Commit: included in this milestone commit.
