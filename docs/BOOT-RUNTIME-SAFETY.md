# Runtime BOOT Safety Audit

Status: **F16 BOOT tap/hold controls are installed and hardware-accepted**.
Short release cycles weapon; hold modifies LEFT/RIGHT touch into strafe during
local single-player play. F15 remains the byte-exact release-only rollback;
F14.1 remains the no-runtime-BOOT rollback.

## F15.1 bounded hold-strafe extension

F15.1 changes interpretation, not the flash transaction. It retains F15's
25 ms sampling period, SRAM callback, SDK multicore coordination, 2 ms timeout,
and two-sample debounce. Flash CS is still floated only during each brief
sample; it is not held high-impedance while the physical button remains down.
All active DMA sources remain in SRAM.

`DOOM_BOOT_HOLD_STRAFE` defaults off and requires both F15 BOOT input and the
F14 thumb zones. With it off, the resulting UF2 remains byte-identical to F15:
`769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`.
With it on, a short release remains next weapon. At 250 ms after the debounced
press—roughly 300 ms physically—the modifier activates. LEFT/RIGHT become
held strafe, the forward transition bands become forward-strafe, and UP/DOWN
remain unchanged. A resolved hold suppresses weapon cycling on release.

Static audit of candidate
`6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`:

- `read_bootsel_raw=0x200008c4` and `audio_dma_irq_handler=0x20001858`;
- `audio_buffers=0x2001deb8`, `panel_chunks=0x2003d7c8`, and
  `silence_buffer=0x20046cac`, all in SRAM;
- `__end__=0x20049bf0`, leaving 220,176 bytes after the fixed heap before
  `__StackLimit=0x20080000`.

First physical test remains deliberately bounded:

1. Use USB power only and keep the board visible on a clean, non-conductive
   surface. Exact F15 remains available for immediate rollback.
2. Confirm normal Doom menu, sound, and E1M1 before pressing BOOT.
3. Briefly press/release BOOT once and confirm one weapon change.
4. Hold BOOT until the modifier threshold, touch LEFT briefly, then release
   touch and BOOT. Confirm left strafe, no weapon change, and restored turning.
5. Repeat once with RIGHT. Stop immediately on unexpected audio, reset, freeze,
   heat, odour, smoke, or visible damage; do not repeat after an abnormality.

### Hardware result — passed 2026-08-18

The candidate booted with normal display and sound. Short BOOT still changed
weapon; held BOOT plus LEFT/RIGHT strafed as intended, and release restored
turning. Alexander reported that it worked great, with no abnormal behavior.
This admits the hold vocabulary as F16. It does not by itself establish
long-session stability, which still needs ordinary combat play.

## Why BOOT is different from a normal button

The board schematic connects Key2 (BOOT) to the external W25Q128 flash
chip-select net, `QSPI_SS_N`, through a 1 kΩ resistor. Pressing the switch is a
designed pull-down, not a connection across a power rail. The schematic also
shows no direct path from Key2 to GPIO19, the NS4150B speaker amplifier, or the
audio power rails:

- <https://files.waveshare.com/wiki/RP2350-Touch-AMOLED-1.8/RP2350-Touch-AMOLED-1.8.pdf>
- <https://www.waveshare.com/wiki/RP2350-Touch-AMOLED-1.8>

This makes a literal BOOT-induced supply short unlikely. It does **not** prove
the two earlier abnormal Doom runs were harmless or explain the reported
burning odour. Both BOOT-enabled builds behaved abnormally and the otherwise
equivalent rollback did not, so runtime flash interaction remains the common
correlated change.

Raspberry Pi officially demonstrates BOOTSEL as a runtime input. Its example
runs the sampler from SRAM, disables interrupts, temporarily makes flash CS
high-impedance, reads the QSPI CS input, restores the output override, and then
returns to flash. The same example explicitly warns that this fails if another
core or an XIP streamer accesses flash concurrently:

- <https://github.com/raspberrypi/pico-examples/blob/master/picoboard/button/button.c>
- <https://github.com/raspberrypi/pico-examples>

## The missing hazard in the earlier Doom implementation

The previous BOOT experiment paused core 1 with raw multicore lockout and
disabled core 0 interrupts while sampling. That covered CPU instruction and
WAD reads, but it did not audit autonomous DMA reads.

The current executable proves one important unsafe source:

| Runtime reader | Source | Link address | BOOT implication |
| --- | --- | ---: | --- |
| Core 0 | code and WAD/lumps | `0x100...` XIP | must be inside the SDK safe zone |
| Core 1 | render code and WAD/lumps | `0x100...` XIP | must register with `flash_safe_execute_core_init()` |
| AMOLED DMA | `panel_chunks` | `0x2003d6f4` SRAM | does not read external flash |
| Audio DMA, queued SFX | `audio_buffers` | `0x2001ddf8` SRAM | does not read external flash |
| Audio DMA, empty queue | `silence_buffer` | `0x1005a1ec` XIP flash | **unsafe while CS is floated** |
| Pico-net DMA | fixed work buffers | `0x200...` SRAM | no XIP source, but BOOT must remain disabled in network modes |

`flash_safe_execute()` coordinates CPU cores and disables the calling core's
interrupts; it does not stop an arbitrary DMA/XIP reader. The flash-resident
audio silence buffer therefore gives a concrete mechanism by which the old
sampler could corrupt the audio stream while the queue was empty. This is a
plausible explanation for abnormal loud audio. It does not prove the cause of
the odour.

The SDK contract is stricter and clearer than the previous custom wrapper:

- `flash_safe_execute()` calls the RAM callback only after it has established
  a safe CPU/IRQ state, otherwise it returns an error.
- A core launched with `multicore_launch_core1()` must call
  `flash_safe_execute_core_init()` on that core.
- The caller must separately ensure that DMA or other XIP streamers do not read
  flash during the callback.

Official reference:
<https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_flash>

## Gate A: isolated probe

`boot_safety_probe` is the only image currently allowed to sample BOOT. It:

- runs on core 0 only;
- forces GPIO19 low before waiting, physically disabling the speaker amp;
- forces AMOLED power GPIO17 low, so the screen is intentionally black;
- leaves I2S GPIO20-24 as high-impedance inputs;
- initializes no codec, PIO state machine, DMA, I2C, display, touch, IMU,
  audio, USB stdio, or second core;
- samples at 20 Hz through `flash_safe_execute()` with a 2 ms safety timeout;
- requires two pressed samples and two released samples; and
- enters ROM BOOTSEL only after release, giving the host an unambiguous success
  signal through `picotool`.

The build's SRAM callback was checked in the ELF at `0x20000110`; its
disassembly contains only direct QSPI/SIO register access, the bounded delay,
the SRAM result write, and return. The prepared UF2 SHA-256 is
`05ca114df6c699d4deeb4af733e000ebdafe22dfae286ad25ea5bbcaeb65c04f`.

### Gate A hardware result — passed 2026-08-18

The probe was installed over USB with no battery. Its intentionally black,
audio-disabled application booted, one brief BOOT press/release was detected,
and the firmware entered ROM BOOTSEL exactly as designed. `picotool info`
confirmed `boot type: bootsel`, RP2350 A2, and chip ID
`05bcf1c1aa06aa58`. There was no unexpected audio during the bounded test.

The byte-verified F14.1 Doom image was then restored and rebooted into
application mode; its UF2 SHA-256 remains
`821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
Gate A proves the exact circuit and SDK wrapper can detect one bounded press in
the isolated configuration. It does not yet prove safety with Doom's second
core, rendering, or audio active.

### Physical test protocol

1. Use USB only; disconnect the battery if one is attached.
2. Keep the bare board visible on a clean, non-conductive, non-flammable
   surface. Do not hold it close to the face or intentionally smell it.
3. Flash only `boot_safety_probe.uf2`. A black screen is expected because the
   display and amplifier are deliberately disabled.
4. Briefly press BOOT once and release it. Do not test a hold, double press, or
   repeated tapping in this gate.
5. Success means the board enters ROM BOOTSEL and `picotool` can identify it.
6. Disconnect USB immediately if there is unexpected sound, smoke, odour,
   visible damage, or rapid heating. Do not retry the probe after any of these.
7. Restore the exact F14.1 Doom image after recording the result.

## Gate B: default-off Doom integration

Only after Gate A passes:

1. Move `silence_buffer` into SRAM (or otherwise stop and prove idle every
   flash-reading DMA channel before a sample).
2. Add `flash_safe_execute_core_init()` at the start of Doom's core 1 before it
   signals `core1_launch` ready.
3. Add a default-off `DOOM_BOOT_NEXT_WEAPON` build option. Keep the accepted
   F14.1 binary path unchanged when it is off.
4. Sample only during a single-player `GS_LEVEL`; do not sample in menus,
   startup, profiling/report boots, or network play.
5. Treat a debounced short press as pending input and queue one forward cycle
   through `G_BuildTiccmd()` only after release. Do not invent weapon state or
   bypass Doom's selectable-weapon order, ownership, shareware, or
   pending-weapon rules.
6. Skip a sample on every non-`PICO_OK` safety result. Never guess that a failed
   sample means pressed.
7. Do not add BOOT hold or double-click actions. First prove one release-based
   next-weapon event.

### Gate B implementation and binary audit — 2026-08-18

The default-off `DOOM_BOOT_NEXT_WEAPON` candidate implements the gate above:

- normal builds contain no BOOT sampler; enabling it requires hybrid controls;
- core 1 calls `flash_safe_execute_core_init()` before declaring itself ready;
- core 0 samples at most every 25 ms through `flash_safe_execute()` with a
  2 ms timeout, requires two pressed and two released samples, skips every
  failed safety handshake, and acts only after release;
- sampling is limited to a local single-player `GS_LEVEL`, never menus,
  startup, reports, or network play;
- one release sets Doom's existing forward `next_weapon` request, so
  `G_BuildTiccmd()` retains the original selection and ownership rules;
- GPIO19 is held low from the first shared hardware initialization; the codec,
  audio PIO and audio DMA are never initialized; and GPIO20-24 are explicit
  high-impedance inputs; and
- the formerly flash-backed 2 KiB silence block moves to SRAM in this candidate
  only. It is inert because audio initialization returns false, but its
  placement removes the known XIP-DMA hazard before any physical test.

The Release ELF places `read_bootsel_raw` in SRAM at `0x200008c4`,
`silence_buffer` in SRAM at `0x20046880`, `audio_buffers` at `0x2001da98`, and
the display DMA source `panel_chunks` at `0x2003d3a0`. The audio initializer's
disassembly contains only amplifier/pin shutdown and returns false; it has no
calls to codec or audio-DMA initialization. The image ends at
`__end__=0x200497bc`; after the fixed 2 KiB heap it leaves 221,252 bytes before
core 1's stack boundary at `0x20080000`. Candidate UF2 SHA-256:
`891d6076db58253064dba38c4634322ac6109095915737243f38852de7d36076`.

Rebuilding the option-off F14.1 configuration reproduced its exact known-good
UF2 SHA-256
`821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
This proves the candidate remains isolated behind its build switch. Static
audit is not physical proof: Gate C still begins silent and stops immediately
on unexpected audio, heat, odour, reset, freeze, or display corruption.

## Gate C: staged game hardware tests

1. First BOOT-enabled Doom test: amplifier disabled and no audio subsystem.
2. Confirm menu, E1M1 load, one short BOOT press, one weapon change, release,
   and several minutes of stable movement/rendering.
3. Restore effects only after the silence buffer is confirmed in SRAM and the
   executable map contains no active flash-backed DMA source.
4. Retest a single short press. Stop on any abnormal audio, heat, odour, reset,
   freeze, or display corruption.
5. Only after both tests pass may the option be considered for the normal
   playable firmware. The safe F14.1 UF2 remains the rollback image throughout.

### Silent Doom hardware result — passed 2026-08-18

The audited Gate B image was installed and flash-verified through application-
mode USB. Doom booted normally with the expected silence. In a level, one brief
BOOT press/release produced the requested single weapon change, and Alexander
reported the result as all good: no display instability or other abnormal
behaviour was observed. This validates the release recogniser and SDK core
lockout while Doom's renderer and display DMA are active; it does not yet
validate simultaneous sound DMA.

The exact F14.1 effects-only image was immediately restored, flash-verified,
and confirmed back in application mode. Its SHA-256 remains
`821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.

### Effects-enabled candidate audit — 2026-08-18

`DOOM_BOOT_WITH_SOUND_EFFECTS` is a second explicit default-off gate and is
rejected unless `DOOM_BOOT_NEXT_WEAPON` is also enabled. The silent candidate
remains the default whenever BOOT input is enabled. Gate C with effects keeps
the existing effects-only backend but always compiles its two possible DMA
sources into SRAM:

| Active DMA/read path | Audited source | Address |
| --- | --- | ---: |
| Audio queued effects | `audio_buffers` | `0x2001deb8` |
| Audio empty queue | `silence_buffer` | `0x20046ca8` |
| AMOLED presentation | `panel_chunks` | `0x2003d7c8` |
| Pico-net (inactive in allowed local level) | `large_buffer_16` / `small_buffer_16` | `0x2003d094` / `0x200474c0` |

The BOOT callback remains in SRAM at `0x200008c4`, and the audio DMA IRQ handler
is also SRAM-resident at `0x20001858`. Disassembly confirms the effects build
initializes the codec and audio DMA; source inspection confirms that DMA can
select only `audio_buffers` or `silence_buffer`. BOOT sampling remains disabled
for network play. The build has `__end__=0x20049be8`; after its fixed 2 KiB heap
it leaves 220,184 bytes before core 1's stack boundary. Candidate SHA-256:
`769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`.

After adding the second gate, both earlier references remain exact: the silent
Gate B candidate rebuilds as
`891d6076db58253064dba38c4634322ac6109095915737243f38852de7d36076`,
and option-off F14.1 rebuilds as
`821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
At this pre-test checkpoint, the effects candidate was statically ready for one
short BOOT press/release with normal sound effects but was not yet installed.

### Effects-enabled Doom hardware result — passed 2026-08-18

The exact audited candidate was installed and flash-verified through the same
application-mode USB path. Doom returned in application mode, normal sound
effects remained present, and a brief BOOT press/release cycled weapons
correctly. Alexander reported that everything appeared normal and that the
button worked well. No sound distortion, display instability, reset, freeze,
heat, or odour was reported.

This completes the staged admission. The installed image is now the F15
playable milestone, with UF2 SHA-256
`769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`.
F14.1 remains the byte-verified rollback. Approval covers only one short
release-based next-weapon action under the existing single-player gate; BOOT
hold, double-click, menu actions, and network-play sampling remain excluded.

## Decision

The isolated probe, silent Doom, and effects-enabled Doom tests all passed.
F15 therefore accepts one short BOOT press/release as next weapon during local
level play. Its SDK multicore coordination, SRAM callback, release debounce,
failed-sample skip, and SRAM-only active DMA sources are mandatory parts of the
feature—not optional hardening. Retain F14.1 as rollback and continue watching
longer sessions, but the staged safety question is closed for this exact action
and configuration.
