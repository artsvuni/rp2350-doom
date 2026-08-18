# Handoff

## Status

F18 is installed and hardware-accepted as the core handheld experience. It
combines asymmetric fixed thumb zones and visible guides with in-zone double-tap Use,
PWR release/hold for Fire and Escape, BOOT short release for next weapon, and
BOOT hold as a LEFT/RIGHT strafe modifier. Alexander reported that the complete
model worked great and that he loves the full-panel presentation. F18 fills
448x368 through a deliberate vertical stretch and memory-neutral overlapping
final tile. F17 448x336 is the exact-4:3 rollback; 448x280 remains locked at a
measured 35.2 FPS with effects-only audio. F18 itself now has a checksum-valid
one-minute combat baseline of 34.3 FPS with zero DMA timeouts, and the normal
non-profiler F18 image is restored on the device.

## Done this session

- Preserved exact F15 and added default-off BOOT hold interpretation without
  increasing the 25 ms flash-safe sampling rate or keeping flash suspended.
- Mapped held BOOT plus LEFT/RIGHT to sustained 32-unit strafe; the two forward
  boundary bands become forward-strafe and UP/DOWN remain unchanged.
- Suppressed weapon selection after a resolved hold and prevented modifier
  contacts from becoming double-tap Use on release.
- Audited every active DMA source in SRAM and retained F15's core coordination,
  timeout, debounce, and local-single-player gate.
- Built and flashed candidate UF2
  `6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`;
  it leaves 220,176 zone bytes.
- Hardware-confirmed short weapon cycling, held left/right strafe, and
  release-to-turn with normal display/audio and no reported abnormality.
- Documented the control journey from the playable but high-travel floating
  anchor and unreliable motion experiments to the thumb-friendly F16 model.
- Added a 448-wide height override and built 448x320 without changing the
  accepted two-buffer 20-row DMA path or static memory. Candidate hash is
  `a253683ef45e8e412bcfb35e6a6c1884972f21cd39b653cb15945fcbd5fa8170`.
- Verified AUTO rebuilds exact accepted F16 hash
  `6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`.
- Flashed and hardware-passed 448x320 for image, audio, and short gameplay.
- Built 448x336 with a padded 20-row final transfer, no extra SRAM, unchanged
  220,176 zone bytes, and hash
  `2a9f7a4ed74392e016fd2407fac1981aa1fb4c8e02c7d9724e0d25a31a913ad8`.
- Hardware-passed 448x336 and accepted it as the preferred visual baseline.
- Built F18 448x368 with no new static allocation and changed the panel clear
  to avoid a short 8-row tail. Its hash is
  `5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`.
- Flashed and byte-verified F18 autonomously, then hardware-accepted it as the
  core experience after a strongly positive full-panel play test.
- Captured 2,059 frames over 60.021 seconds at 448x368: 34.3 FPS,
  29,164/45,612us average/max cadence, 24,469/31,064us presentation, and zero
  DMA timeouts. Restored and verified normal F18 immediately afterwards.

## Carry-forward

- F18 is the combined experience baseline. Fine sensitivity, hold threshold,
  guide styling, and zone geometry can be revisited later but are no longer
  blockers.
- Exact F15
  `769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`
  is the release-only BOOT rollback; F14.1 is the no-runtime-BOOT rollback.
- The panel and installed image are 448x368 landscape. F17 exact-4:3 448x336
  and 448x320 remain visual rollbacks.
- The earlier combat freeze remains a long-session watch condition, though it
  has not recurred in the latest meaningful E1M1-to-E1M2 run.
- Local commits must not be pushed unless Alexander explicitly asks.

## Next actions

1. A/B test the promising menu-only swipe mode against fixed-zone menus.
2. Run a longer normal play session when desired, or begin the separate
   Wolf3D/Spear feasibility investigation.
3. Add a low-battery-only overlay later; do not reserve a permanent UI band.

## Risks/blockers

- F18 emits 31.4% more pixels than 448x280; its measured average cadence is
  only 2.5% slower, though the runs are representative rather than identical.
- Full 448x368 deliberately stretches exact-4:3 geometry by 9.5% vertically;
  this trade-off is accepted for the better physical experience.
- The earlier burning odour remains unexplained. Stop any BOOT-related test on
  unexpected sound, heat, odour, smoke, reset, freeze, or visible damage.

## Start-next-session prompt

Continue from installed and hardware-accepted F18 full-panel 448x368 as the
core experience. Preserve F17 448x336, 448x320, and measured 448x280 rollbacks.
F18 is measured at 34.3 FPS with zero DMA timeouts; display optimisation is
closed unless future normal play reveals a concrete regression.
