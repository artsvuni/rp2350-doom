# Project Log

## 2026-08-17

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
