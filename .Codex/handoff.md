# Handoff

## Status

F11 is built, flashed, and playable on the Waveshare RP2350-Touch-AMOLED-1.8.
The locked presentation is 448x280 full-width at a measured 35.2 FPS with
effects-only audio. Pointing-finger move/turn is the accepted baseline; short
bottom-corner strafe bursts are functional and better than no strafe, but not
considered fully tuned. Continuous pitch/roll control is rejected.

## Done this session

- Measured and optimised full-width presentation from 28.7 to 35.2 FPS with
  zero display-DMA timeouts and no additional static presentation memory.
- Repaired FT3168 point tracking, selected pointing-finger control, and tuned
  compact progressive move/turn response through F9.
- Tested multiple pitch and roll models; rejected continuous tilt because
  neutral, activation, and stopping were not dependable in natural grips.
- Added F11 bottom-corner double-tap strafe bursts, retained Use elsewhere,
  softened forward/back with a 140px quadratic curve, and compiled the IMU out.
- Verified F11 at `__end__=0x2004930c` with 224,500 bytes of zone headroom;
  UF2 SHA-256 is
  `ec00e1262a4a3f53f93c9cb09cd6fc49b4e39ef4d9fe2ae820f30d1f1eba2cd6`.
- Updated README, roadmap, context, TODOs, and project history for the milestone.

## Carry-forward

- Never read BOOT during gameplay; use it only for ROM BOOTSEL recovery.
- Effects-only is the product default; optional MUSX music works but sounds poor
  on the small speaker.
- The earlier combat freeze is not ordinary zone OOM and remains a long-session
  watch condition, though Alexander reached E1M2 without recurrence.
- The panel is 448x368 landscape. Current 448x280 output leaves 44px bands above
  and below while preserving raw 16:10 geometry.
- Local commits must not be pushed unless Alexander explicitly asks.

## Next actions

1. Audit essential controls, beginning with weapon cycle and Escape/menu; avoid
   crowding the existing move/turn, Use, strafe, and fire vocabulary.
2. If desired, build and measure 448x336 traditional 4:3 correction. It leaves
   16px bands and emits 20% more pixels than 448x280; do not replace the locked
   pipeline without a comparable combat capture.
3. Either perform a longer E1M1-to-E1M2 stability run or close this port and
   start a separate Wolfenstein 3D/Spear of Destiny feasibility project.

## Risks/blockers

- F11's exact strafe direction/burst length, Use outside corners, and new
  forward curve have not received a structured combat comparison.
- A true 448x368 fill costs 31.4% more output pixels and cannot preserve both
  raw 16:10 geometry and the whole image without stretch/crop/redesign.
- A two-game console needs explicit 16 MiB flash budgeting, application slots,
  asset partitions, reset/selection flow, and user-supplied legal game data.
- Automatic `picotool -f --ser` reboot intermittently failed in the last cycle;
  manual BOOTSEL plus `picotool load -v` remained reliable.

## Start-next-session prompt

Continue from F11 commit and the 448x280 full-width effects-only build. First
decide the minimum essential action set and prototype weapon cycling without
disturbing current touch controls. Then choose between a measured 448x336
display experiment, a longer stability run, or a separate Wolf/Spear port and
two-game launcher feasibility audit.
