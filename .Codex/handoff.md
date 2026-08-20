# Handoff

## Status

The installed core experience is full-panel 448x368 Doom with the accepted
asymmetric thumb D-pad and visible guides, in-zone double-tap Use, PWR
release/hold for Fire and Escape, BOOT tap for next weapon, and BOOT hold plus
LEFT/RIGHT for strafe. Full-panel combat measured 34.3 presented FPS over one
minute with zero DMA timeouts. Save slots use `SAVED GAME <slot> - <level>`;
existing slots 1 and 2 load successfully.

Quit Game now says `Press PWR to quit.` and short PWR powers the device off
instead of entering the inherited ENDOOM dead end. USB-connected and
battery-only shutdown both passed hardware testing. Hold-to-cancel remains a
small regression check.

Music work is closed on the lightweight fixed-memory MUSX synth with General
MIDI percussion at 50%. The normal firmware includes it but starts Music Volume
at zero. At zero or while paused, its generator detaches, eliminating
continuous synthesis and silent-buffer CPU work; about 1.3 KiB of fixed SRAM
remains reserved. Raising Music Volume in Options resumes the score. Installed
and flash-verified UF2 SHA-256:
`e97168255564a6e5b13b2c5eaed03ecb3abb3baf360601cb232dcdfc1543ebb6`.
A short audible regression check of default silence, SFX, opt-in, and returning
to zero remains useful.

## Done this session

- Replaced keyboard-only Quit Game confirmation with handheld PWR language and
  scoped PWR confirmation handling.
- Replaced bare-metal ENDOOM exit with AXP2101 software power-off plus watchdog
  recovery; confirmed shutdown over USB and on battery.
- Increased the coupled visible/tappable LEFT and BACK edge zones to 30px.
- Built a standalone music lab, diagnosed its invalid silent lifecycle, and
  moved controlled listening back into full Doom.
- Rejected heavy mastering after distorted bursts and rejected the smooth
  profile after it buried tonal voices beneath percussion.
- Added independent percussion scaling, compared 80%, 60%, and 50%, and
  accepted 50% without changing SFX or tonal voices.
- Included music in the normal firmware but muted it by default, then detached
  the generator at zero so muted music has no continuous CPU cost.
- Preserved 218,800 bytes of calculated Doom-zone headroom in the final image.

## Carry-forward

- F18 full-panel is the presentation baseline. F17 448x336 is the exact-4:3
  visual rollback and 448x280/35.2 FPS is the measured performance rollback.
- F15 is the short-BOOT-only rollback; F14.1 is the no-runtime-BOOT rollback.
- The earlier combat freeze has not recurred but remains a long-session watch
  condition.
- The 30px LEFT/BACK geometry is installed; a deliberate longer thumb-control
  assessment remains useful before styling the guides permanently.
- Existing saves load. Overwrite plus explicit reboot-persistence testing is
  still incomplete.
- Do not reopen music synthesis or pursue OPL2 unless later play exposes a
  regression or creates a concrete quality goal.
- Never push future local work without Alexander's explicit request.

## Next actions

1. Run a longer combat session with the installed final firmware, checking
   controls, audio, saves, and E1M1-to-E1M2 stability together.
2. A/B test deliberate menu-only swipe navigation against fixed-zone menu
   navigation without changing gameplay controls.
3. Continue the separate Wolf3D/Spear feasibility project, then assess a small
   two-game launcher only after the second engine works independently.

## Risks/blockers

- Full 448x368 intentionally stretches exact 4:3 by 9.5% vertically; this is
  accepted for the physical experience.
- The music backend permanently reserves about 1.3 KiB SRAM even when muted,
  though muted CPU work is now eliminated.
- The earlier burning odour remains unexplained. Stop any BOOT-related test on
  unexpected heat, odour, smoke, audio, reset, freeze, or visible damage.
- Use the project-owned ARM GNU 15.3 toolchain. The Homebrew ARM package lacks
  the required newlib setup and is not evidence of a source regression.

## Start-next-session prompt

Continue from installed firmware
`e97168255564a6e5b13b2c5eaed03ecb3abb3baf360601cb232dcdfc1543ebb6`.
First decide between a longer integrated Doom play session, the menu-only swipe
A/B test, or continuing the sibling Wolf3D port. Treat full-panel rendering,
the asymmetric D-pad/PWR/BOOT control vocabulary, flash-safe saves, PWR
power-off, and runtime-muted optional music as locked baselines unless testing
reveals a concrete regression.
