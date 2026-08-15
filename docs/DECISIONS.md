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

## Open questions
- BOOT long-press conflict (bootloader-entry vs in-game menu) - not yet
  resolved, see above.
- Exact AXP2101 long-press duration threshold - observed to work, exact
  timing not measured or found in a quick datasheet pass. Not blocking;
  revisit only if it turns out to feel wrong in actual play.
