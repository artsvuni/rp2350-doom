# Project Log

## 2026-08-16

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
