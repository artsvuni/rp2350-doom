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

## Open questions
- BOOT long-press conflict (bootloader-entry vs in-game menu) - not yet
  resolved, see above.
- Exact AXP2101 long-press duration threshold - observed to work, exact
  timing not measured or found in a quick datasheet pass. Not blocking;
  revisit only if it turns out to feel wrong in actual play.
