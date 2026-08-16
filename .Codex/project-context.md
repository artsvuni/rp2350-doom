# Project Context

## Summary

DOOM runs on the Waveshare RP2350-Touch-AMOLED-1.8 and is playable with touch movement plus the PWR button. The current diagnostic build uses centered, pixel-exact 320x200 output. The renderer uses a tiled software transpose because the SH8601 cannot rotate axes in hardware. Recent work recovered substantial SRAM and expanded Doom's zone to 227,944 bytes. Extended gameplay is improved but a silent freeze during active combat remains.

## Active goals

- Make one-finger touch movement comfortable and efficient.
- Confirm extended-play memory stability and diagnose any remaining freeze.
- Eventually replace the blocking audio path and re-enable sound.

## Key decisions made

- Preserve Doom's 16:10 aspect ratio; temporarily use native 320x200 rather than 448x280 scaling to isolate the freeze.
- Use 40-row packed transpose tiles; SH8601 MADCTL has no row/column exchange.
- Place the RP2350 short-pointer window at `0x20040000..0x20080000`, ending at the linker stack limit.
- Keep BOOT input disabled until flash access can be synchronized across cores.

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

## Open questions

- Does the lower-cost pixel-exact video path eliminate or delay the combat freeze?
- If not, where do persistent stage/heartbeat diagnostics show the two cores stopping?
- Does the floating swipe-and-hold model feel better than fixed zones, and what dead-zone tuning does it need?
- How should BOOT/menu and weapon switching be safely integrated?
