# Decisions Log

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

## Open questions
- **Freeze during actual gameplay after a rapid touch-event burst** (see
  immediately above) - not yet investigated. Suspect the touch-input
  event path, not game logic or zone memory (ruled out via the `OOM:
  Z_Malloc` checkpoint not firing).
- Making the audio path non-blocking (see above) - the real fix,
  not yet attempted. `DEBUG_NO_SOUND=1` is a working stopgap.
- Touch/PWR input not being detected at all (see above) - next thing to
  debug.
- What actually drives title-screen advancement in a `PD_COLUMNS` build,
  since vanilla's `D_PageTicker` path is compiled out (see above).
- Letterbox padding noise (cosmetic, see above) - needs a single-
  transfer-based panel clear, not `AMOLED_1IN8_Clear()` as-is.
- BOOT long-press conflict (bootloader-entry vs in-game menu) - not yet
  resolved, see above.
- Exact AXP2101 long-press duration threshold - observed to work, exact
  timing not measured or found in a quick datasheet pass. Not blocking;
  revisit only if it turns out to feel wrong in actual play.
- Button/touch control wiring into the actual game - not started; boot/
  render bring-up has been the entire focus so far.
- WHD_SUPER_TINY/DEMO1_ONLY choice (see fix #5 above) was made for
  shareware's sake; swapping in Alexander's own multi-episode retail WAD
  later will need `whd_gen` and these defines revisited together.
