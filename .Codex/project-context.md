# Project Context

## Summary

DOOM runs on the Waveshare RP2350-Touch-AMOLED-1.8 and is playable with pointing-finger movement plus the PWR button. The product goal is an enjoyable, completable handheld experience rather than proof that the port boots. Full-width 448x280 gameplay sustains 35.2 presented FPS over the complete one-minute combat route, up from 28.7 FPS, without extra static presentation memory. F11 is flashed: deterministic corner strafe is a modest improvement, while continuous tilt control is rejected. Runtime BOOT polling remains forbidden.

## Active goals

- Establish repeatable frame/tic/memory/audio/input measurements on the proven baseline.
- Preserve the locked 448x280 baseline and optionally measure 448x336 traditional 4:3 correction before deciding whether smaller bars justify the cost.
- Make movement and turning enjoyable enough for sustained combat and full-game progression.
- Confirm extended-play stability through repeated E1M1-to-E1M2 runs.
- Preserve the hardware-verified asynchronous sound-effect path while testing extended gameplay stability.
- Keep effects-only audio as the device default while preserving music as an optional experiment.
- Audit remaining essential actions, especially weapon selection and Escape/menu, before further control tuning.
- Measure memory and timing first, then refactor subsystems in small hardware-testable stages.

## Key decisions made

- Keep the measured 448x280 raw-16:10 presentation locked; treat 448x336 traditional 4:3 correction as an optional measured experiment, not an assumed upgrade.
- Keep packed transpose tiles because SH8601 MADCTL has no row/column exchange; the asynchronous two-buffer 20-row presenter is hardware-validated and the synchronous path remains a recovery fallback.
- Place the RP2350 short-pointer window at `0x20040000..0x20080000`, ending at the linker stack limit.
- Never read BOOT during normal gameplay; reserve it exclusively for entering BOOTSEL during power-on/reset.
- Defer any BOOT-as-input revisit to an isolated single-core, audio-disabled SDK experiment; it is not part of the playable firmware roadmap now.
- Commit meaningful changes locally, never push without an explicit request, and normally squash unpublished commits into one milestone before pushing.
- Keep README updates publication-oriented: major milestones or final pre-push cleanup only, not routine local commits.
- Do not use continuous tilt for an essential action; repeated hardware tests made its neutral and activation too difficult to trust.
- Optimise performance before fine-tuning motion controls so display latency does not contaminate input evaluation.
- Treat full-width 448x280 as an ambition; select the largest mode that remains smooth, responsive, stable, and memory-safe.
- Keep performance instrumentation compile-time optional and bounded; measurements must not become a new gameplay dependency.
- Prefer a coherent experience over feature count: effects stay, poor-quality music stays off, and later HUD/battery/settings additions must justify their cost.

## Current state

- Game boots, renders, loads E1M1, and supports combat.
- Touch boundary chatter is filtered and no longer triggers blocking screen logs.
- The first hybrid hardware test confirmed responsive/enjoyable horizontal touch turning and working double-tap Use/Open. It rejected proportional absolute tilt as requiring too large an angle, and rejected PWR hold because the PMIC long event opened the menu and a longer hold appeared to restart the game.
- The second hybrid hardware test rejected the stateful gearbox: forward was inverted, transitions were delayed/unpredictable, and movement did not stop dependably because settled poses continually became new references.
- The third pitch candidate was physically rejected: it could move forward but the narrow start/stop region still produced accidental movement, difficult stopping, and occasional reversal.
- The touch-plus-roll F1 test confirmed that touch navigation feels better, but turning remained too fast and roll produced uncommanded left/right movement with no finger down. Roll is rejected for the active model.
- F2 performs no IMU initialisation or polling. Touch forward/back is unchanged; turning maximum is reduced from 960 to 640, its dead zone grows from 8 to 10px, and full scale moves from 96 to 120px.
- F2 adds 32 static bytes over the locked full-width baseline, leaving 224,504 bytes of zone headroom; hardware validation is pending.
- F2 was the easiest physical playthrough so far, but the 120px horizontal range required too much finger movement and obscured the view.
- F3 keeps the floating anchor and 640 maximum turn rate but compresses full horizontal response to 56px with an 8px dead zone and cubic curve; all other controls remain unchanged.
- F3 remained playable through E1M1 but under-responded to short combat swipes. F4 keeps 56px/640, changes to a 6px dead zone, 80 minimum, and quadratic response; forward/back remains accepted and unchanged.
- F4 improved left/right responsiveness, but exposed the unchanged vertical control as a delayed binary switch: 10px of no response followed by fixed 25-unit walking. F5 leaves turning and diagonal composition unchanged, reduces the vertical dead zone to 4px, and ramps linearly from 8 to 50 movement units by 44px.
- F5 builds in the locked full-width asynchronous configuration with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; its UF2 SHA-256 is `749e7fea1b0fea663d797b07135c2b89e543dfe0cec37f422f7cabee78c4b13f`.
- F5 made forward/back progressive, but its hardware test rejected the remaining 4px vertical and 6px horizontal dead zones as delayed and then aggressive. F6 uses a one-pixel jitter guard on both axes, lowers initial output to 4 movement/48 turn units, and preserves established mid/high response and maximum speeds.
- F6 builds in the locked full-width asynchronous configuration with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; its UF2 SHA-256 is `66f33d5c59e809047c9e8748184fa3a320d28fe33f3f34c99ca55508e40c5efe`.
- F6 still needed roughly a centimetre of physical travel, impossible to explain with its one-pixel software guard. F7 fixes the shared driver: point mode now selects FT3168 Active rather than Monitor mode and reads finger/X/Y together from `0x02..0x06`; all mappings remain unchanged.
- F7 made subtle pointing-finger motion responsive, validating the driver direction, but the 44px movement and 56px turn ranges reached aggressive maximums too easily. A broad thumb remained less reliable; contact-centroid stability is the leading inference, not yet a measured cause. F8 doubles full-scale distance to 88px movement and 112px turning while retaining immediate response and the same maximums.
- F8 builds in the locked full-width asynchronous configuration with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; its UF2 SHA-256 is `7f8f3e3df65668e23cbb0bf0579b42b4be83f933c6313d45fdf388188e9a8dfc`.
- F8 confirms the pointing finger as the intended relative-control contact. Small turning feels good, but large turns are too difficult. F9 keeps the 1px/48-unit quadratic start and 112px range, raising only maximum turn from 640 to 960; movement is unchanged.
- F9 builds in the locked full-width asynchronous configuration with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; its UF2 SHA-256 is `b8e94eebb5310750031883db351b14e019e67703e17c72d826726b6d19b97820`.
- F9's pointing-finger mapping is accepted as usable. F10 keeps it unchanged and enables proportional accelerometer roll only while touch is held. No touch guarantees zero strafe and learns neutral; active touch freezes the reference. Entry is about 10 degrees for two tics, stop is about 5 degrees, and output scales 8-to-32 by roughly 22 degrees.
- F10 builds in the locked full-width asynchronous configuration with `__end__=0x20049330`, leaving 224,464 zone bytes; its UF2 SHA-256 is `af30c8362cb6cd837bedb560e71998057f7f577a40e6fc28c5e9174c04777816`.
- F10 physically strafed but its touch gate was unintuitive and could arm movement when touch began after a grip change. Always-on roll would restore earlier drift, so continuous motion strafe is rejected for the active UX.
- F11 compiles the IMU out, maps double taps in the bottom-left/right 96x72px corners to six-tic 32-unit strafe bursts, and keeps double-tap Use elsewhere. Forward/back becomes quadratic over 140px instead of linear over 88px; F9 turning is unchanged.
- F11 builds in the locked full-width asynchronous configuration with `__end__=0x2004930c`, leaving 224,500 zone bytes and no QMI8658 input symbols; its UF2 SHA-256 is `ec00e1262a4a3f53f93c9cb09cd6fc49b4e39ef4d9fe2ae820f30d1f1eba2cd6`.
- F11 was flashed and its corner strafe was judged better than having no strafe, but not a finished interaction; detailed burst and movement-curve tuning is deferred.
- The landscape panel is 448x368. Current 448x280 output leaves 44px bands above/below. A 448x336 4:3-corrected candidate would leave 16px bands and emit 20% more pixels; full 448x368 would emit 31.4% more and require stretch, crop, or renderer/UI redesign.
- A future Wolfenstein 3D/Spear of Destiny port plus a two-game launcher is plausible but should begin as a separate engine/firmware-slot feasibility project using the shared board drivers.
- The original four fixed hold zones remain available via a compile-time selector.
- Video defaults to centered native 320x200, but 384x240, 416x260, and full-width 448x280 are now selectable Release configurations behind the same fused compose/scale/transpose presenter.
- The optional profiler now waits through a 3-second user-controlled-level warm-up, captures 60 seconds by hardware time, checksum-saves aggregate cadence/core/render/packing/transfer/wait data in reset-retained SRAM, then watchdog-reboots. Before USB/audio/Doom start, the single-core report boot persists 100 bytes to reserved flash sector `0x101ff000`; the WHD begins at `0x10200000`. The host reads this record through `picotool`, so gameplay does not depend on live CDC output.
- The checksum-valid 320x200 one-minute combat baseline captured 2,507 frames in 60.015 seconds (41.8 presented FPS): average/max cadence 23,948/42,958us, core1 work 7,091/22,905us, frame wait 1,367/7,047us, presentation 15,460/20,351us, CPU preparation 11,664us, transfer 3,795/4,197us, render last/max 12,457/26,048us, display wait at most 8us, and zero DMA timeouts.
- CPU compose/scale/transpose preparation consumes 48.7% of average frame time and 75.4% of presentation time. Panel transfer is only 15.8% of average frame time, so the next work targets packing locality and tile size before considering a wholesale display-driver rewrite.
- The first focused candidate made transpose-tile height selectable. Although 8 rows recovered 20,480 bytes at 320, its first physical boot produced a black panel, so it is rejected as a playable default. The hardware-proven 40-row path is restored as default, and a 448x280 one-minute profiler is built with 223,408 bytes of zone headroom for the next user-facing test.
- The checksum-valid 448x280 one-minute combat capture completed 1,724 frames in 60.026 seconds (28.7 presented FPS): average/max cadence 34,838/46,520us, presentation 28,934/35,225us, CPU preparation 21,803us, transfer 7,130/7,600us, render last/max 22,254/31,316us, display wait at most 9us, and zero DMA timeouts. Full width is already near 30 FPS and needs about 1.5ms, or 4.3% of average cadence, removed to cross that threshold.
- Alexander judged the original full-width gameplay playable to slightly sluggish, but menu sound effects lagged and sounded stretched. The two 512-sample audio buffers cover only 23.2ms at 44.1kHz, less than the measured 28.9ms average and 35.2ms maximum presentation, so the presenter now gives the non-blocking mixer a refill opportunity after each completed display transfer. The fix adds no buffers or static memory, and Alexander confirmed the resulting menu audio sounds good on hardware.
- The hardware-validated asynchronous presenter splits the 35,840-byte full-width tile into two 20-row buffers. Its one-minute combat report captured 2,024 frames in 60.012 seconds (33.7 FPS): average/max cadence 29,664/46,151us, presentation 23,940/29,114us, CPU preparation 22,404us, blocking display service 1,535/4,078us, display wait at most 9us, and zero DMA timeouts. It improves cadence by 5,174us (14.9%) and hides about 78% of the previously blocking 7,130us display time without changing linker endpoints.
- The paired-row CPU optimisation passed its final checksum-valid one-minute capture: 2,110 frames in 60.004 seconds (35.2 FPS), average/max cadence 28,451/46,503us, presentation 22,226/27,049us, CPU preparation 20,709us, blocking display service 1,516/3,843us, display wait at most 11us, and zero DMA timeouts. Versus the original synchronous full-width build, cadence improves 18.3% and presentation 23.2% while normal zone headroom remains 224,536 bytes.
- The normal, non-profiling 448x280 asynchronous build is flashed and byte-verified (UF2 SHA-256 `c67fb48fc5ee27c5b348f5c532e316f06bc496b7981a09bd84460ef624d91920`); it no longer auto-reboots after a minute of play.
- All four output modes build. Normal 320x200 retains `__end__=0x20046ae8` and 234,776 zone bytes; full-width 448x280 links at `__end__=0x200492e8` with 224,536 zone bytes.
- OOM now watchdog-reboots into a persistent `OOM req=... free=...` report.
- Latest freeze produced no OOM reboot, ruling out ordinary zone exhaustion.
- Driver hardening did not eliminate the freeze; disabled audio does zero work and display DMA stalls time out and reset PIO.
- Boot diagnostics turn off when game graphics take over, leaving a clean border while preserving next-boot OOM reporting.
- Sound effects are enabled through a two-buffer DMA/IRQ I2S queue; all ADPCM channel mutations are serialized across cores.
- Hardware confirms the game boots and its sound effects play correctly with the asynchronous backend.
- The shareware WAD and generated WHD contain music, and the nine-voice fixed-memory MUSX synthesizer plays it successfully on hardware.
- The lightweight chiptune rendering is not enjoyable through the small speaker, so `DOOM_ENABLE_MUSIC` defaults to `OFF`; the full backend remains available for later experiments.
- The latest effects-only deployment was hardware-confirmed with working SFX, no music, and normal immediate gameplay behavior.
- A source-unchanged rebuild using Pico SDK 2.3.0 and ARM GNU 15.3 matched the installed firmware byte-for-byte, flashed and verified through `picotool -f`, and rebooted without a physical BOOT press.
- Removing duplicate `stdio_init_all()` calls hardware-restored autonomous application-to-BOOTSEL reset and return. CDC text remains unavailable, so performance reports use the reserved flash record instead of a live terminal.
- The complete verified flash backup is durable at `../device-backups/rp2350-doom-working-2026-08-17.bin` (SHA-256 `58aef7f97a624a378cd3a6edd0ba47377852113c1351f16ff25dae90b152cb43`).
- Alexander then confirmed normal operation and reported reaching E1M2 without a freeze.
- Both long-press and single-press runtime BOOT builds produced abnormal hardware behavior; the single-press experiment has been removed from the main firmware.
- Returning to the pre-BOOT-polling effects-only firmware restored normal operation, making runtime BOOT access the common suspect even though the exact electrical/software mechanism is not proven.
- The safe UF2 is byte-identical to the verified pre-BOOT build: SHA-256 `0fa5d343884f25a2c9a99aeea84177eb2014417d5b4cdb0f8f26dd2e27a4f1e2`.

## Open questions

- Can repeated E1M1-to-E1M2 runs reproduce the earlier combat freeze, or can it be downgraded from an active blocker?
- Does the final normal full-width build remain stable through repeated E1M1-to-E1M2 runs now that performance profiling is locked?
- Does asynchronous SFX remain stable during a longer sustained-combat test?
- Can the optional MUSX synthesizer's timbre be improved enough to suit the small speaker?
- What non-BOOT, non-PWR-hold gesture should eventually provide Escape/menu?
- Are F11's left/right corner bursts correctly directed and long enough to dodge without overshooting?
- Is the 140px quadratic movement curve calm enough in the middle while keeping far-travel running reachable?
- Does immediate tap-only PWR fire remain reliable during combat without accidental physical holds?
- Which weapon-change gesture is fast, deliberate, and compatible with move/turn, corner strafe, and Use?
- Does a measured 448x336 build remain smooth enough to justify reducing the black bands from 44px to 16px each?
- Can Doom and a Wolf3D/Spear port fit as independent application/data slots within the 16 MiB flash, selected by a small launcher?
