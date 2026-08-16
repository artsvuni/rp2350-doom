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

## Open questions
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
