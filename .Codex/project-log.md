# Project Log

## 2026-08-20

### 10:23 — Prepare the cross-machine Doom handoff

- Corrected the README to describe the accepted clock-free save label
  `SAVED GAME 1 - HANGAR` instead of the discarded elapsed-time format.
- Recorded the remaining keyboard-only Quit Game prompt as the next small
  device-UI task: short PWR confirms, PWR hold/Escape cancels.
- Refreshed project context and the next-session prompt before publishing the
  accumulated Doom milestone.
- Rebuilt the latest full-panel save target successfully with the project-owned
  ARM GNU 15.3 toolchain. The unpinned Homebrew GCC 16.2 path still lacks
  newlib and is not a valid source check.

## 2026-08-18

### 22:34 — Close the session on effects-only audio

- Wrapped with the slot-first save-name direction accepted and candidate
  `fbc4553b...` installed and programmer-verified.
- Kept effects-only audio as the product baseline; the accepted SFX path is
  unchanged and music remains optional/deferred.
- Preserved the next music attempt as fixed-memory OPL2 with refill-cost and
  underflow instrumentation before another physical listening test.
- Save load, overwrite, and reboot persistence remain the first small checks
  when the project resumes.

### 22:30 — Put the save slot first in generated names

- Changed the handheld format to `SAVED GAME 1 - HANGAR`, with the digit taken
  directly from the selected save slot.
- Bounded the level-title portion to eight characters and trimmed a trailing
  space after clipping, preserving Doom's original 23-character label limit.
- Built, installed, and programmer-verified full-panel effects-only UF2
  `fbc4553b6fa196cee17b99f53153398109c4e167f2572e32a47ecee7d2136d22`;
  the board returned to application mode automatically.

### 22:32 — Install the level-and-slot naming candidate

- Installed and programmer-verified full-panel effects-only UF2
  `5fea76c6a4bb2d5cdb9847d99668d99256cd1ad1ed5856275587c811ae08effa`
  through the autonomous USB reset path.
- The board returned to application mode without a physical BOOT press.
- Physical confirmation of `HANGAR 1` in the save menu remains.

### 22:26 — Simplify handheld save names

- Replaced slot/map/elapsed-time labels with Doom's own level title plus the
  selected slot number, such as `HANGAR 1`.
- Removed level-number prefixes, uppercased the result, and bounded long titles
  to Doom's existing 23-character save-label field.
- Deliberately skipped RTC integration and made no change to save data format.
- Built the full-panel effects-only flash-safe-save configuration successfully;
  naming validation remains.

### 22:35 — Pass the first physical save write

- Hardware-confirmed the revised pause rendezvous and safe flash writer: one
  automatic save completed and Doom returned normally instead of freezing.
- Kept acceptance bounded; load, overwrite, and post-reboot persistence remain.
- Retained the current deterministic `S1 E1M1 03:42`-style label while naming
  alternatives are assessed separately.
- Commit: included in this first-save acceptance documentation commit.

### 22:25 — Correct the save diagnosis at the game-tick boundary

- The first flash-safe candidate was definitely installed and verified, but
  reproduced the identical slot-selection freeze; music remained off
  intentionally because this was the effects-only save build.
- Read and verified the final 64 KiB before restoring F18. It remained erased,
  proving neither attempt reached the first flash-sector write and correcting
  the earlier driver-first diagnosis.
- Traced the deadlock: save runs in `TryRunTics()` before `pd_begin_frame()`, so
  core1 sleeps on `core1_wake`; the inherited pause signalled
  `render_frame_ready` and waited forever.
- Added a dedicated core1 pause acknowledgement/resume handshake and retained
  the later SDK flash lockout. Revised UF2 is
  `24ac61cc18f4b92c6b0298049336a967aa461ef900dc31e94347689aa46cde1f`;
  exact F18 is restored pending the next bounded test.
- Commit: included in this save-pause deadlock fix commit.

### 22:10 — Fix the first real save-write freeze

- The first automatic-save hardware attempt reached slot persistence and
  froze; exact F18 was immediately restored and verified automatically.
- Root-caused the freeze to the legacy writer exiting XIP and erasing/programming
  flash directly while Doom's display/audio core remained active. Generated
  save naming does not participate in that failure.
- Added default-off `DOOM_FLASH_SAFE_SAVES`: drain display work, keep audio DMA
  sources in SRAM, park core 1 with `flash_safe_execute()`, and run only the
  low-level sector callback from SRAM. A failed lockout aborts the write.
- Built and installed candidate
  `9261377673e9422d0e55e86a649f7174ca878e81d69505f2f9e63edaf07afee1`;
  it adds 88 text bytes, no BSS, and awaits a one-save hardware test.
- Commit: included in this multicore-safe save fix commit.

### 22:00 — Reject mastered music and begin F18 recovery

- Hardware rejected the corrected-gain candidate: no coherent soundtrack was
  audible and gameplay produced clearly distorted intermittent "puf" bursts.
- Stopped the listening experiment immediately; no further gain or filter
  tuning is justified on this signal.
- Recorded queue starvation as the leading but unmeasured diagnosis: the
  heavier generator can miss the bounded audio queue, whose defined fallback
  is silence, producing isolated audible blocks.
- Verified the local rollback UF2 is exact accepted F18
  `5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`.
- Two serial-targeted automatic restore attempts could not reach the rejected
  application's USB reset interface.
- After Alexander manually entered BOOTSEL, restored exact F18
  `5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`;
  flash verification passed and `picotool reboot -a` returned it to Doom.

### 21:50 — Restore an audible mastered-music test level

- Hardware feedback found the first mastered candidate effectively silent.
- Confirmed the music backend, MUSX parser, symbols, and build flags are present;
  the candidate had instead reduced the score by about 12dB before accounting
  for the smoother waveform's lower RMS level.
- Restored Doom's proven 8/15 starting level and unity music master, retained
  peak control and SFX ducking, and relaxed the music-only high-pass from about
  160Hz to 90Hz so low title material remains audible.
- Rebuilt candidate
  `2535e3600cdfbe9ac44577405ef276f1ef85a397fabd9e34a7b3e92a7ce51875`
  with unchanged `__end__=0x2004a14c`; flash and listening validation are pending.

### 21:35 — Build a small-speaker music-quality candidate

- Measured the original optional backend at 1,324 static bytes and 6,536 text
  bytes over effects-only F18, ruling out memory burden as the reason it
  sounded poor.
- Audited the real signal path: nine raw discontinuous oscillators and
  full-band noise feed the 44.1kHz mono mixer, ES8311 codec, NS4150B amp, and
  12x10mm 8-ohm speaker without music-specific mastering.
- Compared volume-only, codec DSP, authentic upstream OPL2, and prerecorded
  ADPCM approaches. Selected a reversible source-and-mix improvement before a
  much larger OPL import.
- Added continuous timbres, short attack/release envelopes, filtered
  percussion, music-only high/low-pass filtering, peak control, 4/15 starting
  volume, and SFX ducking. The accepted SFX and codec setup are unchanged.
- Built full-panel music/save candidate
  `cfa84000ec33a560f1a3c20c9f466e27550292f81d31cc87317621f1ba74c16a`;
  it ends at `0x2004a14c` with about 218,804 zone bytes left.
- Verified mastering-off reproduces the old music UF2 exactly and music-off
  reproduces accepted F18 exactly. Physical listening is pending.

### 21:30 — Add a keyboard-free save candidate

- Traced the failed handheld save flow to Doom's keyboard-only description
  editor: the existing joypad auto-name fallback is unreachable because this
  port correctly posts touch and button actions as keyboard events.
- Added a default-off handheld option that immediately saves a selected slot
  with a deterministic slot/map/elapsed-time label such as `S1 E1M1 03:42`.
- Preserved vanilla text entry for builds without the option and avoided any
  dependency on an unconfigured real-time clock.
- Build and hardware create/overwrite/reboot-load validation are pending.

### 20:53 — Record the F18 full-panel performance baseline

- Ran the optional bounded in-level profiler at the accepted 448x368 F18
  presentation: 2,059 frames in 60.021 seconds, or 34.3 presented FPS.
- Validated the 100-byte persistent report checksum; cadence averaged 29,164us,
  presentation 24,469us, and display DMA recorded zero timeouts.
- Recorded that this is only 2.5% slower in cadence than 448x280/35.2 FPS while
  emitting 31.4% more pixels; the two gameplay routes were representative, not
  frame-identical benchmarks.
- Restored and programmer-verified the accepted normal F18 UF2 after capture.
- Updated README, roadmap, decisions, context, TODOs, and handoff.
- Commit: included in this F18 measured-baseline documentation commit.

### 20:07 — Accept F18 as the core handheld experience

- Hardware-accepted installed 448x368 after Alexander reported that he loved
  the full-panel experience and wanted it retained as the core presentation.
- Kept the geometry claim precise: F18 deliberately stretches 448x336 by 9.5%
  vertically; F17 remains exact-4:3 rollback and 448x280 the measured rollback.
- Demoted a 448x368 combat capture from acceptance gate to optional baseline
  documentation; subjective success is sufficient to keep the current image.
- Updated README, roadmap, controls, context, TODOs, and handoff.
- Commit: included in this F18 acceptance milestone commit.

### 19:33 — Build F18 full-panel 448x368 candidate

- Chose a reversible full-panel stretch test after Alexander preferred game
  pixels over permanent top/bottom UI; 448x336 remains the installed rollback.
- Added a memory-neutral final-tile path that reuses the two asynchronous
  buffers and resends rows 348..359 with 360..367 in one 20-row transaction.
- Removed the one-time clear's unreliable 8-row tail by ending with an
  overlapping full 20-row stripe; hardware must confirm whether this removes
  the changing coloured remnant previously visible in the top-right band.
- Built both 448x368 and a separate 448x336 regression configuration. F18 keeps
  `__end__=0x20049bf0`, 220,176 zone bytes, and has UF2 SHA-256
  `5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`.
- Flashed and byte-verified F18 through autonomous `picotool -f`; it rebooted
  to application mode and now awaits Alexander's short physical gate.
- Commit: included in this F18 full-panel candidate commit.

### 19:08 — Accept F17 and retain menu swipe as a future option

- Hardware-confirmed that F17 fixes score-screen progression and fixed-zone
  menu routing, with DOWN restored to its shallow 448x336 presentation.
- Accepted the installed F17 firmware as the new controls baseline.
- Recorded Alexander's positive response to the old menu swipe as a future
  menu-only A/B candidate; gameplay and intermission mappings stay unchanged.
- Updated README and project/control context to reflect the accepted behavior.
- Commit: included in this F17 acceptance documentation commit.

### 18:55 — Build F17 state-consistent controls

- Root-caused the stuck E1M1 score screen: non-level input reverted to arrows
  and PWR Enter, but Doom intermission advances only from native Attack/Use.
- Routed fixed thumb zones to menu arrows, fresh post-release intermission
  touch to Fire, and intermission PWR to Fire without changing Doom mechanics.
- Made DOWN view-relative so 448x336 restores the original 20-pixel visible
  strip; `__end__` and 220,176-byte zone headroom are unchanged.
- Built candidate SHA-256 `8605a47aa53b71c65ba96602c1179f28d3a7b11420691db01e5574c32ad2a324`;
  fixed-zone menu, score progression, and DOWN geometry await hardware test.
- Commit: included in this F17 input-state candidate commit.

### 18:41 — Accept exact-4:3 448x336 on hardware

- Hardware-passed the padded final tile: the image looks correct and good, and
  short gameplay feels very smooth with no perceptible loss versus 448x320.
- Promoted installed 448x336 to the preferred visual baseline while retaining
  exact 448x320 as visual rollback and 448x280/35.2 FPS as measured rollback.
- Kept the performance claim qualitative; one bounded 448x336 combat capture
  is the next gate before closing display work or separately debating 448x368.
- Commit: included in this 448x336 acceptance commit.

### 18:08 — Pass 448x320 and build exact-4:3 448x336

- Hardware-passed installed 448x320 for image, audio, and short gameplay;
  Alexander judged it excellent and visually much better.
- Clarified that 448x320 partially corrects Doom's 320x200 non-square-pixel CRT
  presentation; 448x336 is the exact 4:3 square-pixel equivalent.
- Added one bounded padded-final-tile path: 16 image rows plus four cleared
  border rows in the unchanged 20-row transaction, with no additional buffer.
- Exact 448x280 and 448x320 hashes remain unchanged. Candidate 448x336 hash is
  `2a9f7a4ed74392e016fd2407fac1981aa1fb4c8e02c7d9724e0d25a31a913ad8`;
  it adds 56 flash bytes, no SRAM, and retains 220,176 zone bytes.
- Commit: included in this exact-4:3 display candidate commit.

### 17:51 — Build the isolated 448x320 taller-display candidate

- Added a 448-wide height override while keeping AUTO as the conservative,
  byte-identical 448x280 behavior.
- Selected 448x320 as the first height gate because it adds 14.3% pixels and
  reduces each black band from 44px to 24px without changing the proven
  two-buffer 20-row asynchronous DMA transaction path.
- Rebuilt accepted F16 exactly at
  `6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`.
- Built candidate
  `a253683ef45e8e412bcfb35e6a6c1884972f21cd39b653cb15945fcbd5fa8170`;
  text/data/BSS, `__end__=0x20049bf0`, and 220,176 zone bytes are unchanged.
- Documented why 448x336 follows only after this gate: 336 is not divisible by
  the accepted 20-row tile and therefore mixes scaling with transfer changes.
- Commit: included in this taller-display candidate commit.

### 17:45 — Accept F16 controls and move to taller display work

- Hardware-accepted the BOOT-hold modifier: short release still changes weapon,
  held LEFT/RIGHT strafes, and release restores turning. Alexander reported it
  worked great with no abnormal behavior.
- Documented the complete control evolution: floating-anchor occlusion and
  thumb-centroid problems, rejected pitch/roll reliability, fixed asymmetric
  thumb-zone gains, and the impractical corner-strafe gesture F16 replaces.
- Promoted candidate hash
  `6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`
  to the installed F16 controls baseline; exact F15 and F14.1 remain rollbacks.
- Updated README, safety audit, control design, roadmap, context, TODOs, and
  handoff; sensitivity refinement is deferred while taller display work starts.
- Commit: included in this F16 controls milestone commit.

### 17:32 — Build a gated BOOT-hold strafe candidate

- Added default-off `DOOM_BOOT_HOLD_STRAFE`: short release remains next weapon;
  after 250 ms beyond the debounced press, LEFT/RIGHT thumb zones become
  sustained strafe and the forward transition bands become forward-strafe.
- A resolved hold suppresses weapon cycling on release, restores turning after
  release debounce, and prevents a modifier contact becoming double-tap Use.
- Kept F15's 25 ms flash-safe sampling and all safety invariants unchanged.
  Option-off F15 rebuilds byte-identically at
  `769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`.
- Candidate UF2 SHA-256 is
  `6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`;
  it leaves 220,176 zone bytes and awaits one bounded physical test.
- Commit: included in this F15.1 candidate commit.

### 14:33 — Accept F15 BOOT next-weapon with normal sound

- Hardware-passed the final effects-enabled gate: normal SFX remained present,
  one brief BOOT release cycled weapons correctly, and Alexander reported the
  button worked well with everything appearing normal.
- Accepted the installed image as F15. Runtime BOOT remains release-only and
  local-level-only; hold, double-click, menu, and network actions stay excluded.
- Updated README, control design, roadmap, safety audit, decisions, context,
  TODOs, and handoff to reflect the user-facing control milestone.
- Installed UF2 SHA-256 is
  `769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`;
  F14.1 remains the exact rollback.
- Commit: included in this F15 milestone commit.

### 14:26 — Pass silent Doom and audit the effects-enabled BOOT gate

- Hardware-passed silent Gate B: Doom booted, one brief BOOT release changed
  weapon once in-level, and Alexander reported everything normal.
- Restored and flash-verified exact F14.1
  `821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`;
  its application-mode USB interface returned normally.
- Added default-off `DOOM_BOOT_WITH_SOUND_EFFECTS`, valid only with BOOT next-
  weapon. BOOT builds always move the audio fallback DMA source to SRAM; the
  audio-disabled version remains the default.
- Audited the effects candidate's audio buffers, silence, display tiles,
  optional network buffers, BOOT callback, and audio IRQ as SRAM-resident. It
  leaves 220,184 zone bytes and has UF2 SHA-256
  `769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`.
- Rebuilt silent Gate B and normal F14.1 byte-identically. Did not flash the
  effects candidate; one bounded sound-on test is next.
- Commit: included in this effects-gate checkpoint.

### 14:19 — Build and audit silent Gate B next-weapon firmware

- Added default-off `DOOM_BOOT_NEXT_WEAPON`; normal F14.1 builds contain no
  BOOT sampler and rebuilt byte-identically at
  `821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
- Registered Doom core 1 with the SDK flash-safety coordinator before its
  launch handshake and limited sampling to local single-player level play.
- Added 25 ms / two-sample press-and-release debounce with a 2 ms safety
  timeout. Failed lockout attempts are ignored; release queues one existing
  Doom forward weapon-cycle request.
- Made Gate B silent from first hardware initialization: amp low, I2S pins
  high-impedance, no codec/PIO/audio-DMA initialization, and the old XIP
  silence buffer relocated to SRAM for the candidate only.
- Verified the BOOT callback at SRAM `0x200008c4`, silence at `0x20046880`,
  display tiles at `0x2003d3a0`, and 221,252 bytes between heap end and core 1
  stack. Candidate UF2 SHA-256 is
  `891d6076db58253064dba38c4634322ac6109095915737243f38852de7d36076`.
- Did not flash the candidate. The next action is one bounded USB-only Gate C
  press/release test with silence expected and F14.1 retained as rollback.
- Commit: included in this Gate B candidate commit.

### 14:09 — Pass the isolated BOOT safety gate and restore F14.1

- Installed only the isolated `boot_safety_probe` over application-mode USB;
  the write and flash verification completed successfully.
- Alexander briefly pressed and released BOOT once. The probe entered ROM
  BOOTSEL exactly as designed; `picotool info` confirmed RP2350 A2, matching
  chip ID `05bcf1c1aa06aa58`, and `boot type: bootsel`.
- No unexpected audio occurred during the bounded test. This passes only the
  single-core, amplifier/display-disabled Gate A and does not yet validate Doom
  core1/render/audio interaction.
- Restored the byte-verified F14.1 UF2
  `821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`,
  verified the flash write, rebooted into application mode, and confirmed the
  board's USB application interface returned.
- Authorized the next engineering stage: relocate the audio DMA silence source
  to SRAM, register Doom core1 with the SDK flash-safety helper, and build a
  default-off, audio-disabled next-weapon candidate.
- Commit: included in this isolated-probe hardware-result commit.

### 13:48 — Gate BOOT next-weapon behind an isolated safety probe

- Hardware-confirmed F14.1's in-zone Use/Open double tap works well and locked
  it into current project context.
- Audited the exact Waveshare schematic and Raspberry Pi runtime-BOOT/flash
  contract. Key2 is a designed 1 kΩ flash-CS pull-down rather than a supply
  short, but the earlier Doom lockout did not cover autonomous DMA readers.
- Found the concrete missing hazard: audio DMA reads `silence_buffer` from XIP
  address `0x1005a1ec` whenever its SRAM SFX queue is empty. This plausibly
  explains the abnormal audio, but does not prove the reported odour's cause.
- Added a written three-gate protocol and a standalone `boot_safety_probe` that
  forces the speaker amp and display off, leaves I2S pins high-impedance, uses
  no DMA/I2C/PIO/audio/core1/USB stdio, and enters ROM BOOTSEL only after a
  confirmed press plus release through `flash_safe_execute()`.
- Built the probe, verified its sampling callback resides in SRAM at
  `0x20000110`, confirmed no DMA/multicore/I2C/PIO/audio/display symbols link,
  and recorded UF2 SHA-256
  `05ca114df6c699d4deeb4af733e000ebdafe22dfae286ad25ea5bbcaeb65c04f`.
- Rebuilt the F14.1 Doom image byte-identically at
  `821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
- Deliberately did not flash the probe or add BOOT to Doom; the isolated
  physical press/release is the next mandatory safety gate.
- Commit: included in this BOOT safety-probe commit.

### 13:32 — Restore Use double-tap inside the accepted thumb controls

- Hardware-accepted F14 as the best control experience so far: all four
  asymmetric zones worked with a thumb, release-to-stop felt right, both
  forward-turn diagonals were usable, and the visible guides improved control.
- Added F14.1: two short stationary taps anywhere inside the F14 control area
  now emit Use/Open on the second release instead of being discarded.
- Preserved immediate movement on touch-down, accepting two tiny directional
  pulses during the gesture rather than adding latency to every held control.
- Rebuilt F13 radial and the pointing-finger fallback byte-identically. F14.1
  keeps `__end__=0x20049310`, 224,496 zone bytes, and UF2 SHA-256
  `821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
- Commit: included in this in-zone Use-double-tap commit.

### 12:56 — Adapt the D-pad to asymmetric thumb zones

- Hardware feedback accepted F13 directionally: its touch-and-hold model feels
  more like keyboard Doom, provides more control, and works with a thumb much
  better than the relative pointing-finger mapping.
- Added default-off F14 thumb zones matching Alexander's sketch: a narrow
  physical-edge LEFT strip, fingertip-width UP zone, large RIGHT zone, and
  shallow bottom-edge DOWN strip. Zones are contiguous and release is the only
  neutral action.
- Preserved forward-turn diagonals only in two deliberate 12px transition
  bands; omitted backward diagonals and the radial speed step for a simpler
  first comparison.
- Added an optional diagnostic outline overlay with amber active-zone
  highlighting. It writes only box boundaries into the existing packed tile,
  adding no framebuffer or alpha-blending pass.
- The accepted pointing-finger and F13 radial UF2s rebuild byte-identically.
  F14 ends at `0x20049310`, leaves 224,496 zone bytes, and its pre-flash UF2
  SHA-256 is `8b2307eee78b403cbe592c5ea51cf2dc37f8fd25692d2c5ed51e65983b90ba61`.
- BOOT remains untouched pending the separate read-only safety audit.
- Commit: included in this asymmetric-thumb-D-pad commit.

### 12:22 — Build a fixed eight-way D-pad comparison

- Recorded F12.3's unchanged physical rapid-fire result and traced the remaining
  middle-speed loss to Doom's 14-tic pistol recovery rather than more driver
  pulse loss; vanilla weapon cadence remains unchanged.
- Added default-off `DOOM_TOUCH_DPAD`: a fixed 160x160 bottom-left pad with a
  12px neutral centre, eight-way movement/turn composition, and normal/fast
  radial response. Its square owns repeated taps to prevent accidental gesture
  actions; Use and right-side strafe remain available outside it.
- The D-pad adds 48 text bytes and no data/BSS. Active `__end__=0x2004930c`
  still leaves 224,500 zone bytes; UF2 SHA-256 is
  `89ce0216629d17c76bc05bd16cbab0fe392c105b3ddc879e2394717d62ce4b5b`.
- Rebuilt the accepted pointing-finger image byte-identically. BOOT remains
  untouched pending the requested read-only safety audit after this test.
- Commit: included in this fixed-D-pad-candidate commit.

### 11:49 — Preserve a second PWR press in a coalesced PMIC status

- Hardware-tested F12.2: paced click-click produced two shots, but Alexander's
  fastest pair still produced one, so pulse spacing was not the only loss.
- Confirmed that AXP2101 REG49 exposes PWR events as latched occurrence flags
  and the board routes Key1 to PMIC PWRON rather than a raw RP2350 GPIO.
- When a known tap's release and the next press arrive together, F12.3 now
  completes the first tap and re-arms the second instead of clearing both.
- Active and fallback builds pass. Active `__end__=0x2004930c`, leaving 224,500
  zone bytes; UF2 SHA-256 is
  `ae1d4d7626ff3f3c4549d4df1bbcc3aa3ae37624a971b60a0cb457af760089b5`.
- Commit: included in this coalesced-PWR-edge commit.

### 11:41 — Preserve rapid PWR taps as distinct Fire pulses

- Hardware-accepted F12.1 release-resolved Fire and Escape as very comfortable
  in gameplay and menus, but very fast click-click could produce one shot.
- Traced the loss to adjacent virtual-key pulses: key-up and the next key-down
  in one Doom tic never give `player->attackdown` a sampled released state.
- Added a four-entry fixed pulse queue with one full low tic between shots;
  first-shot latency and the 450ms hold are unchanged, and Escape flushes the
  queue before opening the menu.
- Active and fallback builds pass. Active `__end__=0x2004930c`, leaving 224,500
  zone bytes; UF2 SHA-256 is
  `702d44306ff7203f5d5fd5d786fca19f24c5ab9634a4e6e1440b63a3acbdb7fb`.
- Commit: included in this rapid-PWR-pulse commit.

### 11:25 — Extend short PWR hold to menu Escape/Back

- Hardware-confirmed the installed F12 in-level hold works.
- Extended the 450ms recogniser to menu/title/intermission contexts, then
  corrected the press/hold ambiguity: press commits nothing; release before
  450ms emits Fire or Enter by context; reaching 450ms emits Escape/Back.
- A winning hold suppresses release and replaces the hybrid PWR double-click
  path, preventing any Fire, Enter, or delayed Select from the same gesture.
- Active and fallback builds pass. Active `__end__=0x2004930c`, leaving 224,500
  zone bytes; UF2 SHA-256 is
  `ff0cb48361fdcb364912f67fe502eab45dd5be230a70223d6b971abb843cd107`.
- Commit: included in this menu-context PWR-hold refinement commit.

### 11:06 — Build a short software-timed PWR hold for Escape

- Preserved F11 touch controls and immediate one-shot PWR fire; a continuous
  PWR hold now emits Escape once after 450ms rather than waiting for the
  AXP2101's 1–2.5-second long-press IRQ.
- Suppressed all PWR events until physical release after Escape so the PMIC's
  completed short-press event cannot select an item in the newly opened menu.
- Left the PMIC configuration unchanged. Its documented power-off range remains
  4–10 seconds, so continued holding can still power off the board.
- Active and fallback builds pass. Active `__end__=0x20049314`, leaving 224,492
  zone bytes; UF2 SHA-256 is
  `9a3206a60349be30928a14a5fa75ff45291f254e612bf7d90ca4eeda4fdec62d`.
- Bounded application-mode `picotool` reset attempts failed before writing;
  F11 remains installed and F12 awaits manual BOOTSEL installation.
- Separated any future BOOT-as-control work into a written safety audit and an
  isolated single-core, audio-disabled experiment; no runtime BOOT code changed.
- Commit: included in this short-PWR-hold candidate commit.

### 00:31 — Wrap the playable full-width controls milestone

- Updated the publication README to reflect the measured 448x280/35.2 FPS
  pipeline, current F11 pointing-finger controls, rejected continuous tilt,
  effects-only audio decision, build flags, and exact 224,500-byte headroom.
- Recorded panel geometry: 448x368 landscape, current 44px top/bottom bands,
  and a future measured 448x336 4:3 candidate with 16px bands and 20% more
  output pixels.
- Added next-session work for essential control coverage, led by weapon change
  and Escape/menu, while deferring minor F11 tuning unless clearly valuable.
- Captured the longer-term Wolfenstein 3D/Spear of Destiny plus two-game
  launcher idea as a separate flash-slot/driver-reuse feasibility phase.
- Commit: included in this milestone-documentation commit.

### 00:15 — Replace tilt strafe with corner dodge bursts

- Recorded F10 as mechanically functional but UX-rejected: requiring touch was
  unexpected, and a grip change before touch could arm an unintended strafe.
- Compiled motion out and mapped bottom-left/right 96x72px double taps to
  six-tic, 32-unit directional strafe bursts; double-tap Use remains elsewhere.
- Changed forward/back from an 88px linear ramp to a 140px quadratic ramp while
  retaining the one-pixel guard and 4-to-50 output bounds; F9 turning is intact.
- The full-width asynchronous F11 build passes with `__end__=0x2004930c`,
  224,500 zone bytes, no QMI8658 input symbols, and UF2 SHA-256
  `ec00e1262a4a3f53f93c9cb09cd6fc49b4e39ef4d9fe2ae820f30d1f1eba2cd6`.
- Hardware validation remains pending.
- Commit: included in this corner-dodge-and-movement-curve commit.

### 00:01 — Add deliberate touch-gated proportional roll strafe

- Recorded F9 pointing-finger touch as the accepted navigation baseline and
  left every touch response value unchanged.
- Redesigned optional roll strafing so it can act only while touch is held;
  release guarantees zero sidemove and provides a stable neutral-learning
  window, while active play freezes the completed reference.
- Added a two-tic 10-degree entry, 5-degree stop hysteresis, and proportional
  8-to-32 output reaching full response around 22 degrees.
- The full-width asynchronous F10 build passes with `__end__=0x20049330`,
  224,464 zone bytes, and UF2 SHA-256
  `af30c8362cb6cd837bedb560e71998057f7f577a40e6fc28c5e9174c04777816`.
- Physical direction, stop reliability, and combat value remain to be tested.
- Commit: included in this touch-gated-roll-strafe commit.

## 2026-08-17

### 23:53 — Accelerate large pointing-finger turns

- Recorded the pointing finger as the intended relative-control contact and F8's small-turn precision as directionally accepted.
- Kept the one-pixel guard, 48-unit initial turn, quadratic curve, and 112px range; raised only outer maximum turn from 640 to 960.
- Forward/back, touch driver, diagonals, double-tap Use, tap-fire, and release-to-stop are unchanged.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `b8e94eebb5310750031883db351b14e019e67703e17c72d826726b6d19b97820`.
- Hardware testing remains pending.
- Commit: included in this faster-outer-turn commit.

### 23:40 — Expand touch ranges for pointing-finger precision

- Recorded that F7 enables subtle pointing-finger input, validating the Active/coherent driver direction, while a broad thumb remains less reliable; a stable reported contact point across a changing broad contact is the leading inference rather than a measured cause.
- Identified excessive gain as the new pointing-finger problem: maximum movement at 44px and maximum turning at 56px are reached within a few millimetres.
- Doubled movement full scale to 88px and turn full scale to 112px; one-pixel guards, initial outputs, curves, maximums, diagonals, and release-to-stop are unchanged.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `7f8f3e3df65668e23cbb0bf0579b42b4be83f933c6313d45fdf388188e9a8dfc`.
- Hardware testing remains pending.
- Commit: included in this pointing-finger-precision commit.

### 23:30 — Repair FT3168 point tracking before replacing controls

- Traced F6's roughly centimetre-scale physical delay below its one-pixel software guard: the inherited driver selected FT3168 Monitor mode for point tracking and sampled finger/X/Y separately.
- Point mode now selects continuous Active tracking; gesture mode retains Monitor mode.
- Replaced four touch I2C reads with one coherent five-byte `0x02..0x06` burst and updated every caller to consume its returned finger count.
- Both firmware targets build; Doom text shrinks by 104 bytes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols. UF2 SHA-256 is `7cf7565432d02df9643d05de79d98a57cb01eb3f08933e3082286522c1102579`.
- Hardware testing remains pending; F6 mapping is deliberately unchanged. Commit: included in this active-touch-driver commit.

### 22:53 — Minimise touch dead zones without adding an activation jump

- Recorded F5's progressive movement as directionally correct, but both axes still felt delayed around their 4px/6px dead zones and aggressive at activation.
- Reduced horizontal and vertical dead zones to a one-pixel jitter guard, allowing output from the next coordinate step.
- Lowered minimum movement from 8 to 4 and minimum turn from 80 to 48, preserving the established curves, full-scale distances, maximum speeds, diagonals, and release-to-stop.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `66f33d5c59e809047c9e8748184fa3a320d28fe33f3f34c99ca55508e40c5efe`.
- Hardware testing remains pending.
- Commit: included in this near-zero-dead-zone commit.

### 22:44 — Make compact vertical movement immediate and progressive

- Recorded that F4 improved left/right responsiveness, while forward/back still waited through a larger 10px dead zone and then jumped to fixed 25-unit walking.
- Kept F4's horizontal response and independent diagonal move-plus-turn composition unchanged.
- Reduced the vertical dead zone to 4px and added a linear 8-to-50 movement ramp reaching full scale at 44px, with release still guaranteeing stop.
- The full-width asynchronous build passes with unchanged `__end__=0x20049308`, 224,504 zone bytes, and no IMU symbols; UF2 SHA-256 is `749e7fea1b0fea663d797b07135c2b89e543dfe0cec37f422f7cabee78c4b13f`.
- Hardware testing remains pending.
- Commit: included in this progressive-touch-movement commit.

### 22:30 — Strengthen short compact combat swipes

- Recorded F3 as playable through E1M1 with good forward/back control, but too unresponsive to short left/right swipes during E1M2 combat.
- Kept the compact 56px range and bounded 640 maximum; reduced horizontal dead zone from 8 to 6px, raised minimum from 64 to 80, and changed cubic response to quadratic.
- Forward/back, diagonal composition, release-to-stop, double-tap Use, tap-fire, and the compiled-out IMU are unchanged.
- Full-width build passes with unchanged `__end__=0x20049308` and 224,504 zone bytes; UF2 SHA-256 is `9c8783efa353573284afc8fec59f7241a927c7ee514aeef42e6e0e89d71a7186`.
- Commit: included in this responsive-compact-turning commit.

### 21:59 — Compress touch turning into a bottom-left grip

- Recorded F2 as the easiest playthrough so far; the remaining major issue was excessive horizontal finger travel obscuring the full-screen view.
- Kept the floating touch-down anchor so the player can choose the bottom-left without a fixed overlay or restricted screen region.
- Compressed full turn response from 120 to 56px, reduced dead zone from 10 to 8px and minimum from 80 to 64, kept the 640 maximum, and changed quadratic response to cubic for fine control near neutral.
- Forward/back, diagonals, release-to-stop, double-tap Use, tap-fire, and the compiled-out IMU remain unchanged.
- Full-width build passes with unchanged `__end__=0x20049308` and 224,504 zone bytes; UF2 SHA-256 is `c81069f72199bc11a5d251c3d8ef06d5ac8237db925625f778124f29b3c357f4`.
- Commit: included in this compact-touch-control commit.

### 21:35 — Disable roll and slow touch turning

- Recorded that touch navigation feels better, but the F1 roll path caused uncommanded left/right strafing with no finger down and is rejected for the active build.
- Split roll behind default-off `DOOM_ROLL_STRAFE`; the touch-only build performs no IMU initialisation, reads, or `sidemove` work.
- Reduced touch turn maximum from 960 to 640, minimum from 120 to 80, widened the dead zone from 8 to 10px, and moved full scale from 96 to 120px; forward/back is unchanged.
- Touch-only, opt-in roll, and fallback full-width builds pass. Touch-only `__end__=0x20049308`, leaving 224,504 zone bytes; UF2 SHA-256 is `c0a9fc35cc6ccdacb536d1842bbf8168eb079e4582fd5257a0b1d9d55dbe4a12`.
- Commit: included in this touch-only tuning commit.

### 21:16 — Make touch primary and demote motion to roll strafing

- Recorded the fixed-neutral pitch test as rejected: it remained difficult to start and stop reliably and could turn an attempted stop into reverse movement.
- Added simultaneous vertical touch movement and horizontal touch turning, with release as a guaranteed stop; reduced maximum turn response from 1600 to 960.
- Replaced pitch locomotion with optional direct-position roll strafing: stable neutral calibration, 6-degree two-tic start, 3-degree stop, and no delayed software low-pass.
- Both hybrid and fallback full-width builds pass. Hybrid `__end__=0x20049330`, leaving 224,464 zone bytes; fallback remains `0x200492e8`; candidate UF2 SHA-256 is `5a8c5498bf5258174d5dca86f12a9bdd3242b7b009d58601fb14d742f18104bf`.
- Commit: included in this touch-first roll-strafe control commit.

### 20:49 — Replace the rejected tilt gearbox with fixed-neutral zones

- Recorded the second hardware result: pitch direction was inverted and stateful rebasing caused delayed, unpredictable movement with unreliable stopping.
- Replaced latching/rebasing with one stable 18-tic neutral calibration and direct forward/stop/back position zones.
- Added hysteresis: fixed normal walking starts near 1.5 degrees and stops inside roughly 0.6 degrees; inverted the physically observed direction.
- Clarified that BOOTSEL remains unplug, hold BOOT, reconnect, release after about two seconds; holding through host detection was unnecessary.
- Both hybrid and fallback full-width builds pass. Hybrid `__end__=0x20049334`, leaving 224,460 zone bytes; UF2 SHA-256 is `03b9ec7f4bd00c8f375f217d765a71f3d6cf328eda4f76cbefab6767b2935866`.
- Commit: included in this fixed-neutral tilt-controls commit.

### 20:25 — Replace absolute tilt with a three-state movement gesture

- Recorded the first hardware result: touch turning felt responsive, double-tap opened doors, proportional tilt required too much angle, and held PWR conflicted with PMIC long-press/restart behavior.
- Replaced single-axis proportional tilt with XYZ sampling and signed X/Z gravity-vector rotation around the player's settled grip.
- Added a one-gesture-per-state reverse/stopped/forward controller at normal Doom walk speed; five stable tics rebase and prevent a continuous sweep from jumping through stop.
- Changed in-level PWR to immediate tap-fire only and removed all gameplay release/long-press behavior; Escape remains deliberately unresolved.
- Both hybrid and fallback full-width builds pass. Hybrid `__end__=0x20049338`, leaving 224,456 zone bytes; UF2 SHA-256 is `347c1484fe67ab3b338ed1b366fba25dfe4f49ffa8339e3f84ebef777c68eb35`.
- Commit: included in this stateful-tilt-controls commit.

### 20:02 — Build a hybrid motion-and-touch control prototype

- Added a compile-selectable hybrid model: QMI8658 pitch movement, proportional horizontal touch turning, touchscreen double-tap Use/Open, held PWR fire, and long-PWR Escape/menu.
- Added a minimal fixed-memory accelerometer driver at +/-2g and 62.5Hz, with 18-tic neutral calibration and fixed-point low-pass filtering; gyro remains intentionally disabled.
- Enabled AXP2101 PWRON press/release edge IRQs so fire no longer waits for the old 400ms single/double-click decision.
- Kept the floating swipe-and-hold model as the fallback and preserved its menu behavior. Both hybrid and fallback full-width Release builds pass.
- Measured the hybrid linker endpoint at `0x20049318`: 48 static bytes above baseline and 224,488 bytes of exact zone headroom. Hardware control validation is pending.
- Commit: included in this hybrid-control prototype commit.

### 19:47 — Lock full-width video at 35.2 FPS and install the normal build

- Recovered and checksum-validated the final one-minute report: 2,110 frames in 60.004 seconds, or 35.2 FPS.
- Measured average/max cadence 28,451/46,503us, presentation 22,226/27,049us, CPU preparation 20,709us, blocking display service 1,516/3,843us, display wait at most 11us, and zero DMA timeouts.
- Improved cadence by 18.3% and presentation by 23.2% versus the original 28.7 FPS synchronous full-width baseline, with unchanged static presentation memory.
- Replaced the profiler with the normal 448x280 asynchronous build, flashed and byte-verified as SHA-256 `c67fb48fc5ee27c5b348f5c532e316f06bc496b7981a09bd84460ef624d91920`; regular play no longer auto-reboots into reports.
- Commit: included in this locked-full-width-performance milestone commit.

### 19:38 — Clear 35 FPS in the paired-row hardware capture

- Recovered and checksum-validated the 20-second paired-row report: 727 frames in 20.009 seconds, or 36.3 FPS.
- Measured average/max cadence 27,560/43,794us and presentation 21,842/27,674us; CPU preparation fell to 20,404us while blocking display service stayed at 1,437us.
- Improved cadence by 2,104us and CPU preparation by 2,000us versus the 33.7 FPS pipeline baseline, with display wait at most 9us and zero DMA timeouts.
- Accepted the CPU optimisation directionally and selected one final 60-second capture as the video-performance lock gate.
- Commit: included in this paired-row hardware-result commit.

### 18:53 — Build a shorter paired-row CPU-packing experiment

- Added a selectable profiler duration so intermediate comparisons can take 20 seconds while final validation remains 60 seconds; changing it recompiles only the video source.
- Packed vertically duplicated output rows in one strided x loop when both remain in the same 20-row tile, removing repeated scaled-row loads and loop control without another buffer.
- Verified exact old/new row mapping at 384x240, 416x260, and 448x280; all normal/profiled synchronous and asynchronous configurations build with unchanged linker endpoints.
- Built the profiled 448x280 asynchronous candidate for a 20-second hardware comparison; UF2 SHA-256 is `d2f6ec21b94c2fbba57c4ff6a376c9e9bf8d56b21f462053bf59a96c0d374186`.
- Commit: included in this paired-row-packing candidate commit.

### 18:48 — Measure full-width asynchronous presentation at 33.7 FPS

- Recovered and checksum-validated the asynchronous one-minute combat report: 2,024 frames in 60.012 seconds, or 33.7 presented FPS.
- Improved average cadence from 34,838us to 29,664us (14.9%) and presentation from 28,934us to 23,940us, with zero DMA timeouts.
- Reduced blocking display service from 7,130us to 1,535us, hiding about 78% of the old transfer cost while retaining identical static-memory endpoints.
- Selected the remaining 22,404us CPU compose/scale/transpose work as the next target; about 1.1ms more reaches the 35 FPS simulation rate.
- Commit: included in this measured-asynchronous-performance commit.

### 18:43 — Hardware-validate the asynchronous screen and audio path

- Flashed and byte-verified the profiled 448x280 two-buffer candidate, then rebooted it into application mode.
- Alexander confirmed that the Doom menu is visible, updates normally, and sounds good; the previously stretched menu effects were not reported.
- Promoted the asynchronous presenter from build-only to hardware-valid for screen/menu/audio behavior. The one-minute combat timing comparison remains the next gate.
- Commit: included in this asynchronous-hardware-validation commit.

### 17:56 — Build a memory-neutral asynchronous full-width presenter

- Split the full-width presentation tile into two 20-row buffers when `DOOM_ASYNC_AMOLED=ON`; their combined 35,840 bytes exactly match the proven single 40-row buffer.
- Added paired packed-transfer start/wait APIs that retain display mutex and chip-select ownership while DMA runs, allowing core1 to pack the alternate buffer safely.
- Kept the synchronous 40-row presenter as the default and enforced the asynchronous candidate's memory-neutral 20-row configuration at CMake and compile time.
- Built normal and profiled 320x200/448x280 synchronous images plus normal and profiled 448x280 asynchronous images. Full-width linker endpoints remain exactly `0x200492e8` and `0x20049750`; hardware validation is pending.
- Commit: included in this asynchronous-presentation-pipeline commit.

### 17:43 — Interleave audio service with full-width display transfers

- Recorded Alexander's experience assessment: full-width gameplay was playable to slightly sluggish, while menu effects lagged and sounded stretched.
- Matched that symptom to the measured schedule: two 512-sample buffers cover 23.2ms, but full-width presentation averages 28.9ms and can take 35.2ms.
- Added a non-blocking mixer refill opportunity after each completed 40-row display transfer, without adding audio buffers, static memory, or queue latency.
- Built normal and profiled 320x200 and 448x280 images; linker endpoints are unchanged. Hardware listening remains pending.
- Commit: included in this interleaved-audio-service commit.

### 17:16 — Validate full-width combat at 28.7 FPS

- Flashed, byte-verified, and visually confirmed the hardware-proven 40-row presenter at full-width 448x280; Alexander confirmed the full-screen view rendered correctly.
- Recovered a checksum-valid one-minute combat report with 1,724 frames in 60.026 seconds, or 28.7 presented FPS.
- Measured average/max cadence 34,838/46,520us, presentation 28,934/35,225us, CPU preparation 21,803us, transfer 7,130/7,600us, render last/max 22,254/31,316us, display wait at most 9us, and zero DMA timeouts.
- Established that full width is already close to 30 FPS: removing about 1.5ms, or 4.3% of average cadence, reaches that target. CPU preparation remains the main target; DMA and panel reliability remain healthy.
- Commit: included in this measured-full-width-baseline documentation commit.

### 16:38 — Reject the black-screen small tile and move the next test to full width

- Flashed and byte-verified the instrumented 320x200 8-row candidate; its application USB returned but Alexander observed a black screen, so no performance capture was attempted.
- Restored 40 rows as the hardware-proven default while retaining other tile heights only as explicit driver experiments.
- Built the reliable 448x280 40-row profiler for the next hardware test; it links at `__end__=0x20049750` with 223,408 bytes of exact zone headroom.
- Clarified that 320x200 was used only to isolate one optimization against a measured baseline; subsequent experience testing moves to the full-width target.
- Commit: included in this failed-small-tile follow-up commit.

### 16:23 — Build the first cache-local transpose candidate

- Made transpose-tile height an explicit CMake comparison variable, retaining the original 40-row path while selecting 8 rows automatically for 320/384/448 and 10 for 416.
- Reduced the normal 320x200 tile from 25,600 to 5,120 bytes and moved `__end__` from `0x20046ae8` to `0x20041ae8`, increasing exact zone headroom from 234,776 to 255,256 bytes.
- Reduced the normal 448x280 tile from 35,840 to 7,168 bytes and moved `__end__` from `0x200492e8` to `0x200422e8`, increasing exact zone headroom from 224,536 to 253,208 bytes.
- Built normal and instrumented 320x200 and 448x280 Release images successfully. Hardware must still verify image correctness and whether shorter-stride writes outweigh the additional transfer setup.
- Commit: included in this selectable-small-transpose-tile commit.

### 16:18 — Capture the 320x200 one-minute combat baseline

- Recovered and checksum-validated the autonomous 100-byte report after Alexander completed the warm-up and one-minute combat run; the board returned to Doom without manual BOOTSEL handling.
- Measured 2,507 presented frames in 60.015 seconds (41.8 FPS), with average/max cadence 23,948/42,958us and average/max presentation 15,460/20,351us.
- Split average presentation into 11,664us CPU preparation and 3,795us panel transfer; core1 averaged 7,091us, frame wait averaged 1,367us, display wait peaked at 8us, and no DMA timeout occurred.
- Selected compose/scale/transpose locality and tile size as the first optimisation target. The healthy DMA/panel path does not justify a wholesale driver rewrite yet.
- Commit: included in this measured-combat-baseline documentation commit.

### 16:12 — Extend profiling to a true one-minute combat run

- Hardware-confirmed the first 320x200 flight recorder reboot/report path and recovered its 384-frame non-combat baseline from the reserved flash log.
- Replaced the frame-count cutoff with a 3-second level warm-up plus 60 seconds of wall-clock in-level capture so presentation rate cannot shorten the test.
- Persisted the checksum-validated report only after reboot, single-core and before USB/audio/Doom startup, in the reserved `0x101ff000..0x101fffff` sector immediately before the WHD at `0x10200000`.
- Built 320x200 and 448x280 Release profilers; their exact zone headroom is 233,648 and 223,408 bytes. The normal non-profile builds remain unchanged.
- Commit: included in this one-minute persistent-profiler commit.

### 15:55 — Replace live USB telemetry with a short flight recorder

- Identified duplicate TinyUSB initialisation as the concrete leading cause of the dead runtime CDC/reset interface: the reusable Waveshare module and Doom's entry point both called `stdio_init_all()` during one boot; hardware confirmation is pending.
- Moved stdio ownership to executable entry points and retained the profiling build's bounded 2ms USB output timeout.
- Replaced continuous gameplay serial dependence with a 384-frame real-level-only capture that checksum-saves full aggregate timing in reset-retained SRAM, watchdog-reboots, and repeats the report over quiet USB while cycling key values on the AMOLED.
- Built 320x200 and 448x280 flight-recorder images with ARM GNU 15.3. The profiler leaves 234,596 and 224,356 zone bytes respectively; hardware flashing and capture remain pending.
- Commit: included in this reset-persistent profiling-workflow commit.

### 15:30 — Build the first measured full-width video milestone

- Added compile-out timing instrumentation for game work, rendering, core rendezvous, presentation packing, AMOLED transfer time, presented-frame cadence, and display DMA timeout recovery.
- Reworked the presenter into one fused compose/scale/transpose path with selectable 320x200, 384x240, 416x260, and 448x280 outputs; scaled modes compose each Doom source row once and use exact division-free nearest-neighbour accumulators.
- Built every mode with the same Release toolchain. The normal 320x200 image retains the exact `__end__=0x20046ae8` baseline; full-width 448x280 leaves 224,536 bytes of short-pointer-zone headroom, only 10,240 bytes below baseline.
- Built instrumented 320x200 and 448x280 comparison images. Hardware flashing is pending because the long-running application currently enumerates its USB Reset interface but stalls both picotool and direct reset requests; the verified safe firmware remains installed.
- Commit: included in this performance-instrumentation and scalable-presentation commit.

### 14:50 — Set the enjoyable-handheld roadmap and verify autonomous flashing

- Reframed success from merely running Doom to an enjoyable, completable handheld experience.
- Added a measured performance-first roadmap toward full-width video, followed by horizontal-touch plus pitch-tilt and alternate motion-control experiments.
- Recorded the successful E1M1-to-E1M2 run, byte-identical SDK 2.3.0 rebuild, durable full-flash backup, verified `picotool -f` deployment, and normal post-flash operation.
- Documented later battery, port-settings, HUD, and optional music work without promoting them ahead of controls.
- Commit: included in this experience-roadmap documentation commit

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
