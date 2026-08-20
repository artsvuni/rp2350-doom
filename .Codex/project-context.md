# Project Context

## Summary

DOOM runs on the Waveshare RP2350-Touch-AMOLED-1.8 and is playable with asymmetric thumb controls plus PWR and BOOT. The product goal is an enjoyable, completable handheld experience rather than proof that the port boots. F18 is installed and hardware-accepted as the core experience: full-panel 448x368, fixed thumb zones and guides in gameplay and menus, in-zone Use, score-screen progression, PWR Fire/Escape, BOOT tap next weapon, and BOOT-hold strafing. F18 deliberately stretches exact 4:3 by 9.5% vertically; F17 448x336 remains the exact-4:3 rollback, 448x320 the second visual rollback, and 448x280/35.2 FPS the measured performance rollback.
Its checksum-valid one-minute combat baseline is 34.3 presented FPS with zero
DMA timeouts. Exact F18 is restored after two rejected save attempts. A revised
save-only candidate now fixes both the pre-write core1 pause rendezvous and the
later flash lockout; it is installed and successfully created a save on
hardware, and Alexander subsequently loaded existing slots 1 and 2
successfully. Overwrite and explicit post-reboot persistence remain, while
exact F18 is still the immediate rollback. The installed follow-up deliberately
avoids
RTC integration and names each slot in the format `SAVED GAME 1 - HANGAR`,
where the digit is the selected slot; its flash write was verified, but the
label has not yet been physically checked in the menu. The normal firmware now
includes the hardware-accepted lightweight music backend while starting Doom's
Music Volume at zero, so the preferred default remains effects-only and a
player can opt into music through Options. The final score keeps mastering and
the rejected smooth profile off and reduces General MIDI percussion to 50% of
its original gain. At the default zero volume, the generator is detached so
music keeps its 1.3 KiB fixed SRAM cost but performs no continuous synthesis or
silent buffer production. Optimised image `e9716825...` is installed and
flash-verified; its short audible interaction check remains.

## Active goals

- Establish repeatable frame/tic/memory/audio/input measurements on the proven baseline.
- Preserve 448x336, 448x320, and measured 448x280 rollback artifacts.
- Make movement and turning enjoyable enough for sustained combat and full-game progression.
- Confirm extended-play stability through repeated E1M1-to-E1M2 runs.
- Preserve the hardware-verified asynchronous sound-effect path while testing extended gameplay stability.
- Keep sound effects on and Music Volume at zero by default while compiling the
  accepted lightweight score into normal firmware for menu-based opt-in.
- Treat weapon selection and Escape/menu as solved essential actions; audit only genuinely missing controls before adding more vocabulary.
- Treat F15's inherited F14.1 thumb geometry and visible guides as the navigation baseline.
- Observe F15 through longer combat sessions before considering the BOOT safety/stability question fully mature.
- Preserve F18 as the combined controls/display baseline; refine sensitivity,
  guides, or zone geometry only in isolated future comparisons.
- Hardware-test the reversible 30px edge-zone candidate after the installed
  28px LEFT/BACK version received a positive initial physical assessment.
- Keep the hardware-accepted PWR quit/power-off path; only hold-to-cancel needs
  an explicit regression check.
- Measure memory and timing first, then refactor subsystems in small hardware-testable stages.

## Key decisions made

- Accept full-panel 448x368 as the core experience because its hardware feel
  outweighs strict aspect correction on this small display. Continue to state
  that it stretches exact 4:3 by 9.5% vertically.
- Keep F17 448x336 as exact-4:3 rollback and measured 448x280/35.2 FPS as the
  performance rollback.
- Keep packed transpose tiles because SH8601 MADCTL has no row/column exchange; the asynchronous two-buffer 20-row presenter is hardware-validated and the synchronous path remains a recovery fallback.
- Place the RP2350 short-pointer window at `0x20040000..0x20080000`, ending at the linker stack limit.
- Accept F16 BOOT tap/hold only in local play: tap cycles weapon, hold modifies
  LEFT/RIGHT to strafe. Double-click, menu, and network behavior remain disabled.
- Use the fixed absolute thumb zones as the common interaction language in
  gameplay and menus; translate them to each Doom state's native commands.
- Advance intermissions through native Fire, never a custom level shortcut,
  and require a fresh touch release after level exit before touch can do so.
- Require staged admission for flash-CS input: isolated probe, silent Doom, then effects-enabled Doom; all three passed for F15.
- Treat flash-backed audio DMA as the previously missed hazard: `silence_buffer` links in XIP and must move to SRAM before any BOOT-enabled Doom test.
- Runtime BOOT must use the SDK `flash_safe_execute()` contract, register core1 with `flash_safe_execute_core_init()`, sample only in a single-player level, and map one debounced release to Doom's existing next-weapon path. Do not add hold/double actions.
- Runtime save writes must use the same SDK multicore lockout. First rendezvous
  with core1 after its previous presentation has completed, retain all active
  DMA sources in SRAM, execute only the erase/program callback from SRAM, and
  abort rather than writing if the lockout cannot be established.
- Keep handheld save labels simple and deterministic: `SAVED GAME <slot> -
  <level>`, truncated within the original header. Do not add RTC setup or
  change the save format merely to timestamp a slot.
- On embedded Y/N prompts only, translate menu-forward/Enter to Yes. This lets
  short PWR confirm while the established PWR hold/Escape path cancels; leave
  conventional desktop Doom unchanged.
- Embedded Quit Game must power off through the AXP2101 rather than enter the
  inherited permanent ENDOOM loop. Arm a watchdog restart before the PMIC
  command so failure cannot strand the device on another inert screen.
- Commit meaningful changes locally, never push without an explicit request, and normally squash unpublished commits into one milestone before pushing.
- Keep README updates publication-oriented: major milestones or final pre-push cleanup only, not routine local commits.
- Do not use continuous tilt for an essential action; repeated hardware tests made its neutral and activation too difficult to trust.
- Optimise performance before fine-tuning motion controls so display latency does not contaminate input evaluation.
- Treat full-width 448x280 as an ambition; select the largest mode that remains smooth, responsive, stable, and memory-safe.
- Keep performance instrumentation compile-time optional and bounded; measurements must not become a new gameplay dependency.
- Prefer a coherent experience over feature count: effects stay, poor-quality music stays off, and later HUD/battery/settings additions must justify their cost.
- Ship the accepted lightweight MUSX score at 50% percussion gain, with
  mastering and smooth synthesis off. Compile it into the normal firmware but
  initialise on-device Music Volume to zero; Options can enable it for the
  current boot without a separate firmware image. Keep the generator detached
  at zero so muted music has no continuous CPU cost.
- Do not apply undocumented codec EQ coefficients. The lab may isolate global
  codec DRC, but any winner must be retested with the accepted sound effects.
- Keep the standalone music lab asset-free: it reads the user's existing WHX
  and uses the real MUSX, mixer, DMA, and ES8311 path without gameplay/rendering.

## Current state

- Game boots, renders, loads E1M1, and supports combat.
- Existing save slots 1 and 2 load successfully on hardware; overwrite and an
  explicit reboot-persistence check remain.
- The first `TAP PWR TO QUIT` test confirmed the PWR-to-Yes mapping but exposed
  the inherited bare-metal `I_Quit()` dead end: it entered a permanent
  ENDOOM/text-screen loop and appeared frozen. A corrected `Press PWR to
  quit.` candidate stops audio, requests documented AXP2101 software-off, and
  uses a one-second watchdog restart fallback. Exact-wording UF2 `9ae33128...`
  is now installed and flash-verified. Alexander confirmed the connected-USB
  quit path works and no longer freezes. Battery-only shutdown also passed;
  hold-to-cancel remains a small physical check. The earlier reported
  non-shutdown cannot be attributed to this candidate because it preceded this
  verified install.
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
- F12 preserves F11 and adds a 450ms software-timed PWR hold for Escape. Tap-fire remains immediate and one-shot; after Escape, PWR input is suppressed through release to prevent accidental menu selection. The AXP2101 PMIC configuration is unchanged.
- F12 was flashed and hardware-confirmed in-level. F12.1 resolves PWR on release so a hold never fires/selects first: short release emits Fire or Enter by context, while 450ms emits Escape/Back and suppresses release. Hybrid PWR double-click is replaced by hold; fallback is unchanged.
- F12.1 builds in active and fallback configurations. The installed active image ends at `0x2004930c`, leaving 224,500 zone bytes; its UF2 SHA-256 is `ff0cb48361fdcb364912f67fe502eab45dd5be230a70223d6b971abb843cd107`.
- F12.1 was installed and judged very comfortable for Fire and Escape in gameplay and menus. Very rapid click-click could collapse into one shot because adjacent key-up/key-down events lacked a fully sampled low tic.
- F12.2 queues up to four same-context tap pulses and emits them with one low tic between, while flushing the queue before Escape. Its physical test still lost Alexander's fastest click-click, proving the virtual pulse boundary was not the only loss point.
- The AXP2101 exposes PWR events as latched occurrence flags rather than counters, and Key1 is connected to PMIC PWRON rather than a raw RP2350 GPIO. F12.3 preserves a second press when a known first release and next press arrive in the same status mask. Active and fallback builds pass; active `__end__=0x2004930c`, headroom 224,500 bytes, UF2 SHA-256 `ae1d4d7626ff3f3c4549d4df1bbcc3aa3ae37624a971b60a0cb457af760089b5`.
- F12.3 produced no practical pistol improvement. Doom checks the pistol again only after its 14-tic recovery, so one-tic Fire pulses during recoil are intentionally lost. Preserve vanilla cadence rather than buffering delayed shots.
- F13 adds default-off `DOOM_TOUCH_DPAD`: an immediate fixed 160x160 bottom-left eight-way pad with a 12px neutral centre and normal/fast radial response. Its square excludes double-tap gestures so repeated direction taps cannot become accidental actions; Use and right strafe remain outside it. It adds 48 text bytes, no data/BSS, retains 224,500 bytes of zone headroom, and has UF2 SHA-256 `89ce0216629d17c76bc05bd16cbab0fe392c105b3ddc879e2394717d62ce4b5b`. The default pointing-finger UF2 remains byte-identical.
- F13's hardware test was positive: absolute touch-and-hold felt like traditional keyboard Doom, gave more control, and let a broad thumb work where the relative mapping preferred a pointing finger. This selects fixed-position input for further tuning without yet selecting the radial geometry itself.
- F14 adds default-off `DOOM_TOUCH_DPAD_THUMB_ZONES`: contiguous logical zones matching the sketch over `[0,340)x[70,368)`. LEFT is `[0,24)`, UP `[24,136)`, RIGHT `[136,340)`, and DOWN owns the bottom `[304,368)` across the full 340px control width. Release is the only neutral. Two 12px vertical transition bands compose forward+left/right; backward diagonals and the radial speed step are absent.
- `DOOM_TOUCH_DPAD_OVERLAY` adds temporary dim outlines and an amber active-zone outline by touching only packed boundary pixels. It allocates no framebuffer. The candidate ends at `0x20049310`, leaving 224,496 zone bytes; UF2 SHA-256 is `8b2307eee78b403cbe592c5ea51cf2dc37f8fd25692d2c5ed51e65983b90ba61`. The F13 radial and pointing-finger UF2s remain byte-identical.
- F14's physical test was the best control experience so far. LEFT/UP/RIGHT/DOWN, release-to-stop, and both forward-turn transition bands worked; diagonals could be refined but are not a blocker. The guides materially improved discoverability and remain enabled for extended testing. The only regression was that the control area suppressed Use double-taps.
- F14.1 permits the same bounded stationary double-tap recogniser inside every F14 zone and resolves it to Use/Open. Immediate movement remains on both touch-downs, so the gesture can produce two tiny directional pulses but adds no general movement delay. It retains `__end__=0x20049310`, 224,496 zone bytes, and has UF2 SHA-256 `821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`; F13 and pointing-finger fallbacks remain byte-identical.
- F14.1's in-zone Use/Open double tap is hardware-confirmed and works well.
- The current edge-zone candidate keeps F18 behavior and the visible/tappable
  geometry coupled. The installed 28px LEFT/BACK version felt nice overall.
  The new candidate moves both to 30px, producing LEFT 30x268, UP 106x268,
  RIGHT 204x268, and DOWN 340x30. Build `41d59b39...` retains the same static
  memory, is installed and flash-verified, and has returned to application
  USB; physical confirmation is pending.
- The completed BOOT audit found the board switch is a designed 1 kΩ pull-down on flash CS, not a supply short, but the old Doom lockout missed a real flash reader: audio DMA's empty-queue `silence_buffer` is at XIP address `0x1005a1ec`. This is a plausible abnormal-audio mechanism, not proof of the prior odour's cause.
- `boot_safety_probe` passed its one-press hardware gate: the audio/display-disabled black-screen image detected a brief press/release, entered ROM BOOTSEL, and produced no unexpected audio. The exact F14.1 image was restored and rebooted afterward.
- Gate B is built behind default-off `DOOM_BOOT_NEXT_WEAPON`. It registers core 1 with the SDK flash-safe coordinator, samples only in a local level, ignores failed 2 ms safety handshakes, and queues one native forward weapon cycle after two stable press and release samples.
- Gate B never initializes codec, audio PIO, or audio DMA; amp GPIO19 starts low and I2S GPIO20-24 stay high-impedance. The 2 KiB silence block moves from XIP to SRAM only in this candidate. `read_bootsel_raw=0x200008c4`, `silence_buffer=0x20046880`, `panel_chunks=0x2003d3a0`, zone headroom is 221,252 bytes, and UF2 SHA-256 is `891d6076db58253064dba38c4634322ac6109095915737243f38852de7d36076`.
- Silent Gate B passed on hardware: one brief BOOT release changed weapon once and Alexander reported no abnormal behaviour. Exact F14.1 was restored and flash-verified immediately afterward.
- F15 enables the separate sound-effects gate. Its active audio sources (`audio_buffers=0x2001deb8`, `silence_buffer=0x20046ca8`), display tiles (`0x2003d7c8`), BOOT callback (`0x200008c4`), and audio IRQ (`0x20001858`) are all SRAM-resident. It leaves 220,184 zone bytes; installed UF2 SHA-256 is `769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`. Normal SFX and weapon cycling passed together on hardware.
- F15.1 adds default-off BOOT hold interpretation without increasing the 25 ms
  sampling rate or keeping flash suspended. A short release remains next
  weapon; after about 300 ms physical hold, LEFT/RIGHT and the two forward
  transition bands become sustained 32-unit strafe. Release suppresses weapon
  cycling and restores turning. The candidate leaves 220,176 zone bytes and
  has UF2 SHA-256
  `6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`.
- F15.1's bounded hardware test passed and the image is accepted as F16.
  Alexander reported short weapon cycling, held left/right strafe, and
  release-to-turn all worked great with no abnormal behavior.
- The landscape panel is 448x368. Current 448x280 output leaves 44px bands above/below. A 448x336 4:3-corrected candidate would leave 16px bands and emit 20% more pixels; full 448x368 would emit 31.4% more and require stretch, crop, or renderer/UI redesign.
- A selectable 448x320 stepping-stone now builds with 24px bands and 14.3%
  more output pixels. It retains the proven two 20-row buffers and has exactly
  the same text/data/BSS totals, `__end__=0x20049bf0`, and 220,176 zone bytes as
  F16. Candidate hash is
  `a253683ef45e8e412bcfb35e6a6c1884972f21cd39b653cb15945fcbd5fa8170`;
  its short image/audio/gameplay gate passed and Alexander judged it excellent.
  AUTO rebuilt exact F16.
- The 448x336 candidate is the exact square-pixel equivalent of Doom's intended
  4:3 CRT presentation. Its final 16 image rows use a padded 20-row transfer
  with four cleared border rows. It adds 56 flash bytes, no SRAM, retains
  `__end__=0x20049bf0` and 220,176 zone bytes, and has hash
  `2a9f7a4ed74392e016fd2407fac1981aa1fb4c8e02c7d9724e0d25a31a913ad8`.
  Its physical gate passed: image correct, visually good, and no perceptible
  smoothness loss versus 448x320. It is now the preferred visual baseline.
- The accepted input path was incorrectly gated to an active level. F17 routes
  the same fixed zones to menu arrows, maps fresh intermission touch and PWR to
  native Fire, and requires touch release on score-screen entry. At 448x336 it
  also moves DOWN from physical y=304 to y=332 so only 20 pixels overlap the
  image, with the 16-pixel black border extending its edge target. The build
  retains `__end__=0x20049bf0` and 220,176 zone bytes. Its physical test passed:
  menu routing, score progression, and shallow DOWN are fixed. Alexander liked
  the accidental menu swipe enough to retain it as a future menu-only A/B test.
- F18 scales the whole Doom frame to 448x368. Its partial eight-row tail is
  assembled into an overlapping 20-row transaction using only the existing
  two buffers. The one-time panel clear now also avoids its former 8-row final
  stripe, the leading cause hypothesis for the changing coloured top-edge
  remnant. F18 retains `__end__=0x20049bf0`, 220,176 zone bytes, and has UF2
  SHA-256 `5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`;
  it was flashed, byte-verified, and hardware-accepted. Alexander loves the
  full-panel result and reported no observable smoothness regression. It is
  now the core presentation and combined experience baseline.
- The optional F18 profiler captured 2,059 frames in 60.021 seconds (34.3 FPS):
  average/max cadence 29,164/45,612us, presentation 24,469/31,064us, CPU
  preparation 22,570us, blocking transfer service 1,899/4,312us, frame wait
  1,350/3,808us, display wait at most 10us, and zero DMA timeouts. This is only
  a 2.5% cadence regression from 448x280/35.2 FPS while emitting 31.4% more
  pixels. The normal F18 firmware was restored after capture.
- Two keyboard-free save attempts froze after a slot was selected. Readback of
  the final 64 KiB proved it remained erased, correcting the first diagnosis:
  neither attempt reached the flash writer. `G_DoSaveGame()` runs inside
  `TryRunTics()`, before `pd_begin_frame()` wakes core1, but the inherited pause
  queued `render_frame_ready` and waited while core1 slept on `core1_wake`.
  The revised candidate wakes core1 into a dedicated save-pause rendezvous,
  then retains the independently necessary `flash_safe_execute()` sector
  lockout and SRAM-only DMA sources. It adds 16 BSS bytes and has UF2 SHA-256
  `24ac61cc18f4b92c6b0298049336a967aa461ef900dc31e94347689aa46cde1f`.
  Its first physical save completed and returned control normally.
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
- The later lightweight baseline is coherent and sounds acceptable, but
  `DOOM_ENABLE_MUSIC` remains `OFF` by default until the standalone comparison
  identifies a genuinely pleasant profile and it passes an in-game SFX test.
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
- Does the revised pause-and-flash-safe save candidate overwrite and retain its
  saves after an explicit normal reboot? Existing slots 1 and 2 load correctly.
- How does the accepted F16 control baseline feel during a longer combat run?
- Is the 140px quadratic movement curve calm enough in the middle while keeping far-travel running reachable?
- Does release-resolved PWR tap-fire remain responsive and reliable during combat?
- Would an explicitly menu-only swipe mode feel better than F17 fixed-zone
  navigation without weakening input consistency elsewhere?
- Does the installed short-PWR/Enter confirmation candidate show the expected
  text, quit on a tap, and cancel cleanly on PWR hold/Escape?
- Can Doom and a Wolf3D/Spear port fit as independent application/data slots within the 16 MiB flash, selected by a small launcher?
