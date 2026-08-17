# Project Context

## Summary

DOOM runs on the Waveshare RP2350-Touch-AMOLED-1.8 and is playable with touch movement plus the PWR button. The current diagnostic build uses centered, pixel-exact 320x200 output. The renderer uses a tiled software transpose because the SH8601 cannot rotate axes in hardware. Runtime BOOT polling has been abandoned after two BOOT-enabled hardware tests behaved abnormally while the pre-BOOT build remained normal. Extended gameplay is improved but an earlier silent freeze during active combat remains under investigation.

## Active goals

- Make one-finger touch movement comfortable and efficient.
- Confirm extended-play memory stability and diagnose any remaining freeze.
- Preserve the hardware-verified asynchronous sound-effect path while testing extended gameplay stability.
- Keep effects-only audio as the device default while preserving music as an optional experiment.
- Evaluate selectable full-tilt and hybrid QMI8658 control models against the documented floating-touch baseline.
- Measure memory and timing first, then refactor subsystems in small hardware-testable stages.

## Key decisions made

- Preserve Doom's 16:10 aspect ratio; temporarily use native 320x200 rather than 448x280 scaling to isolate the freeze.
- Use 40-row packed transpose tiles; SH8601 MADCTL has no row/column exchange.
- Place the RP2350 short-pointer window at `0x20040000..0x20080000`, ending at the linker stack limit.
- Never read BOOT during normal gameplay; reserve it exclusively for entering BOOTSEL during power-on/reset.
- Defer any BOOT-as-input revisit to an isolated single-core, audio-disabled SDK experiment; it is not part of the playable firmware roadmap now.
- Commit meaningful changes locally, never push without an explicit request, and normally squash unpublished commits into one milestone before pushing.
- Keep README updates publication-oriented: major milestones or final pre-push cleanup only, not routine local commits.
- Treat motion control as an optional experiment: preserve floating swipe-and-hold as Control Model A and test hybrid tilt-plus-touch before considering replacement.

## Current state

- Game boots, renders, loads E1M1, and supports combat.
- Touch boundary chatter is filtered and no longer triggers blocking screen logs.
- Current experimental movement is a floating four-way digital joystick: touch anywhere, drag, and hold.
- The original four fixed hold zones remain available via a compile-time selector.
- Video is centered native 320x200: 128,000 display bytes and five packed transfers per frame.
- OOM now watchdog-reboots into a persistent `OOM req=... free=...` report.
- Latest freeze produced no OOM reboot, ruling out ordinary zone exhaustion.
- Driver hardening did not eliminate the freeze; disabled audio does zero work and display DMA stalls time out and reset PIO.
- Boot diagnostics turn off when game graphics take over, leaving a clean border while preserving next-boot OOM reporting.
- Sound effects are enabled through a two-buffer DMA/IRQ I2S queue; all ADPCM channel mutations are serialized across cores.
- Hardware confirms the game boots and its sound effects play correctly with the asynchronous backend.
- The shareware WAD and generated WHD contain music, and the nine-voice fixed-memory MUSX synthesizer plays it successfully on hardware.
- The lightweight chiptune rendering is not enjoyable through the small speaker, so `DOOM_ENABLE_MUSIC` defaults to `OFF`; the full backend remains available for later experiments.
- The latest effects-only deployment was hardware-confirmed with working SFX, no music, and normal immediate gameplay behavior.
- Both long-press and single-press runtime BOOT builds produced abnormal hardware behavior; the single-press experiment has been removed from the main firmware.
- Returning to the pre-BOOT-polling effects-only firmware restored normal operation, making runtime BOOT access the common suspect even though the exact electrical/software mechanism is not proven.
- The safe UF2 is byte-identical to the verified pre-BOOT build: SHA-256 `0fa5d343884f25a2c9a99aeea84177eb2014417d5b4cdb0f8f26dd2e27a4f1e2`.

## Open questions

- Does the lower-cost pixel-exact video path eliminate or delay the combat freeze?
- If not, where do persistent stage/heartbeat diagnostics show the two cores stopping?
- Does asynchronous SFX remain stable during a longer sustained-combat test?
- Can the optional MUSX synthesizer's timbre be improved enough to suit the small speaker?
- Does the floating swipe-and-hold model feel better than fixed zones, and what dead-zone tuning does it need?
- Which PWR or touch gesture should provide Escape without runtime BOOT access?
- Which tilt mapping is most playable: full tilt, tilt steering plus touch movement, or touch steering plus tilt movement?
- Can proportional motion feed `ticcmd_t` directly without destabilizing deterministic game-tic behavior?
