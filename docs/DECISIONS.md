# Decisions Log

## 2026-08-18 — Measure and lock the F18 full-panel performance baseline

The optional documentation capture is complete. After the normal three-second
in-level warm-up, the F18 profiler recorded 2,059 presented frames over 60.021
seconds at 448x368: **34.3 FPS**. The checksum-valid report measured average/max
cadence of 29,164/45,612us, presentation of 24,469/31,064us, CPU preparation of
22,570us, blocking transfer service of 1,899/4,312us, frame wait of
1,350/3,808us, display wait at most 10us, and zero DMA timeouts. The last/max
game tic was 34,171/44,087us and last/max render time was 19,396/27,507us.

Compared with the locked 448x280 capture, F18 presents 31.4% more pixels while
average cadence rises only 713us, or 2.5% (35.2 to 34.3 FPS). Presentation work
rises by 2,243us, but the async pipeline continues to hide enough work that the
whole-frame cadence changes much less. The gameplay routes were not identical,
so subsystem deltas should not be treated as a microbenchmark; the bounded
result is evidence that the accepted full-panel experience retains the target
smoothness and a healthy display path.

The profiling build linked at `__end__=0x2004a060`, leaving 219,040 zone bytes.
It was used only for the bounded capture. The accepted normal F18 image (SHA-256
`5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`)
was restored and programmer-verified afterwards; its `__end__=0x20049bf0` and
220,176-byte zone remain the release baseline. Close display optimisation unless
future normal play exposes a concrete regression.

## 2026-08-18 — Accept F18 full-panel Doom as the core experience

The installed 448x368 build passed its experience gate. Alexander described it
as the version he loves and wants to keep as the project's core experience.
The image fills the complete AMOLED, feels really good in play, and did not
introduce an observable smoothness problem during the acceptance run.

This is an intentional experience decision, not a claim that 448x368 is
aspect-correct or already benchmarked. It stretches the exact-4:3 448x336
presentation by 9.5% vertically, but the physical result is preferred on this
small device. F17 448x336 remains the exact-4:3 rollback, 448x320 the second
visual rollback, and measured 448x280/35.2 FPS the performance rollback.

Accept installed UF2 SHA-256
`5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`
as F18. It retains `__end__=0x20049bf0`, 220,176 Doom-zone bytes, effects-only
audio, and all F17 controls. A bounded 448x368 combat capture remains useful
for documentation and future comparison, but it is no longer an acceptance
gate and should not reopen optimisation unless play exposes a real regression.

## 2026-08-18 — Build a bounded 448x368 full-panel experience candidate

Alexander prefers using the remaining 16-pixel bands for the game rather than
reserving permanent port UI. F18 therefore scales the complete 320x200 Doom
frame to the panel's native 448x368 area. This fills every pixel but is a
deliberate 9.5% vertical stretch beyond the accepted 448x336 4:3 presentation.
Aspect-preserving fill would instead crop roughly 21 pixels from each side and
risk losing HUD information; a renderer/FOV redesign is a much larger project.
The stretch is the smallest reversible way to judge the experience on hardware.

The panel path keeps the two existing 20-row buffers. After presenting rows
340..359, it waits for that buffer, combines its final 12 rows with rows
360..367 already packed in the other buffer, and submits rows 348..367 as one
overlapping 20-row transaction. Twelve rows are transferred twice, but no full
framebuffer, third tile, or static allocation is added. The configuration is
admitted only for asynchronous 448x368 with 20-row tiles.

The changing coloured strip photographed in the old top-right black band is
most consistent with stale panel GRAM, not Doom content: the one-time clear
ended with an 8-row transfer, and this hardware previously produced a black
panel with 8-row transactions. The clear now ends with a full 20-row transfer
overlapping the preceding stripe by 12 rows. This is a strong diagnosis, not a
confirmed root cause until the physical panel no longer shows the artefact.

Both 448x368 and a fresh 448x336 regression configuration build. F18 retains
`__end__=0x20049bf0` and 220,176 zone bytes; its UF2 SHA-256 is
`5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`.
Preserve the F17 UF2 as rollback. F18 was flashed and byte-verified through
autonomous `picotool -f`, then rebooted to application mode. It remains a
candidate until a short physical gate checks full-panel coverage, geometry,
DOWN reachability, effects audio, smoothness, and removal of the coloured edge
remnant.

## 2026-08-18 — Keep fixed controls across menus and intermissions

The accepted F16 mapping was only active while `gamestate == GS_LEVEL` and the
menu was closed. Every other state silently returned to the old floating-swipe
path. This explains both newly observed failures: menus navigated by swipe
instead of the visible fixed D-pad, and touch on the E1M1 score screen could
only emit arrow keys. A PWR tap also became Enter there, while vanilla
`WI_checkForAccelerate()` listens only for the player's native Attack or Use
button bits. The game was responsive and not frozen; no valid advance command
was reaching the intermission state machine.

F17 keeps gameplay movement unchanged, translates the same absolute thumb
zones to cardinal menu keys whenever a menu is active, and translates a fresh
post-release D-pad contact to Fire during intermission. The release requirement
prevents a movement contact held across the level exit from skipping stats.
PWR also resolves to Fire during intermission while retaining Enter in menus
and the established 450 ms Escape behavior everywhere. This restores Doom's
native progression mechanism without changing its timing or level logic.

The taller view exposed a separate geometry coupling. DOWN remained fixed at
physical `y=304`, so only 20 pixels overlapped the 448x280 image but 48 pixels
overlapped 448x336. At every 448-wide mode its threshold is now derived from
the centred view so exactly 20 image pixels remain visible; the bottom black
border continues to extend the tactile target. At 448x336 this moves the
threshold to `y=332`, giving 20 visible pixels plus the 16-pixel border.

The 448x336 Release candidate builds with unchanged `__end__=0x20049bf0` and
220,176 zone bytes. Its UF2 SHA-256 is
`8605a47aa53b71c65ba96602c1179f28d3a7b11420691db01e5574c32ad2a324`.
The flashed build passed its physical gate. Menu touch now uses the fixed zones,
the score screen advances normally, and DOWN is restored to the intended
shallow presentation. Accept F17 as the installed controls baseline.

The accidental menu swipe was nevertheless a good interaction in its own
right. Preserve that finding as a future controlled comparison: a menu-only
swipe mode may be more fluid than absolute zones, but it must be explicitly
scoped to `menuactive` and must never again become the generic fallback for
intermission or other non-level states.

## 2026-08-18 — Accept 448x336 as the preferred visual baseline

The flashed and verified 448x336 image passed its short physical gate. It
displayed correctly, looked good, and felt very smooth; Alexander could not
observe a performance difference from 448x320. This validates the padded final
20-row transaction on hardware and selects exact 4:3 correction as the
preferred visual baseline.

This is qualitative acceptance, not a measured performance claim. Preserve
exact 448x320 as the visual rollback and measured 448x280 F16 at 35.2 FPS as
the performance rollback. The next engineering gate is one bounded 448x336
combat capture. Do not proceed directly to 448x368: it would add another 9.5%
pixels over 448x336, exceed Doom's intended 4:3 geometry, and conflate
performance with a separate crop/stretch/UI decision.

## 2026-08-18 — Advance from accepted 448x320 to exact 4:3 448x336

The 448x320 firmware was flashed and verified, returned to application mode,
and passed its short physical image/audio/gameplay gate. Alexander described
the result as excellent and visually much better, and asked to test farther.
It is now the installed visual baseline, although its combat cadence has not
yet been measured; exact 448x280 F16 remains the measured 35.2 FPS rollback.

This is not arbitrary full-panel stretching. Doom renders 320x200, but its
original 4:3 CRT presentation used non-square pixels. The square-pixel
equivalent is 320x240; at 448 pixels wide that becomes exactly 448x336. The
successful 448x320 view is therefore a partial correction toward the intended
shape, while 448x336 completes it. Full 448x368 would go beyond that correction
and remains a separate crop/stretch/UI decision.

Decision: admit one bounded padded-final-tile implementation for 448x336. The
last 16 image rows occupy their normal positions in a 20-row transpose buffer;
four unused entries per column are cleared and sent into the adjacent black
border. DMA transaction size, both 20-row buffers, row buffer, renderer, audio,
and controls remain unchanged. The candidate adds 56 flash bytes, no SRAM,
retains `__end__=0x20049bf0` and 220,176 zone bytes, and has UF2 SHA-256
`2a9f7a4ed74392e016fd2407fac1981aa1fb4c8e02c7d9724e0d25a31a913ad8`.
Exact 448x320 and 448x280 rebuild hashes remain unchanged. Hardware-accept only
after the same short correctness gate.

## 2026-08-18 — Test 448x320 before changing asynchronous tile boundaries

The accepted 448x280 view leaves 44-pixel bands above and below the image.
448x336 remains the desirable traditional 4:3 target, but 336 rows are not
divisible by the hardware-proven 20-row asynchronous tile. Testing it directly
would combine a 20% pixel increase with new partial-final-tile transfer logic,
making either a black-screen regression or a pacing change harder to isolate.

Decision: use 448x320 as the first taller presentation gate. It reduces each
band to 24 pixels and emits 14.3% more pixels, while retaining the exact two
20-row buffers, DMA transaction shape, 448-pixel row buffer, audio refill
points, Doom 320x200 renderer, and F16 controls. The candidate builds with the
same text/data/BSS totals and `__end__=0x20049bf0`, leaving the same 220,176
zone bytes as F16. Its UF2 SHA-256 is
`a253683ef45e8e412bcfb35e6a6c1884972f21cd39b653cb15945fcbd5fa8170`.

The source change is rollback-safe: `DOOM_DISPLAY_HEIGHT_OVERRIDE=AUTO`
rebuilds accepted F16 byte-for-byte at
`6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`.
Do not call 448x320 accepted until a short image/audio/gameplay gate passes;
do not add 448x336 partial-tile handling until this isolated height test is
understood.

## 2026-08-18 — Accept BOOT-hold strafing as the F16 controls baseline

The bounded physical test passed. Short BOOT remains next weapon; holding BOOT
and using LEFT/RIGHT produced sustained strafe, and releasing it restored the
ordinary turn mapping. Alexander reported that it worked great. No abnormal
audio, display behavior, reset, freeze, heat, or odour was reported.

Decision: accept the installed candidate as F16 and close essential control
mapping. The previous floating-anchor model was playable but required too much
finger travel and favored a pointing finger; continuous pitch/roll was hard to
start and stop reliably; corner double-tap strafe was technically functional
but impractical. F16's absolute asymmetric zones work with a thumb, preserve
immediate release-to-stop, make the guides useful, and add a classic explicit
strafe modifier without another touch gesture.

Installed UF2 SHA-256 is
`6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`.
It leaves 220,176 zone bytes. Keep exact F15
`769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`
as the release-only BOOT rollback and F14.1 as the no-runtime-BOOT rollback.
Sensitivity and geometry can be refined later, but are no longer the active
engineering subject. Move next to a measured taller presentation candidate.

## 2026-08-18 — Build a gated BOOT-hold strafe-modifier candidate

Alexander identified strafing as the last uncomfortable essential combat
action. The corner double-tap bursts are reachable but not practical during
normal play. A classic modifier is a better fit for the established controls:
hold BOOT and the existing LEFT/RIGHT thumb zones become strafe, while UP/DOWN
remain forward/back and the two forward transition bands become forward plus
strafe. Releasing BOOT restores ordinary turning.

This is a new F15.1 candidate, not an expansion silently folded into F15.
`DOOM_BOOT_HOLD_STRAFE` defaults off and requires the accepted BOOT sampler plus
F14 thumb zones. With the option off, F15 rebuilds byte-identically at
`769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`.

The candidate does not keep flash suspended during the hold and does not poll
more often. It retains F15's 25 ms `flash_safe_execute()` samples, two-sample
debounce, 2 ms failed-lockout skip, core1 registration, SRAM callback, and
SRAM-only active DMA sources. About 300 ms after the physical press (250 ms
after the debounced state), the modifier activates. A release after activation
does not cycle weapon; a release before activation preserves next weapon.
Touch contacts used while modified cannot resolve as double-tap Use afterward.

The Release candidate builds at `__end__=0x20049bf0`, leaving 220,176 bytes
after the fixed heap before the core1 stack. Its UF2 SHA-256 is
`6a6703b1a0c512f426252ca6981258360c18ac861395bb580b0949187076eaad`.
Active audio buffers, silence, display tiles, BOOT callback, and audio IRQ all
remain in SRAM. Do not call this hardware-safe or accepted until one bounded
USB-only hold/strafe/release test passes; exact F15 remains the rollback.

## 2026-08-18 — Accept release-only BOOT next-weapon as F15

The final effects-enabled hardware gate passed. The audited image booted with
normal sound effects, and one brief BOOT press/release cycled weapons correctly;
Alexander reported the button worked well and everything appeared normal. This
follows successful isolated and silent-Doom gates, so the result covers the
complete intended runtime environment rather than only the electrical sampler.

Decision: accept the installed image as F15. BOOT has exactly one admitted
runtime meaning: after a debounced short press and release in a local single-
player level, queue Doom's native forward weapon cycle. It has no menu or
network action. Do not add hold, double-click, or repeated-tap vocabulary
without a new explicit safety and interaction review.

The safety implementation is part of the feature contract: SDK multicore flash
coordination, SRAM-only raw callback, failed-lockout skip, and SRAM placement
for every active display/audio DMA source. The installed UF2 SHA-256 is
`769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`,
with 220,184 zone bytes after the fixed heap. Keep F14.1
`821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`
as the byte-verified rollback.

## 2026-08-18 — Pass silent Doom and isolate the effects-enabled final gate

The silent Gate B firmware passed on hardware: it booted normally, one brief
BOOT press/release changed weapon once in a level, and Alexander reported no
abnormal behaviour. The exact F14.1 effects build was then restored and
flash-verified before proceeding.

Do not turn audio back on implicitly. Add default-off
`DOOM_BOOT_WITH_SOUND_EFFECTS`, valid only alongside the already gated BOOT
feature. BOOT-enabled builds always place the audio fallback block in SRAM;
the silent build remains the default, while the new option deliberately starts
the established effects-only codec/PIO/DMA backend.

The effects candidate proves every possible active DMA source is SRAM-backed:
queued audio `0x2001deb8`, audio silence `0x20046ca8`, and display tiles
`0x2003d7c8`. The unused local-game piconet buffers are also in SRAM. The BOOT
callback and audio DMA IRQ handler are SRAM-resident at `0x200008c4` and
`0x20001858`. It leaves 220,184 bytes after the fixed heap before core 1's
stack, with UF2 SHA-256
`769879efd084c38f6702732479202f45e0a3c254ff3f026e82ec27f7d9fd5ec6`.
Silent Gate B and normal F14.1 still rebuild byte-identically. Authorize only
one short press/release for the next physical test; hold and double-click BOOT
remain out of scope.

## 2026-08-18 — Build Gate B as a silent, release-only weapon-cycle candidate

Gate A passed and the restored F14.1 firmware remained visibly normal. Proceed
with exactly one default-off integration candidate, not a normal feature build.
`DOOM_BOOT_NEXT_WEAPON` requires hybrid controls, registers core 1 with the Pico
SDK flash-safety coordinator, and polls BOOT only during a local single-player
level. A 2 ms lockout failure is ignored; two confirmed press samples followed
by two confirmed release samples queue one forward weapon cycle.

Use Doom's existing `next_weapon` request consumed by `G_BuildTiccmd()` instead
of synthesizing a weapon number or a new game rule. This preserves the engine's
owned/selectable/shareware/pending-weapon logic. Do not add BOOT hold or double
press behavior before the release-only path passes on hardware.

The first integration test is intentionally silent. GPIO19 remains low from
shared hardware initialization, I2S GPIO20-24 stay high-impedance, and the
sound backend returns before codec, PIO, or DMA initialization. The 2 KiB
silence block is nevertheless moved from XIP to SRAM in the candidate so the
known autonomous flash reader is eliminated before testing.

The binary audit places the SRAM BOOT callback at `0x200008c4`, silence at
`0x20046880`, audio buffers at `0x2001da98`, and display tiles at `0x2003d3a0`.
It leaves 221,252 bytes between the fixed heap end and core 1's stack boundary.
Candidate UF2 SHA-256 is
`891d6076db58253064dba38c4634322ac6109095915737243f38852de7d36076`.
The option-off F14.1 image still rebuilds exactly as
`821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
Do not flash Gate B until this audit is complete; then test one brief
press/release with USB power only and the established stop protocol.

## 2026-08-18 — Gate BOOT next-weapon behind an isolated safety probe

Alexander wants a short BOOT press to cycle to the next weapon, but two prior
BOOT-enabled Doom builds behaved abnormally and one produced loud audio plus a
burning smell. A fresh audit therefore precedes any in-game implementation.

The exact Waveshare schematic shows Key2 pulling the external-flash
`QSPI_SS_N` signal toward ground through 1 kΩ. It is a designed flash-select
input rather than a supply short, and it has no direct electrical route to the
speaker amplifier. Raspberry Pi's official runtime-BOOT example uses the same
SRAM/Hi-Z technique, but explicitly forbids another core or XIP streamer from
reading flash during the sample.

The earlier Doom audit missed an autonomous XIP reader: `silence_buffer` is
`const` and links at `0x1005a1ec`, so the audio DMA reads external flash every
time its SRAM SFX queue is empty. Raw multicore lockout paused CPU core 1 but
did not pause DMA. This is a concrete violation of the official precondition
and a plausible cause of the abnormal audio; it does not prove the odour's
cause. Display DMA reads `panel_chunks` at `0x2003d6f4` in SRAM, and queued
audio reads SRAM buffers at `0x2001ddf8`.

Decision: conditional go, in three gates. First use the new standalone
`boot_safety_probe`, which runs single-core with amplifier/display power low,
I2S pins high-impedance, no DMA/I2C/PIO/display/audio/USB stdio, and the SDK's
`flash_safe_execute()` wrapper. A confirmed press plus release enters ROM
BOOTSEL. It is built but not flashed; SHA-256 is
`05ca114df6c699d4deeb4af733e000ebdafe22dfae286ad25ea5bbcaeb65c04f`.

Only after that physical gate passes may Doom move the silence source to SRAM,
register core 1 with `flash_safe_execute_core_init()`, and add a default-off,
single-player-level-only BOOT sampler. The action will be release-based and
inject Doom's existing next-weapon key path, preserving vanilla weapon-order
and selection rules. The first Doom test must still have audio disabled. No
hold/double BOOT vocabulary is in scope. Full evidence and the stop protocol
are recorded in `docs/BOOT-RUNTIME-SAFETY.md`.

Gate A subsequently passed exactly as designed. With USB power only, the
black-screen/audio-disabled probe detected one brief press plus release and
entered ROM BOOTSEL. `picotool` confirmed the boot type and matching chip ID;
no unexpected audio occurred. The verified F14.1 image was restored and
rebooted afterward. This authorizes Gate B implementation, not a normal
BOOT-enabled release: the first Doom candidate must still relocate the silence
buffer, use the SDK core-safety initialization, remain default-off, and run
with audio disabled.

## 2026-08-18 — Let stationary double taps mean Use inside F14

F14 is now the leading navigation model. Its asymmetric fixed zones produced
Alexander's best play experience, a broad thumb worked reliably, and the guide
outlines improved rather than harmed control. Retain the overlay during longer
testing and defer its permanent visual styling until the mapping is stable.

The initial inherited D-pad rule discarded all taps beginning inside the
control area. This protected against accidental gestures but made doors
unreachable from the natural thumb position. F14.1 instead classifies a bounded
stationary double tap inside any F14 zone as Use/Open. F13 radial retains its
gesture-exclusive square, so its accepted comparison binary does not change.

Do not delay movement while waiting to learn whether the contact is a tap. That
would impose the 340ms double-tap window on every normal hold and undo F14's
immediate feel. Both contacts command their zones immediately, release stops
them, and the second release emits Use. The expected cost is two brief direction
pulses during a door gesture; test that physically before locking F14.1.

The change adds no static state. `__end__` remains `0x20049310`, leaving 224,496
bytes before `__StackLimit`; UF2 SHA-256 is
`821fa198cd0115f8e520166939ed318ded16df5e752bbb4f9d43e3990ac7434d`.
F13 radial remains
`89ce0216629d17c76bc05bd16cbab0fe392c105b3ddc879e2394717d62ce4b5b`
and pointing-finger remains
`ae1d4d7626ff3f3c4549d4df1bbcc3aa3ae37624a971b60a0cb457af760089b5`.

## 2026-08-18 — Tune the accepted fixed model with asymmetric thumb zones

The F13 hardware comparison changed the navigation direction. Absolute
touch-and-hold felt more like traditional keyboard Doom, offered more control,
and worked with Alexander's thumb; relative control had required a pointing
finger. Continue with a fixed model while retaining both F13 radial and the
pointing-finger mapping as exact fallbacks.

F14 follows Alexander's edge-aware sketch instead of enlarging a symmetric
radial pad. Its contiguous area is `[0,340)x[70,368)`: a 24px left-edge turn,
112px forward zone, 204px right-turn zone, and 64px bottom-edge reverse strip.
The smaller left/back targets rely on tactile screen edges; the higher-value
forward/right actions get most of the thumb area. There is no software neutral
zone: lifting the thumb stops movement.

Keep forward-turn diagonals because advancing while aiming is essential, but
limit them to deliberate 12px boundary bands. Omit backward diagonals and the
radial normal/fast step until the cardinal geometry is physically understood.
Use fixed normal Doom movement/turn values 25/640 for this test.

The optional overlay uses boundary-only writes to the existing packed tile,
not a translucent full-frame composition pass. It adds no presentation buffer.
F14 adds four static bytes after alignment and ends at `0x20049310`, leaving
224,496 bytes before `__StackLimit`; its pre-flash UF2 SHA-256 is
`8b2307eee78b403cbe592c5ea51cf2dc37f8fd25692d2c5ed51e65983b90ba61`.
The F13 radial hash remains
`89ce0216629d17c76bc05bd16cbab0fe392c105b3ddc879e2394717d62ce4b5b`
and the pointing-finger hash remains
`ae1d4d7626ff3f3c4549d4df1bbcc3aa3ae37624a971b60a0cb457af760089b5`.

BOOT is not part of F14 and remains forbidden at runtime pending its separate
read-only safety audit.

## 2026-08-18 — Compare a fixed eight-way D-pad before reopening BOOT input

Alexander wants one direct comparison between the accepted pointing-finger
mapping and a classic fixed D-pad. F13 adds default-off `DOOM_TOUCH_DPAD` under
the existing hybrid-control boundary, so the proven build remains available
byte-for-byte. The in-level pad is an invisible 160x160 logical-pixel square in
the bottom-left, centred 80px from both edges, with a 12px neutral centre and
eight sectors. Inner input commands normal movement/turning (25/640); the outer
ring commands run/fast turn (50/960). Diagonals combine movement and turning.
The pad owns its full square so repeated direction taps cannot become gestures.
Double-tap Use and bottom-right strafe remain outside it; overlapping
bottom-left strafe is excluded from this focused comparison.

The candidate adds 48 bytes of text and no data/BSS. It retains
`__end__=0x2004930c`, leaving 224,500 bytes before `__StackLimit`; UF2 SHA-256
is `89ce0216629d17c76bc05bd16cbab0fe392c105b3ddc879e2394717d62ce4b5b`.
Rebuilding the default pointing-finger configuration produces its prior UF2
hash exactly, proving the new selector does not change the locked fallback.

BOOT remains excluded from this candidate. Only after the D-pad comparison
will the project perform a read-only audit of the board schematic, the
flash-CS sampling circuit, RP2350/Pico SDK flash-safety requirements, dual-core
XIP/display/audio interaction, and both abnormal hardware runs. A possible
short press for next weapon plus a distinct short hold for a second action is a
UX goal, not permission to reintroduce runtime sampling before that audit and
an isolated safety-gated test plan.

## 2026-08-18 — Revisit PWR Escape with a short software hold

The earlier PWR-hold candidate is not being restored unchanged. It treated the
PMIC's own long-press classification as Escape and kept fire held meanwhile,
which produced several shots before the menu and preceded an apparent restart.
F12 instead uses the already-proven AXP2101 press/release IRQ edges. The press
still emits one immediate one-tic fire pulse; if it remains physically held for
450ms, software emits one `key_menu_activate` pulse.

Opening Escape changes `menuactive` while the same physical press is still in
progress. F12 therefore suppresses every subsequent PWR event until it sees
the release edge or completed short-press flag. This prevents the release from
being interpreted by the established menu mapping as Select. A new press after
release is handled normally.

The official AXP2101 datasheet defines `REG 27` IRQLEVEL as 1, 1.5, 2, or 2.5
seconds and OFFLEVEL as 4, 6, 8, or 10 seconds. The new 450ms timer is earlier
than both ranges and does not write any PMIC configuration register. It cannot
override the PMIC: holding for several seconds may still power off or restart
the device, so the first physical test must release as soon as the menu opens.

Both hybrid and fallback builds pass. The active F12 image ends at
`0x20049314`, leaving 224,492 bytes before `__StackLimit`; UF2 SHA-256 is
`9a3206a60349be30928a14a5fa75ff45291f254e612bf7d90ca4eeda4fdec62d`.
Installation remains pending after bounded application-mode `picotool` reset
attempts failed before any flash write.

The image was subsequently installed through direct BOOTSEL USB access and its
in-level interaction worked on hardware. Alexander expected the same hold to
work after the menu had opened, then identified that a press-time Fire makes a
clean hold impossible. F12.1 therefore commits nothing on press. Release before
450ms emits Fire in a level or Enter/Select in a menu; reaching 450ms emits
Escape/Back and suppresses release. The hybrid model bypasses the old PWR
double-click recogniser because hold now owns Back. The active image ends at
`0x2004930c`, leaving 224,500 bytes before `__StackLimit`; its UF2 SHA-256 is
`ff0cb48361fdcb364912f67fe502eab45dd5be230a70223d6b971abb843cd107`.

Hardware accepted release-resolved Fire and Escape as very comfortable, but a
very fast click-click sometimes produced one shot. The cause is the virtual-key
pulse boundary: releasing pulse one and pressing pulse two in the same Doom tic
leaves `gamekeydown[key_fire]` continuously true, so `player->attackdown` does
not observe a distinct trigger pull. F12.2 queues up to four same-context PWR
taps and guarantees one sampled low tic between pulses. First-shot latency and
the hold timer are unchanged; a hold flushes pending pulses before Escape. The
active image remains at `0x2004930c` with 224,500 bytes of zone headroom; UF2
SHA-256 is
`702d44306ff7203f5d5fd5d786fca19f24c5ab9634a4e6e1440b63a3acbdb7fb`.

The physical F12.2 test showed that the pulse queue was necessary but not
sufficient: paced click-click emitted two shots, while Alexander's fastest pair
still emitted one. The official AXP2101 register description confirms that
REG49's PWR press, release, short, and long indications are individual
read-write-one-to-clear flags, not counters. The board schematic also confirms
that Key1 pulls AXP2101 PWRON low and is not exposed as a raw RP2350 GPIO;
GPIO18 carries SYS_OUT instead. There is therefore no safe direct-key shortcut
inside the current firmware.

F12.3 fixes the remaining software-ordering case. When a press is already
active and the next REG49 sample contains both release and press, firmware now
completes the first tap and preserves the second press for its later release.
Previously the release branch cleared both flags as one completed tap. A
combined press/release sample with no prior active press remains one fast tap.
If two complete click cycles occur entirely between 35Hz polls, REG49 cannot
represent their multiplicity; servicing the shared I2C bus from a high-rate IRQ
or timer is not justified for this control refinement.

Both active and fallback F12.3 builds pass. The active image remains at
`0x2004930c`, leaving 224,500 bytes before `__StackLimit`; UF2 SHA-256 is
`ae1d4d7626ff3f3c4549d4df1bbcc3aa3ae37624a971b60a0cb457af760089b5`.

F12.3 made no practical difference in the physical pistol test. The remaining
middle-speed loss is explained by Doom's weapon state machine: PWR emits Fire
for one 35Hz tic, but the pistol reaches `A_ReFire` only after 14 attack tics.
A tap pulse that begins and ends during recoil is not buffered and therefore
cannot start another shot. Slow taps align with the next legal firing point;
the fastest complete click cycles may additionally coalesce in REG49. Preserve
vanilla weapon cadence and do not add delayed-shot buffering by default.

## 2026-08-18 — Replace tilt strafe with corner dodge bursts and soften movement

F10 confirmed that roll input and strafe output function, but rejected the
touch-gated interaction. The player naturally expected tilt to work without a
finger. More importantly, changing the device angle before touching could arm
an immediate sidestep against the older neutral reference. Recalibrating at
touch-down would erase the intended held angle, while always-on roll would
restore F1's accidental movement. Continuous tilt strafing is therefore not
the active control direction.

F11 compiles `DOOM_ROLL_STRAFE` off and uses deterministic touch gestures.
Double-tapping the bottom-left or bottom-right 96x72-pixel corner emits a
six-tic, 32-unit strafe burst in that direction; repeated double taps repeat the
dodge. The burst does not latch. Double-tapping anywhere else preserves
Use/Open, so the existing door interaction remains reachable.

Forward/back also felt too fast through the middle of its gesture. F11 keeps
the one-pixel guard, 4-unit initial response, and 50-unit maximum, but replaces
the 88-pixel linear mapping with a 140-pixel quadratic curve. At the old
88-pixel full-scale point it now produces about normal walking speed; maximum run
requires a deliberate reach toward the far side of the display.

The locked 448x280 asynchronous F11 build ends at `0x2004930c`, leaving 224,500
bytes before `__StackLimit`, and contains no QMI8658 input symbols. UF2 SHA-256
is `ec00e1262a4a3f53f93c9cb09cd6fc49b4e39ef4d9fe2ae820f30d1f1eba2cd6`.
The first hardware judgement was that corner strafe is better than having no
strafe, but not compelling enough to justify more tuning in this session. F11
is the wrap-up build; exact burst length and the new movement curve remain
future checks rather than blockers.

## 2026-08-18 — Keep 448x280 locked and measure 448x336 before full-panel fill

The AMOLED is 448x368 in landscape. Current 448x280 output fills the width and
leaves 44-pixel bands above and below. Filling 448x368 would emit 31.4% more
pixels and cannot preserve the whole raw 16:10 image without stretching,
cropping, or redesigning the renderer/UI.

A 448x336 candidate is the better next experiment: it applies traditional 4:3
display correction, leaves only 16-pixel bands, and emits 20.0% more pixels
than the measured baseline. Keep 448x280 locked until a comparable combat
capture proves that 448x336 preserves pacing and audio. The bands may instead
be useful later for quiet port UI such as battery state.

## 2026-08-18 — Gate deliberate proportional roll strafing behind active touch

F9 is an acceptable pointing-finger touch baseline: small motion supports
precision and a deliberate larger displacement turns quickly. The next motion
experiment therefore adds an optional secondary action without retuning or
replacing touch navigation.

The earlier F1 roll candidate was rejected because a six-degree threshold,
three-degree stop zone, fixed 24-unit output, and level-start reference allowed
uncommanded lateral movement while no finger was down. F10 redesigns that
contract. Roll strafing can exist only while the touchscreen control is held;
releasing the finger sets `sidemove` to zero immediately. While released, 18
stable accelerometer samples learn the player's current comfortable grip.
Touch-down freezes the latest completed reference so active play cannot move
neutral underneath the player.

Strafe starts only after two consecutive samples beyond roughly 10 degrees and
stops inside roughly 5 degrees or upon crossing centre. Output scales from 8
to 32 Doom movement units, reaching full response around 22 degrees. This uses
accelerometer gravity position, not gyro rate, because the intended held-angle
action is Mario-Kart-like; gyro integration would introduce drift.
There is no latch, and no-touch movement is structurally impossible.

F10 remains behind default-off `DOOM_ROLL_STRAFE` until physically accepted.
The locked 448x280 asynchronous build ends at `0x20049330`, leaving 224,464
bytes before `__StackLimit`; the 40-byte delta from F9 is the restored QMI8658
path and motion state. UF2 SHA-256 is
`af30c8362cb6cd837bedb560e71998057f7f577a40e6fc28c5e9174c04777816`.

## 2026-08-17 — Add stronger outer-edge turning for the pointing finger

F8 established the pointing finger as the intended relative-control contact.
Its expanded 112-pixel quadratic turn range makes small motion suitably calm,
but large reorientation now takes too much effort. Forward/back was not
reported as the problem and remains unchanged.

F9 keeps the one-pixel guard, 48-unit initial turn, 112-pixel full-scale range,
and quadratic curve, but raises the outer maximum from 640 to 960. Because the
increase is multiplied by the square of displacement, its effect remains small
near the anchor and grows strongly toward the edge. This implements the desired
contract directly: small pointing-finger motion turns slowly; a deliberate
large movement turns quickly.

The 960 bound is below Doom's 1280 fast-turn value and matches an earlier
hardware candidate's maximum, but that candidate had a shorter range, larger
minimum, different dead zone, and pre-repair touch reporting. F9 therefore
requires a new physical judgement rather than inheriting the earlier rejection.

## 2026-08-17 — Trade compact full scale for pointing-finger precision

The F7 driver repair produced the first clearly immediate fine input: a smaller
pointing-finger contact now moves and turns after subtle physical motion. This
validates Active mode and coherent samples directionally. A broad thumb remains
less responsive. The leading explanation is that the current interface exposes
one X/Y point for the broad contact, whose reported position can remain stable
while the contact shape changes; this is an inference, not yet an instrumented
measurement.

The newly visible problem is excessive gain. F7 reaches the 50-unit run bound
after only 44 pixels and the 640 turn bound after 56 pixels—roughly a few
millimetres on this panel. With a precise pointing finger, normal motion can
therefore reach maximum response too easily.

F8 changes only full-scale distance. Movement keeps its one-pixel guard,
4-to-50 linear curve, and doubles full scale from 44 to 88 pixels. Turning keeps
its one-pixel guard, 48-to-640 quadratic curve, and doubles full scale from 56
to 112 pixels. Initial response remains immediate and maximum capability is not
removed, but small and medium motion become substantially more precise.

Thumb ergonomics are not declared solved. If pointing-finger precision becomes
enjoyable but the thumb remains unreliable, the next independent experiment is
the proposed fixed bottom-left eight-way D-pad, where initial touch position
commands direction without requiring centroid movement away from an anchor.

## 2026-08-17 — Repair FT3168 tracking before changing the control model

F6 reduced both software dead zones to one pixel, yet Alexander still needed
roughly a centimetre of physical finger travel before Doom reacted. That cannot
be explained by the control mapping: F6 emits output after the second reported
coordinate step. Pressing harder appeared to help, but the FT3168 is capacitive
and has no pressure channel; changing contact area can instead change the
reported touch position.

The inherited driver put register `0xA5` into Monitor mode (`0x01`) even when
the caller explicitly requested point tracking. The FT3168 datasheet says
Monitor mode detects a valid touch but does not perform full tracking or report
coordinates until it returns to Active mode. Active mode scans at 60Hz by
default. The driver also read finger count, X, and Y through four separate I2C
transactions, allowing state and axes to come from different scan frames.

F7 repairs the driver rather than retuning controls. Point mode now writes
Active (`0x00`) and each poll reads registers `0x02..0x06` in one five-byte
burst, returning finger count and coherent X/Y from the same register snapshot.
Gesture mode retains Monitor mode. F6's one-pixel guards, response curves,
ranges, maximums, diagonals, and release-to-stop remain unchanged.

Both the Doom and standalone hardware-test targets build. Doom text shrinks by
104 bytes, static SRAM remains unchanged at `__end__=0x20049308`, and exact zone
headroom remains 224,504 bytes. Hardware must now establish whether 1–2mm
motion is reported immediately; success is not inferred from the build.

Primary references:

- https://files.waveshare.com/wiki/common/DATA_SHEET_FT3168.pdf
- https://www.waveshare.com/wiki/RP2350-Touch-AMOLED-1.8

## 2026-08-17 — Replace broad dead zones with a one-pixel jitter guard

F5 made forward/back progressive, but the physical test confirmed that the
remaining 4-pixel vertical and 6-pixel horizontal dead zones still felt like
input delay. Once each axis activated, its non-zero minimum output also made
the transition feel more aggressive than the preceding silence.

F6 keeps one pixel on each axis only as a quantisation/jitter guard; deliberate
movement can therefore produce output from the next coordinate step. To avoid
replacing delay with a hard jump, the initial forward output falls from 8 to 4
and initial turn output from 80 to 48. The existing linear movement and
quadratic turn curves preserve almost the same mid-range response, while full
scale remains 50 movement units at 44 pixels and 640 turn units at 56 pixels.

This is deliberately not a zero-dead-zone build. A stable finger can move by a
single reported pixel due to capacitive contact noise; the one-pixel guard is
the smallest practical protection. If it still feels delayed, zero becomes a
valid controlled experiment. If it drifts, use explicit jitter filtering or
hysteresis rather than restoring a broad unresponsive centre.

## 2026-08-17 — Make vertical touch immediate and progressive

The F4 hardware test improved compact left/right responsiveness, but exposed an
unbalanced vertical mapping. Turning began after 6 pixels and scaled with thumb
travel, while forward/back waited for 10 pixels and then jumped directly to a
fixed 25-unit walk speed. Slow deliberate thumb motion therefore appeared to do
nothing for too long, followed by an abrupt single-speed response.

F5 changes only vertical response. Its dead zone falls to 4 pixels, then a
linear mapping starts at 8 movement units and reaches Doom's bounded 50-unit
run speed at 44 pixels. This provides the requested small-motion/slow-movement
and larger-motion/faster-movement progression. Release still guarantees stop.

Diagonal touch is retained because the two axes are calculated independently:
it does not steal or suppress horizontal input, and in Doom it produces the
essential ability to move while turning rather than an implicit strafe. F4's
6-pixel, 80-to-640 quadratic horizontal response remains unchanged so the next
hardware test isolates the vertical transfer function.

## 2026-08-17 — Strengthen short horizontal combat swipes

F3 stayed playable and Alexander completed E1M1, but turning remained the
limiting control in E1M2 combat. Forward/back felt good. Short repeated
left/right swipes often failed to produce enough visible rotation, making it
difficult to track enemies while firing.

The issue is the compact cubic mapping, not its maximum: much of a short swipe
remains close to the minimum output. F4 keeps the 56-pixel range and maximum 640
turn rate, reduces the horizontal dead zone from 8 to 6 pixels, raises minimum
output from 64 to 80, and changes cubic response to quadratic. This increases
small/mid displacement response without exceeding the accepted speed ceiling.
Forward/back and every other control remain unchanged.

## 2026-08-17 — Compress the floating touch control into the bottom-left grip

The touch-only F2 test is the strongest result so far: Alexander could play and
progress more easily than with every prior control model. The remaining major
cost is that horizontal turning requires too much finger travel, obscuring the
full-screen view. His preferred interaction is a finger held in the bottom-left
area with small movements producing the complete useful response.

The current touch model already anchors neutral wherever the finger lands, so
the next experiment does not need fixed quadrants, an overlay, or a second
input architecture. F3 changes one transfer function. Full horizontal response
now fits within 56 pixels instead of 120. The dead zone moves from 10 to 8
pixels, minimum output from 80 to 64, maximum remains 640, and the quadratic
curve becomes cubic. This preserves fine control near neutral while allowing
normal maximum turning inside the compact grip area.

Forward/back, release-to-stop, diagonal combination, double-tap Use, tap-fire,
and the compiled-out IMU are unchanged. Keep the full screen touch-capable for
now; consider restricting or visualising the bottom-left control region only
after the response itself is physically accepted.

## 2026-08-17 — Disable roll strafing and tune touch in isolation

The first touch-plus-roll test clearly preferred the touch navigation direction,
but rejected the combined experiment. Horizontal turning still accelerated too
quickly for precise control on the small display. More importantly, the player
repeatedly strafed left/right with no finger touching the screen. In this build,
no-touch lateral movement can only be produced by the calibrated roll
`sidemove` path, so this is not ambiguous touch noise.

Decision: disable motion completely while touch is tuned. `DOOM_ROLL_STRAFE`
now independently guards IMU initialisation, every accelerometer read, motion
state, and `sidemove` application; it defaults off. The code remains available
for a later controlled experiment, but the active build performs no gameplay
motion-sensor work.

Touch forward/back remains unchanged. Turning maximum falls from 960 to 640,
the minimum from 120 to 80, the dead zone grows from 8 to 10 pixels, and full
scale moves from 96 to 120 pixels. This substantially reduces both near-anchor
and maximum turn response without changing another control variable.

The touch-only full-width build ends at `0x20049308`, leaving 224,504 zone bytes;
the opt-in roll build still ends at `0x20049330`, and the fallback remains at
`0x200492e8`. Touch-only, roll-enabled, and fallback configurations all build.

## 2026-08-17 — Demote tilt to optional roll strafing and restore touch navigation

The third pitch-movement test was learnable but still not dependable. Movement
could begin after a small accidental tilt, the activation point was difficult
to find consistently, and trying to stop could cross the narrow neutral region
and immediately command reverse. This confirms that pitch should not own an
essential navigation action on this device.

Published evidence supports the split rather than arguing for more pitch
tuning. Touch was faster and more accurate than accelerometer input in a mobile
game comparison, while tilt was perceived as engaging. A dual-control shooter
study found tilt movement viable but also observed unintended continuous motion
when players rested outside its five-degree dead zone. Wrist-dexterity research
identifies pronation/supination as a strong tilt axis, and order-of-control work
supports direct held-position mappings over velocity-style control.

Decision: touch now owns simultaneous forward/back and turning, with release as
a guaranteed stop. Horizontal turning keeps its anchor-relative quadratic
mapping but reduces its maximum from 1600 to 960 after Alexander found it too
sensitive. Vertical movement uses a 10-pixel dead zone and fixed normal walking
speed. Double-tap remains Use/Open and PWR remains tap-fire.

Motion is demoted to optional roll strafing. A stable 18-tic grip calibration
defines neutral in the accelerometer Y/Z plane. Strafing begins only after two
consecutive samples beyond roughly six degrees and stops inside roughly three
degrees. It uses the QMI8658 hardware low-pass but removes the extra software
low-pass that contributed to delayed, watery feedback. A cross/dot angle test
makes the thresholds independent of the comfortable grip's projected gravity
magnitude. Returning neutral always stops and no value latches.

The full-width hybrid build ends at `0x20049330`, 72 static bytes above the
locked video baseline and leaving 224,464 zone bytes. The fallback build still
ends at `0x200492e8`. Both build successfully; physical left/right sign, touch
direction, stopping, and combat value remain hardware gates.

Primary references:

- https://www.yorku.ca/mack/ec2017.pdf
- https://www.yorku.ca/mack/mhci2013h.html
- https://hci.cs.umanitoba.ca/publications/details/tilt-techniques-investigating-the-dexterity-of-wrist-based-input
- https://www.yorku.ca/mack/ie2014.html

## 2026-08-17 — Reject tilt latching; use one fixed neutral with hysteresis

The second motion test rejected the stateful movement gearbox. Forward tilt
initially moved backward, commands arrived late, and the player sometimes kept
walking or changed direction after Alexander expected a stop. This is not just
a sign error: automatically rebasing each settled pose made neutral move under
the player, while the one-degree threshold and filter made transitions both
noise-sensitive and delayed.

The third candidate returns to direct position control. It calibrates
Alexander's comfortable roughly 11-o'clock pose once, only after 18 samples
remain stable both sample-to-sample and across the whole window. Signed X/Z
gravity-plane rotation is measured against that fixed reference. The observed
direction is inverted, movement starts at 430 raw-equivalent counts (roughly
1.5 degrees in the ideal plane), and stops at 180 counts (roughly 0.6 degrees).
Forward and backward both use normal Doom walk speed. The only retained state
is start/stop hysteresis; movement never latches and neutral never moves.

The physical BOOTSEL sequence has not changed: unplug USB, hold BOOT, reconnect,
then release after about two seconds. Asking Alexander to continue holding while
the host confirmed enumeration was an unnecessarily cautious instruction, not
a new hardware requirement.

The candidate adds 76 static bytes over the locked full-width baseline, leaving
224,460 zone bytes. Both hybrid and fallback builds pass; hardware validation
of direction, start sensitivity, and reliable return-to-neutral stopping is
pending.

## 2026-08-17 — Replace absolute tilt with a stateful movement gearbox

The first hybrid hardware test produced a useful split result. Horizontal
anchor-relative touch turning felt responsive and enjoyable, and touchscreen
double-tap reliably opened a door. Those interactions stay unchanged.

The proportional single-axis motion mapping is rejected. From Alexander's
comfortable roughly 11-o'clock grip, useful movement appeared only after a
large tilt toward 9 o'clock. The implementation compounded its 950-count dead
zone by emitting only seven Doom movement units at first activation, far below
normal keyboard walking speed.

The replacement treats tilt as a small directional gesture controlling three
latched states: reverse, stopped, and forward. It reads accelerometer XYZ in one
burst and derives signed pitch from X/Z gravity-vector rotation relative to the
last settled pose. A roughly 320-count signal advances one state at fixed normal
Doom walking speed. Afterward, five stable tics rebase the comfortable pose and
re-arm input. This makes an opposite gesture stop first and prevents a single
continuous sweep from crossing directly into reverse.

Held PWR is also rejected. Hardware produced several shots, then the PMIC long
event opened the menu; a longer hold subsequently appeared to restart the game.
In-level PWR now emits one immediate one-tic fire pulse from the press edge and
ignores release/long events. Escape/menu is intentionally left unresolved
rather than competing with the physical power control.

The new build uses no heap allocation and adds 80 static bytes over the locked
full-width baseline, leaving 224,456 zone bytes. Hardware must now validate
pitch sign, small-gesture threshold, state re-arm, dependable stopping, and
tap-fire.

## 2026-08-17 — Build the first hybrid control candidate around combat intent

The first selectable motion candidate is now **horizontal touch turning plus
pitch movement**, superseding the earlier tentative ordering of hybrid axes.
This matches Alexander's preferred interaction and gives each input one job:
the device controls travel while the finger controls view direction. It also
keeps accelerometer tilt away from precise aiming, where prior comparative
research found touch stronger for orientation.

The in-level action mapping is deliberately redesigned rather than preserving
the delayed legacy click patterns:

- pitch tilt produces bounded proportional forward/back movement and neutral
  stops;
- horizontal displacement from the touch-down anchor produces a quadratic,
  bounded turn rate;
- a clean touchscreen double-tap emits Use/Open once;
- AXP2101 PWRON press/release edges hold fire with at most one Doom-tic polling
  delay; and
- the AXP2101 long-press event opens Escape/menu, without reading BOOT.

Menus keep the existing floating swipe navigation and PWR short/double click
semantics. `DOOM_HYBRID_CONTROLS` is compile-time optional so the hardware-proven
floating-touch model remains a recovery build.

The QMI8658 driver is intentionally smaller than the vendor demo: probe both
documented I2C addresses, validate every transaction count, enable only the
accelerometer at +/-2g and 62.5Hz with its LPF, and read one 16-bit axis per
tic. Gyro remains off because it is unnecessary for gravity-relative pitch,
adds power and filtering complexity, and cannot earn its place until the basic
interaction is tested. Direct fixed-point output at `ticcmd_t` avoids noisy
virtual-key transitions and adds only 48 bytes of static SRAM to the locked
full-width build.

No physical feel claim is made yet. The next gate is a short test of sensor
axis/sign, neutral stopping, turn direction/range, double-tap door use, held
fire, and PWR edge polarity. Long-session combat testing follows after control
tuning, as requested.

Primary references:

- https://www.qstcorp.com/upload/pdf/202210/13-52-27%20QMI8658C%20Datasheet%20Rev%20A%20%281%29.pdf
- https://files.waveshare.com/wiki/common/X-power-AXP2101_SWcharge_V1.0.pdf
- https://doi.org/10.1016/j.entcom.2017.04.005
- https://www.yorku.ca/mack/gi2014.html

## 2026-08-15 — Control scheme design discussion
Minimal action set to actually complete Doom (it's 2.5D — turning doubles
as aiming, there's no separate look axis): move forward, turn left/right,
fire, use. Move backward and weapon-switch are "very worth having, not
strictly required." Strafing and run are genuinely optional.

Ruled out the accelerometer/gyro as a primary movement or aiming control —
tilt-based aiming is notoriously imprecise even in games built around it
from scratch, and would fight against itself while also trying to keep
the screen readable. Kept as an optional novelty toggle at most, not core
input.

Noticed our screen's aspect ratio works in our favor: Doom renders at
320×200 (wide, short); our panel is 368×448 (narrow, tall). Kept
pixel-exact (no upscaling — sharper, cheaper to render) and centered, the
leftover vertical space below the game view is roughly as large as the
game view itself — plenty of room for on-screen touch controls without
ever covering gameplay, unlike most handheld Doom ports which fight for
every pixel.

Alexander's proposed scheme (refined from an initial discussion, see his
sketch): invisible hold-to-move touch zones (UP/DOWN/LEFT/RIGHT) on the
left side of the screen; PWR button 1-click = fire, 2-click = use/open
door, long-press = standby (matches its existing confirmed power-toggle
behavior); BOOT button 1-click = next weapon, long-press = in-game menu,
2-click reserved for later. Immersive mode (hiding Doom's status bar HUD)
raised as a nice-to-have, explicitly deferred.

Two real risks flagged before treating this as locked:
1. PWR's click-pattern behavia was never actually tested in firmware —
   we'd only ever observed its hardware-level power-toggle behavior, never
   read it as a discrete input ourselves.
2. BOOT long-press is already claimed in `../mp3player/` for
   `reset_usb_boot()` (reflash without unplugging) — conflicts with using
   it for an in-game menu. Not yet resolved; will need a different
   threshold or a boot-time-only check once we're actually building the
   game rather than just testing buttons.

## 2026-08-15 — Confirmed on hardware: button press-pattern detection works
Built a standalone test (`firmware/main.c`) to settle risk #1 above before
designing anything further around it. Findings:

**BOOT** is read via the existing flash-CS-float technique from
`../mp3player/firmware/lib/button/` — reused directly, no changes needed.
Single/double/long press are all synthesized in firmware from raw
press/release timing (double = two releases within 400ms, long = held
past 1200ms).

**PWR has no direct GPIO at all.** It's wired to the AXP2101 power chip's
PWRON pin, not a plain RP2350 pin — confirmed via the AXP2101 datasheet
(https://files.waveshare.com/wiki/common/X-power-AXP2101_SWcharge_V1.0.pdf).
Reading it means polling the AXP2101 over I2C1 (same shared bus as the
ES8311 codec, address `0x34`) for its own short/long-press IRQ status:

- `REG 0x49` ("IRQ Status 1"): bit 3 = POWERON Short Press IRQ, bit 2 =
  POWERON Long Press IRQ. Both enabled by default (`REG 0x41` bits 3/2
  default to `1`) - no configuration needed, just read and react.
- Status bits are RW1C (write-1-to-clear) - write back exactly what you
  read to acknowledge only the bits that were actually set.
- The AXP2101 itself decides what counts as "short" vs "long" press
  (some internal timing threshold, not something we've dug into
  configuring) - our firmware only synthesizes "double" on top of that,
  same 400ms-window approach as BOOT.

Driver: `firmware/lib/pwr_button/pwr_button.c` (new, ~25 lines).

**Confirmed on hardware**: single, double, and long press all correctly
distinguished on both buttons. Critically, PWR's long-press did **not**
trigger an unexpected hard shutdown before firmware could react (a real
concern going in, since the AXP2101 also has its own hardware-level
auto-poweroff-on-long-press behavior that we hadn't verified was disabled
or would lose the race) - it showed "LONG" on screen and kept running.

This fully de-risks the input layer of the control scheme above. Risk #2
(BOOT long-press conflict) is still open.

## 2026-08-15 — Confirmed: screen orientation and touch zone layout
Display is native portrait (368×448) - no rotation needed, matches how
the button-test text already renders. Touch zones for movement (UP/DOWN/
LEFT/RIGHT) go bottom-left exactly per Alexander's sketch - invisible hit-
regions only, no visible overlay graphics drawn, regardless of whether
that screen region ends up over rendered game pixels or blank letterbox
space once the actual game-view layout is decided. (Earlier note about
"leftover space below the game view" was about where the *game view*
could sit, not a suggestion to move the touch zones there instead of
where sketched - these are independent, both stand.)

## 2026-08-15 — Touch d-pad calibrated on hardware: landscape, asymmetric zones
Supersedes the "no rotation needed" note above - after seeing a real
mockup with the game view in landscape, Alexander wants the game rendered
landscape (`Paint_SetRotate(ROTATE_90)`), not native portrait. Confirmed
safe: GUI_Paint's `Paint_SetPixel` already transforms logical (rotated)
coordinates to physical panel coordinates for every draw call, so no
display-driver changes were needed - just enable rotation and use
`Paint.Width`/`Paint.Height` (now swapped: 448x368) instead of the raw
panel dimensions for centering math.

**Touch coordinates needed their own transform.** The FT3168 touch
controller has no concept of our software rotation - it always reports
raw coordinates in the native portrait frame. `touch_to_logical()` applies
the inverse of `Paint_SetPixel`'s ROTATE_90 transform
(`logical_x=raw_y, logical_y=PANEL_WIDTH-raw_x-1`) so zone hit-testing can
work in the same logical landscape space the visual layout is designed in.
Confirmed correct empirically: asked Alexander which edge of the rotated
view was physically closest to the buttons, and it matched the transform's
prediction.

**Zone layout went through several rounds of on-hardware tuning**, each a
full build-flash-test cycle with serial logging of raw+logical touch
coordinates for calibration data:
1. Started as a simple 3x3 grid (edge cells = zones, corners/center dead).
2. Alexander asked to remove the UP/DOWN dead zone - simplified to
   LEFT/RIGHT as side columns, UP/DOWN splitting the middle column exactly
   in half.
3. First attempt at UP/DOWN vertical assignment was backwards - fixed by
   testing which produced the button-adjacent zone Alexander expected.
4. Several "shift the whole group down by N%" adjustments (`DPAD_Y0`
   84->97->112->129->155->186) - simple, fast iteration once the
   direction was confirmed. The last shift intentionally pushes the zone
   past the visible canvas bottom; the DOWN zone's reachable area just
   naturally shrinks since touch can't register off-panel - matches
   Alexander's sketch (DOWN drawn shorter than UP) with no extra code.
5. Final layout replaced the uniform grid entirely: Alexander provided a
   sketch overlaid on an actual landscape gameplay screenshot showing an
   asymmetric layout - LEFT and DOWN hugging the physical screen edges (so
   a resting hand barely moves to find them by feel), UP and RIGHT sized
   deliberately larger (used more often, costlier to mistap). Implemented
   as four independently-sized rectangles (`zones[]` in `firmware/main.c`)
   rather than forcing it back into a grid.

**Alexander's idea, not yet built**: keep this calibration test reachable
as a debug overlay once the real game exists, rather than throwing it
away - so zone tuning can continue by feel during actual gameplay instead
of only in isolation. Worth revisiting once we understand the game's
render loop structure well enough to know where an overlay would hook in;
premature to design that now.

## 2026-08-15 — Engine cloned and verified; adaptation boundary confirmed
Cloned `kilograham/rp2040-doom` and `raspberrypi/pico-extras` (both
external dependencies, gitignored - see below for how to re-fetch).
Installed SDL2 (+mixer, +net) via Homebrew for the desktop verification
build.

**Desktop `chocolate-doom` target builds clean** (two trivial unused-
variable warnings only) and actually runs - confirmed via process CPU
usage (~13%, consistent with a live render loop) rather than a visible
window, since this Mac's shell sandbox has no GUI/display access for
screenshotting a spawned SDL window. Good enough confirmation the
codebase itself is sound before touching RP2350-specific code, matching
upstream's own stated verification approach.

**RP2350 (`PICO_BOARD=pico2`) build of `doom_tiny_nost`** (the non-USB,
larger-WAD-capable target - no USB keyboard needed since we're using our
own button/touch input) configures cleanly and gets most of the way
through compiling before failing on exactly two files:
`src/i_main.c` (references `PICO_AUDIO_I2S_DATA_PIN` etc., pin macros
meant to come from a custom VGA-board header we don't have, since we're
not using their VGA/I2S hardware) and `src/pico/i_picosound.c` /
`src/pico/i_video.c` (pico-extras' `pico_audio_i2s`/`pico_scanvideo_dpi`
libraries hit an RP2350 hardware errata (RP2350-E2) spinlock safety check
that needs an explicit `PICO_AUDIO_RP2350_OVERLAY_SDK_SPINLOCKS=1` /
`PICO_SCANVIDEO_RP2350_OVERLAY_SDK_SPINLOCKS=1` define to satisfy).

**Both failures are in exactly the two files we already planned to
replace** with our own AMOLED display and ES8311/PIO audio drivers - every
other file (game logic, WAD loading, rendering math, sound mixing,
`tables.c`, `w_wad.c`, `p_*.c`, etc.) compiled cleanly for the RP2350 ARM
target before that point. This confirms the adaptation boundary is
exactly where expected: only `i_video.c`/`i_picosound.c`/relevant bits of
`i_main.c` need real changes; the rest of the engine is untouched. Didn't
bother getting their stock VGA/I2S config building first (would need a
custom board header + the spinlock overlay flags) since that's throwaway
effort for files we're deleting anyway - moving straight to writing our
own replacements instead.

**Repo hygiene**: `engine/rp2040-doom/` (2.2GB+ cloned repo, mostly a
`tinyusb` submodule we don't need for this target) and `engine/wads/`
(WAD files - copyrighted game data, not ours to redistribute even for the
free shareware one) are both gitignored, not committed. To reproduce:
```
git clone --depth 1 --recurse-submodules --shallow-submodules \
  https://github.com/kilograham/rp2040-doom.git engine/rp2040-doom
git clone --depth 1 --recurse-submodules --shallow-submodules \
  https://github.com/raspberrypi/pico-extras.git ~/pico/pico-extras
```
Shareware `doom1.wad` (free, id Software-authorized distribution since
1993) is currently at
`https://raw.githubusercontent.com/Akbar30Bill/DOOM_wads/master/doom1.wad`
(4,196,020 bytes - verify size/hash if re-fetching, third-party mirrors
can disappear or change). Alexander's own retail WAD should replace this
once we're past bring-up.

## 2026-08-16 — Engine vendored, `i_video.c`/`i_picosound.c` written, hardware bring-up (in progress, paused)
Vendored ~220 files of `kilograham/rp2040-doom`'s `DOOM_TINY` engine into
`firmware/engine/`, built a `whd_gen` host tool from source (no CMake -
direct clang invocations) to preprocess `doom1.wad` (shareware, legal to
redistribute) into a flash-mapped `WHD` blob (`IWHX`/super-tiny format,
`TINY_WAD_ADDR=0x10200000`), and wrote real replacements for `i_video.c`
(presents `frame_buffer`/`palette`-composited scanlines to the AMOLED via
QSPI/DMA) and `i_picosound.c` (routes audio through `mp3player`'s
`audio_pio`/ES8311 driver instead of `pico_audio_i2s`). No serial console
available/working in this setup, so a small custom **on-screen boot log**
library (`lib/bootlog/`) was built for hardware debugging: prints short
checkpoint strings directly to the AMOLED as boot proceeds, no scroll (it
just wraps/clears), with `bootlog_skip_until(n)` to jump straight to
checkpoint `n` when RAM is too tight to keep a long visible history.

**Five separate, genuinely unrelated bugs** blocked boot entirely (black
screen, no USB enumeration), found one at a time via checkpoint
bisection across many hardware round-trips:
1. Upstream's 270MHz overclock + a QMI (flash timing) tweak tuned for
   RP2040/their flash chip - silent hang on our RP2350 board. Commented
   out; stock clock is fine for now, revisit for performance later.
2. `PICO_SMPS_MODE_PIN` (GPIO23, from the generic `pico2` board header we
   use for toolchain/SDK purposes only) collides with our board's actual
   I2S LRCLK pin. Commented out.
3. `AMOLED_1IN8_DisplayWindows()` (Waveshare's own per-row-DMA windowed
   update function) is **intermittently unreliable on this hardware** -
   works repeatedly, then silently stops updating the panel with no code
   change, across dozens of test cycles. `AMOLED_1IN8_Display()`
   (full-panel, single DMA transfer) never once failed. Wrote
   `AMOLED_1IN8_DisplayWindowPacked()` replicating `Display()`'s
   single-transfer pattern for a sub-window; both `bootlog.c` and
   `i_video.c` use it instead. This one cost the most time - it looked
   like random flakiness across totally unrelated variables (bootlog
   size, frame buffer size, float/double config) before being correctly
   isolated to this one function.
4. `dma_tx` (the DMA channel the AMOLED driver uses) is only ever
   assigned inside `DEV_Module_Init()` - missing that call left it
   pointing at an unclaimed/unconfigured channel, and
   `dma_channel_is_busy()` on it spins forever. Added the call to both
   `bootlog_init()` and `I_InitGraphics()`.
5. WAD magic mismatch: `whd_gen` as built always emits `"IWHX"` (super
   tiny format), but the firmware checked for `"IWHD"` since
   `WHD_SUPER_TINY` wasn't defined. Added `WHD_SUPER_TINY=1`,
   `DEMO1_ONLY=1` (shareware is single-episode anyway),
   `NO_USE_FINALE_CAST=1`, `NO_USE_FINALE_BUNNY=1` to match.

**Two deeper architectural issues**, found after the above via the same
checkpoint bisection technique, once boot got further:
- `pico_set_float_implementation(doom none)` /
  `pico_set_double_implementation(doom none)` (which upstream sets, since
  Doom's own code is float-free) silently hung inside `mclk_pio_init()` -
  `mp3player`'s `set_mclk_frequency()` does real `double` math, and
  `none` replaces float/double ops with non-functional stubs rather than
  erroring. Removed both defines.
- The engine's "short pointer" memory compression scheme
  (`shortptr_t`/`ptr_to_shortptr()` in `doomtype.h`) requires zone memory
  to live inside a fixed address window
  (`[SHORTPTR_BASE+4, SHORTPTR_BASE+0x40000)`), enforced by an
  unconditional `bkpt #0` (not a graceful assert) on violation. This
  window computation was accidentally gated behind `USE_ZONE_FOR_MALLOC`
  (disabled for an unrelated reason - it collides with pico_malloc's own
  `__wrap_malloc`), so zone memory came from a plain `malloc()` call
  instead, landing outside the valid window and hitting the breakpoint.
  Made the `__end__`/`SHORTPTR_BASE`-based computation in
  `AutoAllocMemory()` (`i_system.c`) unconditional. This freed up the
  zone to claim everything from `__end__` up to `SHORTPTR_BASE+0x40000`,
  which in turn left too little of the C library heap for `panel_window`
  (235KB) to `malloc()` - fixed by making it a `static` array instead
  (accounted for in `.bss` *before* `__end__` is computed, so it doesn't
  compete with the zone for the same sliver of RAM).

**Milestone reached**: with all of the above fixed, boot reliably
proceeded through the *entire* early sequence - display init, audio init
(ES8311/PIO), WAD loading/parsing, zone memory, `R_Init`, `P_Init`,
`S_Init`, `D_CheckNetGame` - all the way to `I_InitGraphics()` completing
successfully. Furthest point reached in the project so far.

**Then, restoring the real `pd_render.cpp`** (the actual DOOM_TINY
renderer - had been swapped for a no-op stub during isolation testing)
**failed at link time**: `.bss` overflowed the 520KB RAM region by
~11.4KB, since `pd_render.cpp`'s real static state (`list_buffer`
~88.5KB, `visplane_bit`, patch/flat decoder scratch buffers, etc. -
~103KB total, vs. the stub's near-zero footprint) is real weight on top
of `frame_buffer` (~105KB), `panel_window` (~230KB), and bootlog's own
buffer. Fixed by shrinking bootlog's buffer further (64px tall → 32px,
i.e. down to showing 1 checkpoint line at a time) - it's explicitly a
temporary diagnostic tool, the cheapest thing to shrink. Link succeeded
with ~32KB of `.bss` headroom to spare.

**First hardware test of the real renderer regressed**: rather than
progressing past checkpoint 17 as before, boot now shows a
distorted/noisy ("помехи") frozen panel, and after restoring full
checkpoint visibility (removing `bootlog_skip_until`, since RAM allows
only 1 visible line right now anyway) the last checkpoint reported was
**`z6: I_ZoneBase returned to Z_Init`** - i.e. it's now hanging *earlier*
than the previously-fixed SHORTPTR_BASE bug's location, despite that fix
still being in place and nothing in this area being touched today. Not
yet root-caused. Leading hypotheses, untested:
- Stack overflow: several of `pd_render.cpp`'s real functions have large
  local buffers (e.g. `flush_visplanes` ~1.8KB, `get_patch_decoder`
  ~0.9KB, nested several calls deep) now linked in for the first time,
  possibly corrupting nearby RAM (including the zone/bootlog buffers)
  even though `z6` executes long before any of that code actually runs -
  worth checking whether something in *static initialization* (C++
  global constructors for `pd_render.cpp`'s file-scope state, which run
  before `main()`) is the actual culprit, not runtime call depth.
  A "noisy" panel (vs. a clean hang) suggests a real DMA transfer of
  garbage data happened, which points at memory corruption rather than a
  pure infinite loop.
- The RAM budget is now genuinely tight (~32KB headroom in `.bss` alone,
  before accounting for stack/heap in the remaining sliver above the
  zone) - worth deliberately re-measuring actual stack usage rather than
  guessing.
- Should NOT re-suspect `AMOLED_1IN8_DisplayWindows()` (bug #3 above) -
  both bootlog and i_video already use the proven
  `AMOLED_1IN8_DisplayWindowPacked()` path.

**Status**: paused here to document and decide whether to continue.
Extensive temporary diagnostics (bootlog checkpoints scattered through
`i_main.c`, `d_main.c`, `i_system.c`, `z_zone.c`, `i_picosound.c`) are
still in the tree, deliberately not cleaned up yet.

**Root-caused and fixed the `z6` regression, same day**: it was the RAM
budget again, not new corruption. `panel_window` (the AMOLED presentation
buffer) was sized for the *entire* 368px-wide physical strip
(368x320x2 = 235KB) when Doom's rotated image only ever occupies a
`SCREENHEIGHT`(200)-wide sub-band of that 368 (the rest is letterbox
padding that was being allocated and DMA'd but never actually drawn
into). Shrunk `panel_window` to exactly the used band (320x200x2 =
128000 bytes, zero slack), moving the letterbox offset into the
`AMOLED_1IN8_DisplayWindowPacked()` call's window instead of the buffer
layout. This pulled the linker's `__end__` symbol back from
`0x2007c8a4` (51KB *past* the hard `SHORTPTR_BASE+0x40000` zone
boundary - the "short pointer" scheme's absolute limit, not a soft
budget) to `0x200624a4` (~55KB of margin *below* it). Also added a
proper guard in `i_system.c`'s `AutoAllocMemory()` for this exact
direction of the bug: the existing check only caught zone memory
starting *too low*; there was no check for it starting *too high*
(past the window), which is what let `*size` silently underflow to a
huge bogus value and hand Z_Init a garbage-sized zone instead of
erroring - that's almost certainly what produced the distorted/noisy
screen (a real DMA transfer of corrupted memory), not a clean hang.

**First real rendered frame, confirmed alive**: with that fixed, boot
proceeded all the way through the entire game loop, and the AMOLED
displayed the actual `TITLEPIC` splash graphic decoded from the WHD/WAD
- correct colors, correct proportions, right-side up - the first time
any WAD-derived pixel data has rendered on this hardware. A per-frame
heartbeat checkpoint (`d_main.c`'s game loop) confirmed ~350 frames in
~10 seconds, i.e. running at Doom's native ~35Hz tic rate - not hung,
just correctly idling at a static (non-animated) title screen with no
button/touch input wired up yet to advance past it. This is the
CLAUDE.md-stated first milestone ("get a WAD loading and rendering at
all") - reached.

**Known remaining cosmetic issue**: the letterbox padding area around
the centered Doom image (both the pillarbox strips beside it and
whatever's outside bootlog's own window) is never explicitly cleared,
so it shows leftover/noise content from whatever was on the panel
before. `AMOLED_1IN8_Clear()` (Waveshare's own full-panel clear
function) turns out to use the same per-row `dma_channel_configure`-in-
a-loop pattern already proven intermittently unreliable on this
hardware (see bug #3 above) - so fixing this properly needs a
single-transfer-based clear (like `AMOLED_1IN8_DisplayWindowPacked()`),
not a call to the existing `Clear()`. Deferred; purely cosmetic.

## 2026-08-16 (cont'd) — First pass at wiring touch/PWR input, not yet working
Added `PollHardwareControls()` to `engine/pico/i_input.c`, called once per
tic from `I_GetEvent()` (confirmed this is really on the tic path via
`d_loop.c`'s `BuildNewTic()` -> `I_StartTic()`). Touch d-pad zones (UP/
DOWN/LEFT/RIGHT, coordinates copied verbatim from the validated
calibration firmware) post `ev_keydown`/`ev_keyup` for `key_up/down/left/
right` on zone-entry/exit; PWR single/double-press (reusing
`poll_pwr_button()`'s logic, also copied verbatim) posts a one-tic pulse
(`keydown` now, `keyup` next poll call - see `PulseKey()`'s comment for
why a same-tic down+up wouldn't register) for `key_fire`/`key_use`.
Linked `touch`/`pwr_button` libraries into the `doom` target. Deliberately
did NOT wire BOOT: it reads via floating the flash QSPI CS pin, which
would race with core1's concurrent XIP flash reads (WAD data + code) now
that the renderer runs on core1 - needs a real cross-core guard first,
not a straight port of the calibration firmware's version.

**Two separate open problems found, not yet resolved**:
1. Touching a zone or pressing PWR produces no visible effect at all - a
   diagnostic bootlog checkpoint added to fire the moment
   `PollHardwareControls()` detects either (`"IN: touch key=..."`/
   `"IN: PWR ..."`) never appeared during on-hardware testing, meaning the
   input detection itself isn't firing, not just that Doom ignores what
   it receives. Not yet root-caused - candidates: `FT3168_Init()`'s lazy
   first-call timing (it runs on the first game-loop tic, well after
   `I_InitGraphics()` already launched core1 - unlike the calibration
   firmware where it ran immediately after `DEV_Module_Init()` with
   nothing else going on), an I2C bus assumption that doesn't actually
   hold once core1 is active, or something more basic like the zone
   coordinates or `touch_to_logical()` needing re-validation in this
   build's actual runtime rotation/orientation.
2. Separately (found while investigating #1, still true regardless of
   whether input gets fixed): the title screen's automatic countdown-and-
   advance mechanism (vanilla's `D_PageTicker`/`D_PageDrawer`) is compiled
   out entirely for this build - `D_Display()`'s `case GS_DEMOSCREEN:
   D_PageDrawer();` sits inside `#if !PD_COLUMNS`, and `PD_COLUMNS=1` is
   set (required for the DOOM_TINY renderer). Something else must be
   responsible for both drawing the title screen we did see and for
   advancing off of it in a `PD_COLUMNS` build - not yet traced into
   `pd_render.cpp` to find what. Worth understanding before assuming a
   fixed input pipeline will actually make the title screen advance.

**Status**: paused again to pick up later. Diagnostic checkpoint left in
place in `i_input.c` (harmless when idle - only prints on a detected
press).

## 2026-08-16 (cont'd) — Menu freeze root-caused to the audio subsystem
Long debugging arc (many hardware round-trips) chasing a 100%-
deterministic freeze: menu navigable exactly once (either opening it, or
one move within it - each of which triggers a sound effect,
`sfx_swtchn`/`sfx_pstop`), then frozen solid on the *second* interaction
of any kind, always. Ruled out, in order, with real evidence each time
(not just "tried it and moved on"):
- The WHD demo-decoder hang (a real, separate bug - see the entry above -
  fixed by skipping demo playback slots in `D_DoAdvanceDemo`).
- `AMOLED_1IN8_Clear()`'s per-row DMA loop pattern (never actually called
  in this path - ruled out by inspection, not testing).
- A cross-core race on the shared `dma_tx` DMA channel between bootlog's
  diagnostic prints (core0) and the game's own frame presentation
  (core1) - real and fixed (added `dma_tx_mutex` in `AMOLED_1in8.c`), but
  didn't fix this freeze.
- A cross-core race in the audio mixing path (`I_Pico_UpdateSound()`,
  called from both cores via `pd_render.cpp`'s `SafeUpdateSound()`) -
  real and fixed (added `update_sound_mutex` in `i_picosound.c`), but
  didn't fix this freeze either.
- `pd_render.cpp`'s own `wipestate` transition state machine getting
  stuck non-`WIPESTATE_NONE` - added a diagnostic+safety-break guard,
  never fired.
- Core1's stack (`PICO_CORE1_STACK_SIZE`, was 0x4f8 = 1272 bytes,
  upstream's RP2040 value) being too tight for real rendering - bumped
  to 0x1000 (the most SCRATCH_X, the fixed 4KB hardware SRAM bank it
  lives in, can hold), no change.
- `bootlog_print()` itself having no protection around its own shared
  state (`fb[]`/`next_line`/`print_count`), only around the DMA transfer
  - real gap, fixed (added `bootlog_mutex`), no change.
- Our own diagnostic instrumentation being heavy enough to cause the
  problem itself (very plausible given how much was added) - tested by
  stripping per-tic checkpoints back to near-nothing; freeze persisted
  identically, ruling this out too.

Every one of the above was a real, legitimate bug or gap worth fixing
regardless (multiple genuine unguarded cross-core races existed and are
now fixed), but none of them were *this* freeze's cause. The actual
confirmation: building with `S_StartSound()` stubbed out entirely
(`DEBUG_NO_SOUND=1` in CMakeLists.txt, `s_sound.c`) - the exact same menu
navigation that reliably froze on the second interaction works
perfectly with sound off. This conclusively narrows the bug to
`i_picosound.c`'s `I_Pico_StartSound()`/`I_Pico_UpdateSound()` (or
something in `audio_pio.c`/`es8311.c` beneath them) specifically
triggered by a **second** sound-effect start - not corruption, not a
race (all of those are now independently fixed and ruled out), not
diagnostic overhead. A leading unconfirmed hypothesis: something in
channel reuse/reset between `S_StopChannel()` (game-level) and this
engine's own `channels[]` array (hardware-level, `stop_channel()` /
`is_channel_playing()`) leaves stale state that only bites on a second
trigger - not yet verified with the channel-state checkpoints
(`as1`/`as1b`/`as2`, printing channel index + decompressed_size/offset/
step) added right before this was found, since `DEBUG_NO_SOUND=1` was
tried next instead and immediately confirmed the audio path as the
cause.

**Status**: `DEBUG_NO_SOUND=1` currently enabled - unblocks menu/game
navigation entirely, at the cost of no sound effects. Good enough to
keep making progress on gameplay while the actual audio bug gets a
dedicated look later. To resume: remove `DEBUG_NO_SOUND=1` from
CMakeLists.txt, reproduce the freeze (open menu, then one more
interaction), and read the `as1`/`as1b`/`as2` channel-state checkpoints
already in place in `i_picosound.c`'s `I_Pico_StartSound()` - they were
added but never actually read before the sound-off test intervened.

## 2026-08-16 (cont'd) — Likely real cause of the audio freeze found (via upstream comparison)
Fetched upstream's actual `src/pico/i_picosound.c` (the file ours was
adapted from) from the `kilograham/rp2040-doom` repo to compare
architecture, per a suggestion to check whether other ports/forums hit
this class of bug before spending more time on checkpoint bisection.

Upstream's `I_Pico_UpdateSound()` calls
`take_audio_buffer(producer_pool, false)` - the `false` is
non-blocking: if the buffer pool (managed by `pico_audio_i2s`) has no
free buffer, it returns null and the function just skips that tic's
audio entirely, doing nothing. The actual I2S output happens later,
asynchronously, via `pico_audio_i2s`'s own IRQ-driven DMA - completely
decoupled from this function's caller. This is DESIGNED to be callable
from anywhere, any number of times, without ever stalling the caller.

Our replacement (`i_picosound.c`, using mp3player's `audio_pio` driver)
does the opposite: `audio_out()` calls `pio_sm_put_blocking()` in a
loop, a hard synchronous block that can take ~12ms per call
(MIX_BUFFER_SAMPLES=512 at 44.1kHz) once real audio is playing (silence
before that plays through fast, which is why nothing looked wrong until
the first real sound). `audio_pio.c` (from `mp3player`, itself adapted
from Waveshare's demo) was written for a single-core, sequential "push
samples, block until accepted" music player - not for being called from
inside a real-time game loop that also has to keep rendering and
processing tics on a tight per-frame schedule from either core. A core
stalling for ~12ms mid-frame is a very plausible way to break
`pd_render.cpp`'s core0/core1 rendezvous handshake (the
`render_frame_ready`/`display_frame_freed` semaphore handoff), which is
presumably designed assuming calls with bounded, short latency
throughout - explaining a freeze that's deterministic (same call
pattern every time) without needing an actual data race (all of the
mutex fixes made along the way were real, legitimate bugs, but not
*this* one).

**Not yet fixed**: properly fixing this means making our audio output
path non-blocking too (real DMA/IRQ-driven double-buffering into the
PIO FIFO, not a busy-wait `pio_sm_put_blocking` loop) - a real driver
rewrite, not a quick patch. `DEBUG_NO_SOUND=1` remains the practical
stopgap. If picked back up: `audio_pio.c`'s `audio_out()`/`data_treating()`
are the functions to redesign; `mclk_pio_init()`/`dout_pio_init()` (PIO
program setup) likely don't need to change, only how samples get handed
to the PIO SM's FIFO.

## 2026-08-16 (cont'd) — Level-load freeze root-caused to zone list corruption, not exhaustion
With `DEBUG_NO_SOUND=1` (sound/music both stubbed - see above), the menu
became fully navigable, but starting an actual game (New Game -> episode
-> skill) froze reliably. Checkpoint-bisected through
`G_DoNewGame -> G_InitNew -> G_DoLoadLevel -> P_SetupLevel ->
P_LoadBlockMap`, each round of testing moving the freeze point further
(a genuinely new, first-time-exercised code path - real map geometry
loading, never reached before this session). `S_Start`/`Z_FreeTags`/
`P_InitThinkers` all confirmed fine; narrowed to `Z_Malloc(1656, PU_LEVEL,
0)` for `blocklinks` (tiny allocation) appearing to hang.

Added a `Z_FreeMemory()` call to a checkpoint right before that
allocation to check available zone space - it reported `free=0`, which
initially looked like confirmation of genuine zone exhaustion (upstream's
own docs, kilograham.github.io/rp2040-doom/speed_and_ram.html, state a
real level can use up to ~45K of their ~58K total heap - very plausible
given our own zone margin measured around the same ballpark).

**That diagnosis was wrong**, caught only because of a bootlog UI
improvement made in the same session (switching from a single
overwritten line to a 3-line scrolling history, at Alexander's
suggestion): a message that would have been instantly overwritten and
invisible in single-line mode turned out to be sitting right there -
`zfm: stuck blk=00000000 tag=0`. This is `Z_FreeMemory()`'s own bounded
iteration guard (added earlier alongside `Z_FreeTags`'s identical guard -
see above) firing: it walked `mainzone`'s block linked list 20000+ times
without reaching the sentinel, meaning the list is **corrupted** - some
block's `sp_next` decodes to a null shortptr, and the walk doesn't
recognize that as anywhere near the end. The `free=0` result was an
artifact of the walk being cut short right after the corruption point,
not a real free-byte count.

**Not yet found**: what actually corrupts the list, or when. Investigated
and ruled out `W_CacheLumpNum()` (called for the blockmap lump right
before this) as the culprit - `DOOM_TINY=1` makes it a trivial inline
function (`w_wad.h:107`) that returns a direct pointer into memory-mapped
flash and never touches the zone allocator at all. Since `Z_FreeTags`
(which walks the identical list, with the identical guard) reported no
issue earlier in the same call chain, either something non-obvious
between the two calls corrupts it, or - more likely given how little
happens in between - `Z_FreeTags`'s own guard *also* fired but scrolled
off the 3-line history before it could be read (many checkpoints exist
between the two calls). A `Z_FreeMemory()` checkpoint was also added at
`D_StartGameLoop` (before ANY menu interaction) specifically to test
whether the corruption predates all user interaction entirely - not yet
observed, since attention was on the later freeze point when this build
was tested. Next step: watch for a "stuck" message immediately at boot/
title-screen time, before touching the menu at all.

## 2026-08-16 (cont'd) — Zone corruption root-caused and fixed: a stray `calloc()` was colliding with the manually-claimed zone

Continuing straight from the previous entry. Bisected the corruption
point using a sequence of one-shot bootlog checkpoints (`Z_FreeMemory()`
calls bracketing progressively smaller windows of execution), each round
narrowing the search:

1. A checkpoint right at `D_StartGameLoop` (before any menu interaction)
   came back clean (`free=8172`/`22504` depending on build) - the
   corruption does *not* predate the game loop.
2. A checkpoint right after the very first `D_RunFrame()` tic (before any
   button press) was *already* corrupted - so it happens within a single
   frame of idle title-screen ticking, not from menu navigation.
3. Instrumented `Z_Malloc()` itself to print every call: `zm#1` (the
   *only* allocation that ever happens, tag=`PU_STATIC`, 136 bytes,
   during `P_Init`) is clean and is the last real allocation before the
   corruption - ruling out the allocator's own bookkeeping entirely.
4. Bracketed core1's per-frame work (`pd_core1_loop()`/`new_frame_stuff()`)
   - already corrupted by the time core1 does its first real work, and
   core1 only starts rendering *after* core0 signals a frame is ready, so
   the break necessarily happens on **core0**, before core1 does anything.
5. Bracketed each step inside `D_RunFrame()` itself
   (`I_StartFrame`/`TryRunTics`/`S_UpdateSounds`/`D_Display`): clean after
   `TryRunTics`, broken immediately after `S_UpdateSounds()`.

**Root cause**: `S_UpdateSounds()` calls `I_UpdateSound()` unconditionally
every frame (regardless of `DEBUG_NO_SOUND` or whether any channel is
actually playing), which dispatches to `I_Pico_UpdateSound()`
(`engine/pico/i_picosound.c`), which *always* calls
`data_treating()` (`lib/audio_pio/audio_pio.c`, ported from mp3player) to
convert the mix buffer - and `data_treating()` called `calloc()`. That's
the **only** malloc-family call anywhere in the doom firmware. Meanwhile
`i_system.c`'s `AutoAllocMemory()` sets the DOOM_TINY zone's base
address directly to the linker's `&__end__` symbol (not via `malloc()`),
with a comment noting "we have set heap size to 0, so `__end__` is a good
value" - an assumption that held right up until this `calloc()` was
introduced. newlib's `_sbrk()`-backed heap *also* starts handing out
memory from `__end__`, so the very first `calloc()` call handed back
memory starting at the exact same address as the zone's own first block
header, silently overwriting DOOM's own bookkeeping there. One call was
enough to permanently corrupt the list (matches every symptom observed:
the "stuck" signature was stable/deterministic from the first check
onward, never got progressively worse, and needed zero further
allocations to reproduce).

This is very likely *also* the true cause of the original menu-freeze
bug from earlier the same day (the one `DEBUG_NO_SOUND=1` was a stopgap
for) - `I_UpdateSound()`'s call chain into `data_treating()` runs
unconditionally on every frame regardless of that flag, so the collision
would have happened on the very first frame either way.

**Fix**: changed `data_treating()` to write into a `static int32_t
samples[512]` buffer instead of `calloc()`ing (the only caller always
passes a fixed length, `MIX_BUFFER_SAMPLES`=512) and removed the now-
invalid `free(frames)` call in `i_picosound.c`. Confirmed on hardware:
zone stays clean (`free=` value constant, no `stuck`) through title
screen, full menu navigation, and `P_SetupLevel()` completing entirely
(`BlockMap`/`Vtx`/`Sect`/`Side`/`Line`/`Sub`/`Node`/`Seg`/`GroupLines`/
`Reject`/`LoadThings`/`SpawnSpecials` all OK) - the level loads
successfully for the first time this session.

Diagnostic instrumentation added along the way and **not yet cleaned
up**: `bootlog`'s history grew from 3→7→18 lines and back down to 7 (18
was only affordable while `i_video.c`'s `present_frame_to_amoled()`/
`panel_window` were temporarily disabled - see that commit - to free
128000 bytes; both are restored now that the corruption is fixed).
`Z_FreeMemory()`'s "stuck" guard now only prints once per boot (was
spamming the same line every call once broken). Many one-shot/capped
checkpoints (`zm#`, `mr#`, `rf#`, `c1a`/`c1b`, `21`/`21b`) remain scattered
across `z_zone.c`, `m_menu.c`, `d_main.c`, `i_video.c` - safe to leave (all
bounded, won't spam indefinitely) but should be stripped once the port
stabilizes.

**New, separate freeze found immediately after - RESOLVED, genuine zone
exhaustion, not a race**: with real rendering re-enabled, starting a game
froze partway through `P_LoadThings()`, stuck right after thing index 48
(doomednum 2035, the first exploding barrel) out of 138. This did *not*
happen when rendering was disabled, which initially looked like a
core0/core1 timing issue. A bracket checkpoint (`sm1`/`sm2` around that
specific `P_SpawnMapThing(48)` call) confirmed the hang was **inside**
`P_SpawnMobj()` itself, not after it returned - and pico-sdk's own
`panic()` (which `DOOM_TINY`'s `Z_Malloc` calls on genuine
out-of-memory) calls `vprintf()`/`puts()` over stdio (USB CDC in this
build), which - per the earlier printf-freeze lesson this same session -
blocks forever with no host reading. That's a silent, total freeze
indistinguishable from a deadlock, with zero further checkpoints able to
print. Patched `Z_Malloc`'s OOM path (`z_zone.c`) to `bootlog_print()` a
message *before* calling `panic()`, and added `Z_FreeMemory()` to the
`P_LoadThings()` per-thing checkpoint. Confirmed on hardware: `fr=0`
already by thing #44 (tiny decorations kept barely fitting), then
`OOM: Z_Malloc size=680` at thing #48 (the barrel, needing a full
`mobjfull_t`) - genuine exhaustion, not corruption or a race.

**Why now specifically**: re-enabling `panel_window` (128000 bytes)
shrinks the zone's own capacity by that much (the zone's ceiling is
fixed at `SHORTPTR_BASE+0x40000`, so anything used for static RAM before
`__end__` comes directly out of the zone's side) - combined with
bootlog's 7-line buffer (72128 bytes), the working "corruption-fixed"
config was actually using *more* total static RAM (200128 bytes) than
the disabled-rendering config that finished loading the whole level
cleanly (185472 bytes, all in an 18-line bootlog with no panel_window at
all). Past corruption-hunting now, so bootlog's history was shrunk from
7 lines down to 3 (72128 -> 30912 bytes, reclaiming ~41KB back to the
zone - comfortably more than the measured shortfall). Confirmed on
hardware: **the game runs** - level loads fully and is playable.

**Third freeze, found during actual play**: after playing briefly
(turning, walking, shooting - real gameplay, HUD visible with 100%
health, weapon/ammo shown, an enemy on screen), the game froze again.
Confirmed via bootlog this is a *different* bug from the two above: no
`OOM: Z_Malloc` line (so not zone exhaustion again - that checkpoint is
a generic hook in `Z_Malloc` itself and would have fired regardless of
call site). The last visible lines before the freeze were a rapid burst
of touch events alternating between two key codes: `IN: touch key=0xad`,
`IN: touch key=0xae`, `IN: touch key=0xad`. Not yet investigated further
(out of time this session) - but the pattern (same two codes repeating
fast, right before the freeze) points at the touch-input handling path
itself (`engine/pico/i_input.c` and/or the touch driver in
`lib/touch/`) rather than game logic - e.g. an event-queue overrun, or a
repeat/debounce issue generating events faster than the game loop drains
them. Next session: find where `0xad`/`0xae` touch events are generated
and queued, and check for an unbounded-growth or overwrite-without-check
pattern under rapid repeated input.

## 2026-08-16 (cont'd) — First fix candidate for gameplay touch-burst freeze

Inspected the touch path after the gameplay freeze whose last visible
messages alternated between `0xad` and `0xae`. The transition handler was
calling `bootlog_print()` for every newly observed zone. That is not a cheap
log operation: it redraws and synchronously transfers the bootlog framebuffer
to the AMOLED. Touch-coordinate jitter at a zone boundary could therefore
force a blocking panel transfer every tic while also posting a keyup and a
keydown for every transition. The tiny Doom event queue has only eight slots
and no full-queue check.

Removed the per-transition bootlog redraw and added a two-consecutive-tic
stability filter for switching into a non-zero touch zone. Finger release is
still accepted immediately so a movement key cannot remain stuck. The
firmware builds successfully. This is a **candidate**, not yet hardware-
confirmed: flashing and sustained play on the physical board are required to
tell whether it fixes the freeze or merely exposes the next failure.

## 2026-08-16 (cont'd) — Combat freeze now points back to zone pressure

The touch stability/filter build played substantially longer, so the earlier
touch-event burst was a real problem. A later freeze happened when a barrel
exploded, initially suggesting `P_RadiusAttack`; a second playthrough froze
while shooting a second enemy in a different room, without a barrel. The
shared trigger is now combat rather than the barrel: shots spawn temporary
puff/blood thinkers, and deaths retain corpses and can spawn dropped items.
Those objects use thinker pools whose new backing blocks come from the zone.

The level only began fitting after shrinking the temporary bootlog from seven
lines to three, so runtime headroom was already suspect. Shrunk it again from
three lines to one, reclaiming 20,608 bytes of static RAM directly back into
the zone address window. Reverted the untested barrel-specific breadcrumbs so
this build changes only memory headroom (in addition to the preceding touch
fix). Builds successfully; sustained combat on hardware is the next test.

The existing `Z_Malloc` OOM bootlog message not appearing on the frozen image
does not conclusively rule OOM out: an already-running full gameplay frame DMA
can overwrite the small diagnostic strip after the OOM print and before the
panic halt becomes visible.

## 2026-08-16 (cont'd) — Scaled tiled video works; extend shortptr zone by 64KB

Hardware test of the 448x280 aspect-preserving (7:5) scaled renderer showed
the actual Doom image clean, centered, and much larger. The colorful noise in
the photo was confined exactly to the two 44px letterbox bands: panel GRAM
that the partial-window presenter never initialized, not corrupt gameplay
pixels. Added a one-time clear of those bands using four packed transfers and
the existing tile buffer, with no new allocation.

The game also survived substantially more combat after replacing the original
128KB full rotated framebuffer with a 35,840-byte 40-row transpose/scale tile,
but eventually froze again, continuing to indicate marginal runtime zone
capacity rather than a barrel-specific failure.

Found a larger architectural RAM reserve: RP2350's `SHORTPTR_BASE` was
`0x20030000`, limiting the 16-bit/word-addressed 256KB window to
`0x20030000..0x20070000`, even though general SRAM is available to the linker
stack limit at `0x20080000`. The only static addresses directly encoded as
short pointers are `players` (`0x20044dd8`) and `thinkercap` (`0x20047308`),
and zone allocation starts at `__end__` (`0x20048598`), all safely above
`0x20040000`. Moved the window to `0x20040000..0x20080000`. This expands the
actual zone from 162,408 to 227,944 bytes: exactly 65,536 additional bytes,
without changing allocations, object lifetimes, or the display buffer.
The upper bound equals (but does not cross) `__StackLimit=0x20080000`.

Build succeeds. Hardware boot/play is required because an invalid direct
short-pointer target would hit the deliberate `bkpt` range guard immediately;
the linked-address audit covers the known direct static targets, while a real
boot exercises initialization end-to-end.

## 2026-08-16 (cont'd) — Touch control model: fixed zones -> floating swipe-and-hold

Documenting the now-superseded-but-preserved first gameplay control model:
four asymmetric, invisible rectangles in logical landscape coordinates map
directly to held arrow keys. LEFT=`[0,65)x[270,345)`, UP=`[70,100)x[90,300)`,
DOWN=`[70,100)x[300,368)`, RIGHT=`[105,290)x[260,340)`. Entering a rectangle
posts keydown, leaving/switching posts keyup, and a two-tic stability filter
suppresses touch-controller boundary chatter. This worked electrically but
was difficult to find and operate by feel on the 1.8-inch panel.

Researched common small-screen/mobile game patterns before replacing it.
Apple's game-control guidance recommends a movement thumbstick that appears
wherever the player lands their thumb rather than a fixed thumbstick, using as
large an input area as possible. One-touch-game research likewise reports
avatar movement gestures performed anywhere on screen, reducing reach and
occlusion problems. Four-way swipe implementations also warn about rapid axis
flipping near diagonals and use an axial bias/hysteresis strategy.

Implemented Alexander's proposed swipe-and-hold model as a **floating digital
joystick** (`TOUCH_CONTROL_SWIPE_HOLD=1` in `i_input.c`):

- Touch anywhere establishes an invisible anchor; no movement happens yet.
- Drag 24 logical pixels past the anchor to choose the dominant cardinal axis.
- Holding maintains that Doom arrow key.
- Sliding around the same anchor can change direction without lifting.
- Returning inside the dead zone or lifting posts keyup immediately.
- An 8px dominant-axis bias plus the existing two-tic transition filter avoids
  diagonal/controller jitter.

The complete original fixed-zone implementation remains compiled in behind
the selector for immediate fallback and further zone tuning if this experiment
does not feel good on hardware.

References:
- https://developer.apple.com/design/human-interface-guidelines/game-controls
- https://arxiv.org/abs/2106.14505
- https://maxkrieger.itch.io/crossniq/devlog/9448/systems-breakdown-swipe-movement-controls

## 2026-08-16 (cont'd) — Touch control model: fixed zones -> floating swipe-and-hold

Documenting the now-superseded-but-preserved first gameplay control model:
four asymmetric, invisible rectangles in logical landscape coordinates map
directly to held arrow keys. LEFT=`[0,65)x[270,345)`, UP=`[70,100)x[90,300)`,
DOWN=`[70,100)x[300,368)`, RIGHT=`[105,290)x[260,340)`. Entering a rectangle
posts keydown, leaving/switching posts keyup, and a two-tic stability filter
suppresses touch-controller boundary chatter. This worked electrically but
was difficult to find and operate by feel on the 1.8-inch panel.

Researched common small-screen/mobile game patterns before replacing it.
Apple's game-control guidance recommends a movement thumbstick that appears
wherever the player lands their thumb rather than a fixed thumbstick, using as
large an input area as possible. One-touch-game research likewise reports
avatar movement gestures performed anywhere on screen, reducing reach and
occlusion problems. Four-way swipe implementations also warn about rapid axis
flipping near diagonals and use an axial bias/hysteresis strategy.

Implemented Alexander's proposed swipe-and-hold model as a **floating digital
joystick** (`TOUCH_CONTROL_SWIPE_HOLD=1` in `i_input.c`):

- Touch anywhere establishes an invisible anchor; no movement happens yet.
- Drag 24 logical pixels past the anchor to choose the dominant cardinal axis.
- Holding maintains that Doom arrow key.
- Sliding around the same anchor can change direction without lifting.
- Returning inside the dead zone or lifting posts keyup immediately.
- An 8px dominant-axis bias plus the existing two-tic transition filter avoids
  diagonal/controller jitter.

The complete original fixed-zone implementation remains compiled in behind
the selector for immediate fallback and further zone tuning if this experiment
does not feel good on hardware.

References:
- https://developer.apple.com/design/human-interface-guidelines/game-controls
- https://arxiv.org/abs/2106.14505
- https://maxkrieger.itch.io/crossniq/devlog/9448/systems-breakdown-swipe-movement-controls

## 2026-08-16 (cont'd) — Swipe controls improve play; optimize scaler and persist OOM

First hardware test of the floating swipe-and-hold control model was clearly
better than fixed zones: still needs tuning, but the game became materially
more playable. Two remaining observations: full-width rendering felt laggy,
and rapid shooting during combat still eventually froze after one enemy kill.

The 448x280 scaler's hottest loop computed `output_x * 5 / 7` once per output
pixel: 125,440 integer divisions per frame on Cortex-M33. Replaced this with an
incremental 5/7 accumulator that produces the exact same nearest-neighbour
source indices using adds, compare, and occasional increment. This addresses
CPU-side scaling cost; the doubled QSPI pixel bandwidth versus the old 320x200
view remains and may need separate frame-pacing work if lag persists.

The freeze is still ambiguous despite the enlarged 227,944-byte zone. Reworked
the `Z_Malloc` OOM path into a crash-persistent diagnostic: it writes an `OOM!`
magic value, failed allocation size, and remaining free bytes into watchdog
scratch registers 0..2, then watchdog-reboots. Early next boot detects the
record, clears the magic so a manual reset recovers normally, displays
`OOM req=<n> free=<n>`, and halts before core1/render DMA can overwrite the
message. If the next combat freeze neither reboots nor shows this report, OOM
is ruled out and investigation should move to thinker/list corruption or a
non-memory deadlock.

## 2026-08-16 (cont'd) — Swipe controls improve play; optimize scaler and persist OOM

First hardware test of the floating swipe-and-hold control model was clearly
better than fixed zones: still needs tuning, but the game became materially
more playable. Two remaining observations: full-width rendering felt laggy,
and rapid shooting during combat still eventually froze after one enemy kill.

The 448x280 scaler's hottest loop computed `output_x * 5 / 7` once per output
pixel: 125,440 integer divisions per frame on Cortex-M33. Replaced this with an
incremental 5/7 accumulator that produces the exact same nearest-neighbour
source indices using adds, compare, and occasional increment. This addresses
CPU-side scaling cost; the doubled QSPI pixel bandwidth versus the old 320x200
view remains and may need separate frame-pacing work if lag persists.

The freeze is still ambiguous despite the enlarged 227,944-byte zone. Reworked
the `Z_Malloc` OOM path into a crash-persistent diagnostic: it writes an `OOM!`
magic value, failed allocation size, and remaining free bytes into watchdog
scratch registers 0..2, then watchdog-reboots. Early next boot detects the
record, clears the magic so a manual reset recovers normally, displays
`OOM req=<n> free=<n>`, and halts before core1/render DMA can overwrite the
message. If the next combat freeze neither reboots nor shows this report, OOM
is ruled out and investigation should move to thinker/list corruption or a
non-memory deadlock.

## 2026-08-16 (cont'd) — Freeze is not OOM; end-to-end driver hardening

Hardware test froze during combat again but did **not** watchdog-reboot or show
the persistent OOM report. This rules out the normal `Z_Malloc` exhaustion
path. Reviewed the render/presentation/audio driver path end to end and found
three concrete defects that explain both lag and a silent permanent freeze:

1. `DEBUG_NO_SOUND=1` only made `S_StartSound`/music no-ops.
   `I_Pico_UpdateSound()` still mixed and synchronously pushed 512 silent
   samples through `audio_out()` whenever either render core waited. Upstream's
   audio pool is non-blocking; ours is not. Made `I_Pico_UpdateSound()` return
   immediately under `DEBUG_NO_SOUND`, so disabled audio now truly does zero
   work and cannot pace/stall the render rendezvous.
2. `AMOLED_1IN8_DisplayWindowPacked()` waited forever on
   `dma_channel_is_busy()`. A single lost PIO DREQ/state-machine stall strands
   core1 permanently; core0 then blocks on its rendering semaphore, producing
   exactly a frozen gameplay frame with no OOM/reboot. Added a 20ms timeout
   (normal 35KB tiles are far faster), DMA abort, FIFO clear, PIO restart, and
   continuation so a hardware transfer fault drops/corrupts at most a frame.
3. `bootlog_init()` and `I_InitGraphics()` both called `DEV_Module_Init()` and
   `QSPI_PIO_Init()`. This claimed/leaked a DMA channel and loaded/reinitialized
   the same PIO program twice. Made both initializers idempotent.

Build succeeds. The remaining unavoidable cost of the current quality setting
is 448x280x2 = 250,880 QSPI bytes per presented frame (versus 128,000 at native
320x200); hardware retest will show how much lag was silent-audio CPU blocking
versus display bandwidth.

## 2026-08-16 (cont'd) — Pixel-exact 320x200 diagnostic build

The driver-hardened 448x280 build still froze during active combat without an
OOM reboot. Switched presentation back to centered, pixel-exact 320x200 as a
controlled performance and memory test. This removes the 7:5 scaler entirely,
cuts QSPI traffic from 250,880 to 128,000 bytes per frame, reduces packed DMA
transfers from seven to five per frame, and shrinks the transpose tile from
35,840 to 25,600 bytes (10,240 bytes recovered). Panel GRAM is cleared once at
startup so pixels left by the previous larger build cannot appear around the
smaller image.

Audio remains fully disabled for this test. The current audio backend uses
blocking PIO writes and would add roughly 12ms bursts of synchronous work; it
must be replaced with a DMA/IRQ-fed ring or double buffer before sound can be
re-enabled without obscuring the freeze diagnosis or worsening frame pacing.

## 2026-08-16 (cont'd) — Hide boot diagnostics after graphics takeover

The pixel-exact build is noticeably faster on hardware. Its black border made
two panel-memory remnants obvious: the white bootlog strip and a thin stale
colored line. `I_InitGraphics()` cleared the panel, but later boot checkpoints
immediately redrew the bootlog outside the smaller game window. Graphics init
now disables normal bootlog rendering before the full-panel clear. A new boot
re-enables diagnostics, so early boot failures and the persistent OOM report
remain available; once Doom owns the panel, later checkpoints are no-ops and
cannot add display DMA work or repaint the border.

## 2026-08-16 (cont'd) — Re-enable SFX with asynchronous DMA audio

Replaced the blocking ES8311/PIO output path before re-enabling sound effects.
The Waveshare/mp3player driver was valid for a dedicated player loop, but its
`pio_sm_put_blocking()` loop stalled whichever Doom render core called it for
the full 512-sample block (about 11.6ms at 44.1kHz). The new backend claims a
separate DMA channel, uses DMA IRQ 1 (display DMA does not use it), and owns two
static 512-frame output buffers. The IRQ continuously schedules the oldest
queued block or a static silence block on underflow. Mixing checks for a free
buffer and returns immediately when both are occupied; game/render code never
waits for I2S timing. If codec clocks or the PIO DREQ stop, audio can fall
silent but no game core blocks on the stuck DMA.

Also extended `update_sound_mutex` across **all** channel starts, stops, volume
updates, playing-state reads, and mixing. Previously it serialized two mixers
but still allowed core0 to replace/decompress a channel while core1 was mixing
the same ADPCM state. Removed the obsolete per-sound bootlog tracing, saturated
mix additions before narrowing to `int16_t`, and corrected pitch scaling (the
old non-normal-pitch formula divided by `pitch` again and cancelled the pitch
change).

`DEBUG_NO_SOUND` is no longer defined, so SFX are active. Music remains
explicitly isolated behind `DEBUG_NO_MUSIC=1`: `i_oplmusic.c` is still a stub,
and the normal named-lump lookup is not suitable for this MUSX-compressed WAD.
The linked build succeeds with `__end__=0x200466a8`, leaving 235,864 bytes in
the short-pointer zone through `0x20080000`. Hardware testing is still required
for codec output, menu progression, combat audio, performance, and stability.

Hardware result: the build boots, progresses normally, and sound effects play
correctly on the ES8311/speaker. No music is heard, as expected with
`DEBUG_NO_MUSIC=1`; this confirms the asynchronous SFX milestone independently
of the unfinished music backend. Longer combat/freeze testing remains open.

## 2026-08-16 (cont'd) — Add fixed-memory MUSX music experiment

The shareware WAD is not missing music. It contains `D_E1M1` through
`D_E1M9`, intermission/title/victory tracks, and `GENMIDI`; `whd_gen` explicitly
converts the music set to compressed `MUSX` and retains each music lump name in
the WHD named-lump index. The previous silence came only from
`DEBUG_NO_MUSIC=1` and the placeholder `i_oplmusic.c`.

For the first hardware experiment, chose a small integer synthesizer instead
of immediately importing the full emu8950 AdLib emulator. Nine static voices
play the real MUSX note/program/volume stream with triangle, square, saw,
pulse, and noise waveforms. This is intentionally a lightweight chiptune
rendering rather than exact OPL timbre: it has a bounded inner loop, no rate
converter, and mixes into the same non-blocking 512-sample DMA producer buffer
before SFX are added. The generator is detached whenever no song is active so
silent music cannot consume continuous mixer/DMA time.

MUSX file and iterator objects are now fixed static storage in the embedded
configuration, removing the parser's per-song `malloc()` calls. Music state is
protected by its own cross-core mutex; the audio callback uses a try-lock and
emits silence instead of blocking if a level transition is changing the song.
Loop restart remains deferred when `pd_render.cpp` marks a deep render stack.

The release build succeeds with `__end__=0x20047018`, leaving 233,448 bytes in
the short-pointer zone through `0x20080000`. This is 2,416 bytes less zone than
the hardware-proven SFX-only build. Hardware confirmation of boot, recognizable
music, SFX/music balance, frame pacing, and combat stability is still required.

## 2026-08-16 (cont'd) — Music works; default to effects only

Hardware testing confirmed the complete music path works: the WAD music is
present, MUSX decoding succeeds, menu/level playback starts, and synthesized
music reaches the ES8311 speaker alongside sound effects. The missing-music
question is therefore resolved.

The nine-voice integer synthesizer is intentionally lightweight, however, and
its chiptune approximation is not enjoyable through this device's small
speaker. Sound effects alone fit the hardware better. Music is now an optional
experiment rather than the product default: the full implementation remains
in `i_oplmusic.c`, while CMake's `DOOM_ENABLE_MUSIC` option defaults to `OFF`.
It can be restored without code changes using:

```
cmake -S firmware -B firmware/build -DDOOM_ENABLE_MUSIC=ON
```

The default build defines `DEBUG_NO_MUSIC=1` and excludes the MUSX/MIDI parser
sources, avoiding active voices, continuous mixing, and music-specific parser
state. Both modes build successfully. The final effects-only build has
`__end__=0x20046ae8`, leaving 234,776 bytes in the short-pointer zone through
`0x20080000`; the optional music build remains at 233,448 bytes. Improving the
synthesizer timbre can be revisited later without risking the stable SFX path.

## 2026-08-17 — Map BOOT long-press to Escape with flash-safe lockout

BOOT is the natural remaining Escape/menu input, but it is not an ordinary
GPIO: pressing it pulls the external flash CS line low. The existing reader
temporarily floats CS and executes from RAM with local interrupts disabled,
which was safe only in the original single-core calibration firmware. Doom's
render core continuously executes code and reads WAD data through XIP, so
calling that reader directly from core0 could corrupt a fetch or hang core1.

The game now registers core1 with the Pico SDK's multicore lockout before
announcing that core as ready. The SDK's FIFO IRQ moves core1 into a
RAM-resident loop with interrupts disabled. Core0 requests that lockout with a
2ms timeout, samples BOOT only after acknowledgement, restores flash CS, and
then releases core1. If core1 cannot acknowledge promptly the sample is
skipped, not blocked, so the new input cannot introduce an unbounded wait.
No other game code uses the inter-core FIFO.

BOOT is sampled at no more than 20Hz to minimize render interruptions. A
continuous 1.2-second hold emits one `KEY_ESCAPE` key pulse; continued holding
does not repeat, and short presses remain unassigned for a possible weapon
switch. The effects-only build succeeds with `__end__=0x20046bc4`, leaving
234,556 bytes in the short-pointer zone. The optional-music configuration also
builds and leaves 233,240 bytes. Hardware validation of menu activation,
release/repeat behavior, frame pacing, and combat stability remains required.

## 2026-08-17 — Research: BOOT input is valid generally, deferred for Doom

Follow-up research found that using BOOTSEL as runtime input is an officially
supported Raspberry Pi technique, not inherently an invalid modification. The
official `pico-examples` repository includes a `button` example that temporarily
suspends flash access, and the SDK provides `flash_safe_execute()` plus
`flash_safe_execute_core_init()` for coordinated multicore flash safety:

- https://github.com/raspberrypi/pico-examples
- https://github.com/raspberrypi/pico-sdk/releases
- https://github.com/raspberrypi/pico-sdk/issues/2243

No official Waveshare example was found that remaps BOOT during runtime on this
exact RP2350-Touch-AMOLED-1.8 board; its documentation uses BOOT only to enter
the ROM loader:

- https://www.waveshare.com/wiki/RP2350-Touch-AMOLED-1.8

The likely distinction is workload. The official button example is small,
whereas Doom executes and streams WAD data from external flash across two cores
while display/audio DMA and interrupts remain active. Our implementation did
lock out core1, but both BOOT-enabled hardware trials behaved abnormally and
the otherwise equivalent pre-BOOT build did not. This correlation is strong,
but the precise failure mechanism and the burning odor remain unproven.

Decision: this is not a current priority. Runtime BOOT remains absent from the
playable firmware. If revisited later, first build a standalone single-core,
audio/amplifier-disabled test using the SDK's higher-level flash-safety API,
then expand one subsystem at a time. Do not experiment inside Doom first.

## 2026-08-17 — Abandon runtime BOOT input and restore the safe baseline

The pre-BOOT-polling effects-only firmware was restored and ran normally. A
second controlled build based on that same safe source plus only debounced
single-press BOOT/Escape handling again behaved abnormally. At Alexander's
request, runtime BOOT input is now abandoned rather than tuned further.

Removed the BOOT reader from the Doom target, the 20Hz polling and Escape pulse,
and core1's multicore-lockout victim registration. BOOT is reserved exclusively
for entering the ROM BOOTSEL loader during power-on/reset. The exact cause of
the abnormal sound/odor is still not proven, but runtime flash-CS manipulation
is the common change and is not worth retaining for one game action. Escape
must move to a PWR or touch gesture.

## 2026-08-17 — Simplify BOOT Escape to one press; deployment paused

The first hardware boot of the long-press Escape build produced abnormal loud
audio and a burning smell. The board had no battery, was disconnected
immediately, and must remain unpowered until the speaker/NS4150B amplifier,
regulator, USB-power area, and board surfaces are inspected. There is no proven
causal link to BOOT polling: that commit did not change the audio configuration
or amplifier control, and the deployed build had music disabled. Nevertheless,
no more deployments are appropriate before physical inspection.

At Alexander's request, the gesture itself is simplified locally from a
1.2-second hold to one press. BOOT remains sampled at 20Hz using the same bounded
multicore flash-CS lockout. Two consecutive pressed samples debounce the input;
the first stable pressed edge emits one `KEY_ESCAPE`, continued holding does not
repeat, and release rearms it. This is a gesture change only: a single press
still requires runtime BOOT/flash-CS sampling and therefore does not remove that
mechanism or its technical risk.

## 2026-08-17 — Plan selectable tilt controls and a measured refactor

The earlier decision against tilt as the primary control is softened, not
silently discarded. This board includes a QMI8658 six-axis IMU, so motion
control is technically inexpensive enough to prototype. Research comparing
mobile-game input found touch produced better objective performance in one
game while tilt was perceived as more engaging, and another shooter study
found that tilt can usefully supplement touch. A separate order-of-control
study found that mapping matters at least as much as the input device: direct
position control substantially outperformed velocity control in its task.
Sources:

- https://www.yorku.ca/mack/mhci2013h.html
- https://doi.org/10.1016/j.entcom.2017.04.005
- https://www.yorku.ca/mack/ie2014.html
- https://developer.android.com/develop/sensors-and-location/sensors/sensors_motion
- https://www.waveshare.com/wiki/RP2350-Touch-AMOLED-1.8

The current floating swipe-and-hold scheme is preserved as **Control Model A**:
the initial touch is a movable anchor, a 24-pixel drag selects one cardinal
direction with an 8-pixel dominant-axis bias, holding sustains that digital
direction, PWR fires/selects (double press uses/backs out), and a 1.2-second
BOOT hold emits Escape. The original fixed-zone scheme remains a compile-time
fallback. This is the working baseline against which experiments are judged.

Planned experiments, behind a small selectable input-model boundary:

1. **Model B — full tilt.** Average a stationary 0.5-1.0 second window to set
   the player's neutral pose. Pitch controls forward/back and roll controls
   left/right turning. Use a dead zone plus hysteresis and a nonlinear response.
2. **Model C — hybrid (preferred first experiment).** Let tilt steer left/right
   while touch swipe-and-hold controls forward/back. This avoids asking the
   same hand motion to steer, keep the screen readable, and operate touch on
   both axes simultaneously.
3. **Model D — gyro-assisted hybrid.** Use accelerometer gravity for the stable
   neutral/low-frequency attitude and gyro rate for responsive turning, with an
   explicit recenter action. This is more capable but adds drift/filter tuning.

Vanilla key events are binary, so angle-dependent speed should eventually feed
bounded values directly into `ticcmd_t.forwardmove` and `ticcmd_t.angleturn`.
The lower-risk first prototype can use two thresholds (walk and run/fast turn)
before that engine integration. Poll on the existing core0 input path at about
35-50Hz, use one burst read, fixed-point filtering, and static state only. I2C1
is shared with touch, audio codec, RTC, and power management, so IMU access must
remain serialized and bounded. Do not import Waveshare's driver unchanged: its
register-write retry return logic is known broken in the supplied bundle.

The proposed performance work will be a measured phased refactor, not a broad
rewrite while the combat freeze is still unidentified. First record zone
headroom, stack high-water marks, game/render timing, DMA waits, and audio queue
pressure. Then isolate display presentation, input models, audio, and diagnostics
behind behavior-preserving interfaces. Make one static-buffer/dead-code/data-flow
change at a time and hardware-test it, recording SRAM and frame-time deltas. This
keeps regressions bisectable and avoids mistaking code movement for optimization.

## 2026-08-17 — Prioritise an enjoyable handheld experience; performance before controls

Alexander confirmed the safe effects-only build progressed from E1M1 into E1M2
without freezing and felt generally good. This does not prove unlimited
stability, but the earlier combat freeze is no longer reproduced in the latest
meaningful run and should not block every other experiment.

The product goal is now explicit: move beyond "Doom runs" to a handheld port
that is enjoyable and controllable enough to finish. Full-width 448x280 is the
visual ambition, but not at the expense of frame pacing, input response, sound
effects, or the short-pointer memory margin. Music remains optional because the
working lightweight synthesizer sounded worse than effects-only audio on this
speaker. Later experience ideas—battery indication, a port settings menu, and
an optional reduced/hidden vanilla HUD—come after performance and controls.

Engineering order is performance first, then detailed motion-control tuning.
The next phase measures game tics, rendering, packing/scaling, DMA/QSPI waits,
audio queue pressure, stack high-water marks, zone headroom, and input latency.
Video experiments then advance through 384x240 and 416x260 toward 448x280. A
display-driver rewrite is justified only if profiling shows the current
presentation boundary is the constraint; the proven non-blocking SFX driver is
preserved unless measurements disagree.

Once the best sustainable view is locked, the first motion experiment is
Alexander's proposed hybrid: horizontal touch/drag turns left/right while
device pitch controls forward/back and neutral stops movement. This will be
compared with full tilt and the alternate tilt-steering/touch-movement hybrid.
Accelerometer gravity supplies stable pitch/roll, gyro rate can add fast
response, and any combined model needs neutral calibration, filtering, dead
zone/hysteresis, nonlinear bounded output, and explicit recentering. Detailed
phases and acceptance rules are now in `docs/ROADMAP.md`.

The same session established a safer development loop on the new Mac. The full
16 MiB flash was backed up, the source-unchanged SDK 2.3.0 build matched the
installed 382,228-byte firmware range byte-for-byte, and `picotool load -v -f
--ser ...` rebooted the running firmware into ROM BOOTSEL, wrote and verified
the UF2, then rebooted into Doom without Alexander pressing BOOT. This
host-driven reset does not reverse the decision against application-side BOOT
sampling.

## 2026-08-17 — Add measured selectable presentation before rewriting the driver

The first performance implementation keeps Doom's renderer at its native
320x200 indexed output and makes only the AMOLED boundary selectable. CMake now
accepts 320x200, 384x240, 416x260, or 448x280. The scaled path composes palette
and overlays once per source row, then uses biased integer accumulators whose
output exactly matches `floor(output * source / target)` on both axes. Pixel
byte-swap and portrait tile transpose remain fused; there is no scaled-frame
allocation and no per-output-pixel division.

Profiling is opt-in and reports one summary per 128 frames. It separates game,
render, display-frame wait, core1 rendezvous, presentation preparation, AMOLED
transfer, complete cadence, and display DMA recovery counts. USB output is
bounded to a 2ms timeout and the sampling window resets even when no host is
connected. The normal 320 build retains the established 234,776-byte zone;
the 448 build retains 224,536 bytes. The scaled presenter's largest measured
function frame is approximately 1.7KB on core1's dedicated 4KB stack.

Every mode builds in Release, but no performance conclusion is claimed until
320 and 448 are captured on the same hardware route. The first deployment
attempt did not change flash: the existing application enumerated CDC and its
correct vendor Reset interface, but reset control requests stalled despite
successful interface claim. This is a runtime USB-state problem, not evidence
against the candidate image; manual BOOTSEL remains the recovery path.

## 2026-08-17 — Make profiling reset-persistent and remove duplicate USB initialisation

The live gameplay capture failed in a specific way: macOS retained the Pico
CDC device and the correct vendor Reset interface, but neither serial output
nor reset control requests were serviced. This was not consistent with a Mac
permission restriction because the same machine had already opened, flashed,
and verified the board. Source inspection found that `bootlog_init()` called
the reused Waveshare `DEV_Module_Init()`, which called `stdio_init_all()`, and
then Doom's `main()` called `stdio_init_all()` again. TinyUSB was therefore
initialised twice in one boot. Stdio ownership now sits at each executable
entry point; the reusable hardware module no longer changes USB lifecycle.

Profiling no longer requires USB to remain live during gameplay. An opt-in
build waits for `GS_LEVEL` with a real user game, measures 384 presented frames,
stores the aggregate report plus checksum in the linker's non-zeroed SRAM
section, writes only a report magic to watchdog scratch, and reboots. It does
not erase or program flash, so the firmware/WAD gap and WAD contents are not at
risk. The next boot validates the checksum and stops before game graphics,
core1, or audio start, repeating short report lines over USB and cycling key
figures through the existing one-line AMOLED bootlog. A further reset clears
the report mode and starts Doom normally.

ARM GNU 15.3 Release builds succeed for both comparison modes. The persistent
report and report-screen state move instrumented `__end__` to `0x20046b9c` at
320x200 (234,596 zone bytes) and `0x2004939c` at 448x280 (224,356 zone bytes).
The normal non-profiled memory baseline remains unchanged. Hardware deployment
and the first captured report are still pending.

## 2026-08-17 — Persist reports after reboot and measure one full minute

Hardware proved the reset-retained capture and AMOLED report state, but CDC
text remained silent even after duplicate TinyUSB initialisation was removed.
An attempted host read of live SRAM also proved unsuitable because entering
ROM BOOTSEL clears that RAM before `picotool` can save it. The report handoff
therefore uses reserved flash, but never while gameplay or core1 is running:
the watchdog first reboots, the checksum is validated from retained SRAM, then
the single-core boot writes one page after erasing sector
`0x101ff000..0x101fffff`. The WHD begins at `0x10200000`, so the log cannot
overlap game data. `picotool` can then read and verify the record after ROM has
cleared SRAM.

The recovered first 320x200 non-combat capture measured 384 frames: average
cadence 22,917us (max 37,970), presentation 15,439us (max 19,544), CPU
preparation 11,663us, transfer 3,775us (max 4,168), render last/max
8,102/18,610us, display wait last/max 3/8us, and zero DMA timeouts. Its game
maximum included a 1,139,117us level-entry spike, so it is directional rather
than the final baseline. CPU preparation is already much larger than panel
transfer, making compose/transpose the leading display optimisation target.

Alexander could not reach enemies before the 384-frame cutoff. The next format
uses a 3-second warm-up to exclude level-entry work and then measures a true 60
seconds using hardware time, independent of frame rate. Report version 2 stores
the measured duration as well as sample count. The persistent-log profiler
leaves 233,648 zone bytes at 320x200 and 223,408 at 448x280; normal builds are
unchanged and pay none of this diagnostic cost.

## 2026-08-17 — Use the one-minute combat baseline to target CPU presentation preparation

The autonomous 320x200 capture completed successfully during real combat and
persisted a checksum-valid version-2 report. It recorded 2,507 presented frames
over 60,015,239us, or 41.8 FPS. Average/max frame cadence was
23,948/42,958us; core1 work 7,091/22,905us; frame wait 1,367/7,047us; and
presentation 15,460/20,351us. Within presentation, CPU preparation averaged
11,664us while panel transfer averaged 3,795us and peaked at 4,197us. The last
and maximum renderer samples were 12,457/26,048us. Display wait never exceeded
8us and no DMA timeout occurred.

The averages account for essentially the entire measured cadence: 7,091us of
core1 work, 1,367us of rendezvous wait, and 15,460us of presentation total
23,918us against the observed 23,948us cadence. CPU preparation alone consumes
75.4% of presentation time and 48.7% of the average frame, while physical panel
transfer consumes 15.8% of the frame. This makes the fused
compose/scale/transpose loop—not QSPI bandwidth, display waiting, or DMA
recovery—the first optimisation target.

Do not rewrite the whole video driver before testing focused changes to packing
locality and transpose-tile height. The current 40-row tile is now an
experimental parameter rather than a permanent choice. Preserve the existing
driver boundary and healthy asynchronous transfer path until measurements show
that it prevents the target view size or frame rate.

## 2026-08-17 — Make transpose-tile height selectable and test eight rows first

The first focused candidate keeps the proven presenter and packed-DMA driver
boundary intact but changes the transpose tile from a hard-coded 40 rows to a
CMake comparison variable. AUTO selects eight rows for 320x200, 384x240, and
448x280, and ten rows for 416x260 so every mode divides exactly. The original
40-row path remains selectable for controlled A/B builds.

At 320x200 the live tile falls from 25,600 to 5,120 bytes and `__end__` moves
from `0x20046ae8` to `0x20041ae8`, increasing normal zone headroom from 234,776
to 255,256 bytes. At 448x280 it falls from 35,840 to 7,168 bytes and `__end__`
moves from `0x200492e8` to `0x200422e8`, increasing zone headroom from 224,536
to 253,208 bytes. Full width now costs only 2,048 zone bytes versus native,
rather than 10,240. Instrumented headroom is 254,128 and 252,080 bytes.

The expected CPU benefit is a shorter destination stride—16 bytes instead of
80—during the measured hot transpose loop. The tradeoff is 25 rather than five
packed transfers at 320, and 35 rather than seven at 448. All four comparison
builds pass, but neither image correctness nor a performance improvement is
claimed until measured on the physical panel.

The first physical 8-row 320x200 boot returned its application USB interface
but left the AMOLED black. No timing capture was attempted. This rejects the
candidate as a playable default and shows that build success plus memory
recovery are insufficient for this panel path. Forty rows is restored as the
default; alternate heights remain available only for isolated driver work.
Rather than ask Alexander to repeat gameplay at the old size, the next
experience test uses the full-width 448x280 40-row profiler, which retains
223,408 bytes of instrumented zone headroom.

## 2026-08-17 — Treat full-width 30 FPS as an achievable near-term target

The 448x280 40-row image was flashed, byte-verified, and visually confirmed on
the physical panel before measurement. Its reset-persistent report then
captured 1,724 frames over 60,026,056us during real gameplay and combat, or
28.7 presented FPS. Average/max cadence was 34,838/46,520us; core1 work
3,781/13,229us; frame wait 2,094/3,740us; and presentation
28,934/35,225us. Presentation split into 21,803us CPU preparation and
7,130us average transfer, with transfer peaking at 7,600us. The last/maximum
renderer samples were 22,254/31,316us. Display wait never exceeded 9us and no
DMA timeout occurred.

Full-width pixel count is 1.96 times native while measured presentation cost
rose 1.87 times, so scaling is broadly efficient but still CPU-heavy. CPU
preparation consumes 62.6% of average cadence and transfer 20.5%. Reaching
30 FPS requires reducing the 34,838us average cadence by roughly 1,505us, only
4.3%; reaching 35 FPS would require roughly 6,267us, or 18.0%. The engineering
decision is therefore to keep 448x280 as the active target and pursue bounded
packing/scaling improvements first. A wholesale video-driver rewrite or lower
resolution is not justified by this result.

## 2026-08-17 — Service audio between display chunks instead of growing the queue

Alexander judged full-width gameplay playable to slightly sluggish, but menu
effects lagged and sounded stretched. This is consistent with an audio
underflow rather than a bad sample: each 512-sample block lasts 11.6ms at
44.1kHz, so the two-buffer DMA queue covers 23.2ms. Measured full-width
presentation averages 28.9ms and peaks at 35.2ms. Simple menu frames also make
fewer incidental renderer-side `SafeUpdateSound()` calls than active gameplay.

The first fix keeps the existing two-buffer DMA/IRQ backend and calls the
non-blocking `I_UpdateSound()` after each completed 40-row packed panel
transfer. At 448x280 this creates seven refill opportunities per presentation,
after the display DMA mutex has been released. The mixer already uses a
try-lock and returns immediately when no effect is active or the queue is full.
This adds no audio buffers, queue latency, or static-memory cost. Normal and
profiled 320x200 and 448x280 Release images build with unchanged linker
endpoints. Hardware listening is required before accepting the change.

## 2026-08-17 — Pipeline 20-row tiles without increasing static memory

The measured synchronous presenter spends 7.1ms per full-width frame inside
panel calls even though transfers use DMA, because every packed-window call
waits for DMA before returning. The first driver-level optimisation therefore
separates packed submission from completion and lets core1 compose the next
tile while the panel consumes the previous one.

The candidate is compile-selectable with `DOOM_ASYNC_AMOLED=ON` and requires
20-row tiles. Two buffers together contain the same 17,920 RGB565 pixels, or
35,840 bytes, as the proven single 40-row tile. The AMOLED driver keeps the
shared display mutex and chip-select transaction from start through bounded
wait/recovery, so bootlog cannot reconfigure the shared DMA/PIO state during an
active transfer. The synchronous API is implemented through the same paired
operations, while the existing 40-row presentation remains the default.

Normal and profiled 448x280 Release builds succeed with unchanged linker
endpoints (`0x200492e8` and `0x20049750`). Profiling counts residual wait plus
command/submission as blocking display time; DMA hidden behind packing is not
double-counted. The next gate is a short physical correctness/audio check,
because the earlier eight-row experiment proved that build success cannot
establish panel behavior. No performance improvement is claimed yet.

The short physical gate then passed. The profiled 448x280 candidate was flashed
and byte-verified; Alexander confirmed a visible, normally updating Doom menu
and good menu sound without the previously reported stretching. This accepts
the 20-row transaction ordering, alternating-buffer lifetime, and interleaved
audio service for a bounded gameplay measurement. It does not yet accept the
pipeline as faster; that decision requires the comparable one-minute report.

The comparable report is now complete and checksum-valid. It captured 2,024
frames over 60,011,526us, or 33.7 FPS. Average/max cadence was
29,664/46,151us; core1 work 4,097/16,145us; frame wait 1,593/3,743us; and
presentation 23,940/29,114us. CPU preparation averaged 22,404us, while the new
blocking display-service measure averaged 1,535us and peaked at 4,078us.
Render last/max was 19,504/28,653us, display wait peaked at 9us, and no DMA
timeout occurred.

Against the synchronous full-width baseline, average cadence improves by
5,174us (14.9%) and presentation by 4,994us (17.3%). Blocking panel cost falls
by 5,595us, meaning the two-buffer pipeline hides about 78% of the old 7,130us
transfer time. Static memory is unchanged. Accept the asynchronous presenter
as the active full-width optimisation direction. Reaching a 35 FPS average now
requires only about 1,093us more, so optimise the remaining CPU packing loop
before changing the driver architecture again.

The first post-pipeline CPU candidate targets duplicated vertical scale rows.
At 448x280, 80 of 200 source rows produce two adjacent output rows. When the
first copy does not close a 20-row tile, both are now written in one strided x
loop, loading each scaled pixel once and sharing loop control. Tile-boundary
cases retain the proven single-row path. This uses no additional buffer and an
exact host simulation produces the same source-row sequence at 384x240,
416x260, and 448x280. All synchronous/asynchronous normal/profile builds pass
with unchanged static endpoints.

To reduce Alexander's repeated testing burden, profile capture duration is now
a positive-integer CMake setting used only by `i_video.c`. Intermediate builds
can capture 20 seconds without rebuilding every engine source; the final
accepted benchmark remains 60 seconds. The paired-row 448x280 asynchronous
20-second candidate is build-complete but not yet hardware-validated, so no
speed improvement is claimed.

The 20-second hardware report is checksum-valid and clearly positive. It
captured 727 frames over 20,008,747us, or 36.3 FPS. Average/max cadence was
27,560/43,794us; core1 work 4,067/15,009us; frame wait 1,636/2,993us; and
presentation 21,842/27,674us. CPU preparation averaged 20,404us and blocking
display service 1,437us, with display wait at most 9us and zero DMA timeouts.

Compared with the one-minute 33.7 FPS pipeline baseline, cadence falls by
2,104us and CPU preparation by 2,000us while display service changes by only
98us. This isolates the gain to the paired-row CPU work rather than a transfer
timing accident. Accept the optimisation directionally because it clears the
35 FPS target without memory cost. Require one final 60-second representative
combat capture before treating full-width video performance as locked.

The final one-minute report is checksum-valid and meets the gate. It captured
2,110 frames over 60,004,468us, or 35.2 FPS. Average/max cadence was
28,451/46,503us; core1 work 4,596/18,042us; frame wait 1,603/3,768us; and
presentation 22,226/27,049us. CPU preparation averaged 20,709us, blocking
display service 1,516us and peaked at 3,843us, display wait peaked at 11us,
and no DMA timeout occurred.

Compared with the original 28.7 FPS synchronous full-width baseline, cadence
improves by 6,387us (18.3%) and presentation by 6,708us (23.2%). The normal
full-width image still ends at `0x200492e8`, leaving 224,536 zone bytes. Lock
the two-buffer asynchronous presenter plus paired-row packing as the video
baseline; a wholesale driver rewrite is no longer justified. The normal
non-profile image was flashed and byte-verified after measurement so ordinary
play no longer enters the flight recorder.

## Open questions
- **Earlier freeze during active combat** — not ordinary zone OOM and not cured
  by removing silent audio work or bounding display DMA waits, but it did not
  recur in the latest run through E1M1 into E1M2. Repeat that route before
  either closing it or adding persistent stage/heartbeat diagnostics.
- Continue extended combat testing of the hardware-verified DMA/IRQ SFX path.
- Optional: improve the fixed-memory MUSX synthesizer's timbre before
  reconsidering music as the device default.
- What actually drives title-screen advancement in a `PD_COLUMNS` build,
  since vanilla's `D_PageTicker` path is compiled out (see above).
- How much can cache-local packing and a smaller transpose tile reduce the
  measured 11,664us CPU preparation cost, and what does the equivalent
  448x280 combat capture then sustain?
- Does optional roll strafing improve the same short combat route versus
  touch-only navigation without introducing accidental movement or fatigue?
- Can proportional QMI8658 input feed `ticcmd_t` directly without affecting
  deterministic tic behavior or shared-I2C latency?
- Exact board-configured AXP2101 timing has not been read at runtime. The
  datasheet range is now established: long-press IRQ 1–2.5 seconds and
  physical power-off 4–10 seconds. F12 intentionally acts at 450ms without
  modifying those registers.
- WHD_SUPER_TINY/DEMO1_ONLY choice (see fix #5 above) was made for
  shareware's sake; swapping in Alexander's own multi-episode retail WAD
  later will need `whd_gen` and these defines revisited together.
