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
one-minute combat baseline of 34.3 FPS with zero DMA timeouts. A revised
save-only image that fixes the pre-write core1 pause deadlock as well as flash
coordination passed its first save. A follow-up image with explicit
`SAVED GAME <slot> - <level>` labels is now installed and flash-verified;
exact F18 remains the rollback. The session is closed with effects-only audio
as the deliberate default; music is deferred rather than abandoned.
The remaining known input/UI gap is Doom's Quit Game confirmation: it still
asks for keyboard Y/N, while a short PWR tap emits menu Enter. The planned
handheld behaviour is `TAP PWR TO QUIT`, with short PWR confirming and the
existing PWR hold/Escape cancelling.

The first flash-safe save candidate still froze. Verified readback of the final
64 KiB showed it remained erased, proving both attempts stopped before flash
and correcting the earlier root-cause claim. Save runs during the game tick,
while core1 sleeps on `core1_wake`; the inherited pause instead signalled
`render_frame_ready` and waited forever. Revised candidate
`24ac61cc18f4b92c6b0298049336a967aa461ef900dc31e94347689aa46cde1f`
uses a dedicated core1 pause acknowledgement and retains the safe sector writer.
The first physical save now completed successfully and returned control.
Installed follow-up UF2
`fbc4553b6fa196cee17b99f53153398109c4e167f2572e32a47ecee7d2136d22`
uses labels such as `SAVED GAME 1 - HANGAR`; visual naming confirmation, load,
overwrite, and reboot persistence remain. The combined speaker-mastered music image
`2535e3600cdfbe9ac44577405ef276f1ef85a397fabd9e34a7b3e92a7ce51875`
is hardware-rejected: it emitted distorted intermittent bursts rather than a
coherent track. Two automatic serial-targeted restore attempts could not reach
its USB reset interface. Manual BOOTSEL recovery then restored and verified
exact effects-only F18, and the board rebooted into application mode.

## Done this session

- Fixed the pre-write core1 save-pause deadlock, retained multicore-safe flash
  writes, and physically completed the first save successfully.
- Replaced keyboard entry with the final clock-free slot format
  `SAVED GAME <slot> - <level>` and installed/verified candidate
  `fbc4553b6fa196cee17b99f53153398109c4e167f2572e32a47ecee7d2136d22`.
- Rejected the speaker-mastered oscillator music candidate after silence and
  distorted intermittent bursts; restored the accepted effects-only path.
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
- Reproduced the save persistence path twice and proved by flash readback that
  the freeze occurs before the first sector write.
- Replaced the mismatched display-frame pause with a dedicated game-tick/core1
  rendezvous, while retaining the safe sector writer for the next stage.

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
- Doom's Quit Game Y/N prompt is not yet handheld-compatible; this is a small
  source/input task, not a WAD or rendering limitation.

## Next actions

1. Make Quit Game display `TAP PWR TO QUIT`; translate short PWR/menu-confirm
   to Yes only while a confirmation dialog is active, retaining hold/Escape
   as cancel, then hardware-test both paths.
2. Reopen Load Game and load the successfully created first slot.
3. Confirm a new E1M1 slot is labelled `SAVED GAME 1 - HANGAR`, overwrite the
   slot, reboot normally, and load it again.
4. If music is revisited, move to fixed-memory OPL2 with refill instrumentation.
   Then return to menu-only swipe or the Wolf3D/Spear feasibility investigation.

## Risks/blockers

- F18 emits 31.4% more pixels than 448x280; its measured average cadence is
  only 2.5% slower, though the runs are representative rather than identical.
- Full 448x368 deliberately stretches exact-4:3 geometry by 9.5% vertically;
  this trade-off is accepted for the better physical experience.
- The earlier burning odour remains unexplained. Stop any BOOT-related test on
  unexpected sound, heat, odour, smoke, reset, freeze, or visible damage.
- The revised pause and first sector-write path passed physically. Load,
  overwrite, and reboot persistence are not yet accepted; stop immediately on
  a freeze or abnormal display/audio behaviour.
- On 20 August the unpinned Homebrew ARM GCC 16.2 path failed because that
  package lacks newlib headers/libraries. The project-owned ARM GNU 15.3 build
  `firmware/build-f19-save-gcc15` completed successfully; use that pinned
  toolchain rather than treating the Homebrew failure as a source regression.

## Start-next-session prompt

Continue from installed slot-first save candidate
`fbc4553b6fa196cee17b99f53153398109c4e167f2572e32a47ecee7d2136d22`.
First fix the keyboard-only Quit Game confirmation by mapping short PWR to Yes
only for active Y/N prompts and changing its suffix to `TAP PWR TO QUIT`; keep
PWR hold/Escape as cancel. Then confirm `SAVED GAME 1 - HANGAR` and load that
slot before overwrite/reboot tests. If anything abnormal
happens, restore exact accepted F18
`5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`.
Once persistence is accepted, music may resume as a separately gated,
instrumented fixed-memory OPL2 experiment; do not revive the rejected
lightweight oscillator mastering path.
