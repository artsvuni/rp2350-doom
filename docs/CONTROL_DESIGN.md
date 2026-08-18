# Handheld Control Design

## Experience goal

The control system should make combat and exploration enjoyable on a 1.8-inch
device, not merely make every Doom command technically reachable. The player
must be able to stop, turn precisely, fire without delay, and open a door
without changing grip or covering much of the display.

## Control evolution and usability record

The first genuinely playable model used a floating touch anchor. Touching
anywhere established neutral; vertical displacement moved forward/back and
horizontal displacement turned. It proved that one finger could control both
axes and made E1M1 playable, but it never became comfortable on this device:

- useful turns required repeated swipes and too much travel across the view;
- a broad thumb often changed contact area without moving the FT3168's reported
  centroid, while a pointing finger responded much more precisely;
- dead zones felt like input delay, but removing them exposed abrupt gain and
  touch noise; and
- combat forced the finger over too much of the 1.8-inch image.

Three pitch-movement experiments and two roll-strafe interpretations were also
tested. They could produce movement, but grip-dependent neutral, delayed start,
difficult stopping, accidental reversal, and uncommanded strafing made motion
untrustworthy for an essential action. Motion was therefore removed from the
accepted model rather than endlessly tuning sensitivity around an unstable
interaction.

The successful direction began with F13's fixed touch-and-hold D-pad. Absolute
position felt more like traditional keyboard Doom and, importantly, worked
with a thumb. F14 then replaced the radial geometry with the current asymmetric
edge-aware zones: narrow LEFT and DOWN targets against tactile panel edges,
larger UP and RIGHT targets, no neutral gap, and two narrow forward-turn bands.
The visible guide overlay improved discoverability enough to remain part of the
playable build. F14.1 restored double-tap Use/Open inside every zone without
delaying movement.

PWR release/hold then closed Fire and Escape, and F15's flash-safe BOOT release
closed next weapon. The remaining usability failure was strafe: bottom-corner
double-tap bursts worked technically but were awkward and non-continuous during
combat. F16 resolves that with a classic modifier. Hold BOOT and LEFT/RIGHT
become sustained strafe; release BOOT and the same zones turn again. Short BOOT
release remains next weapon. Alexander's hardware test found this worked great
and completed the strongest, most comfortable control system so far.

### Accepted F16 control model

| Input | Action |
|---|---|
| Hold UP / DOWN | Move forward / backward |
| Hold LEFT / RIGHT | Turn left / right |
| Hold an UP boundary | Move forward while turning |
| Double tap | Use/Open |
| PWR short release | Fire; confirm in menus |
| PWR 450 ms hold | Escape; back in menus |
| BOOT short release | Next owned weapon |
| Hold BOOT + LEFT / RIGHT | Sustained strafe left / right |
| Hold BOOT + UP boundary | Move forward while strafing |

The fine sensitivity and zone geometry remain tunable later, but they are no
longer blocking another subject. This exact configuration is the controls
baseline for display experiments.

### Accepted F17: one fixed control language across game states

F16 accidentally reverted to the legacy floating swipe whenever gameplay left
`GS_LEVEL`. F17 makes the visible fixed zones consistent without pretending
that every Doom state consumes the same internal command:

| State | Fixed-zone meaning | PWR short release |
|---|---|---|
| Running level | Existing movement and turning | Fire |
| Menu open | Cardinal menu navigation | Select/confirm |
| Intermission/score | Any fresh contact sends native Fire | Native Fire |

Intermission touch remains disarmed until the touchscreen has been released
once after entering the score screen. This prevents a thumb held on movement
at level completion from immediately accelerating or skipping the statistics.
One or more deliberate contacts then use Doom's original Attack-driven
progression path; there is no level-transition shortcut or altered game timer.

DOWN is also made view-relative for every 448-wide build. It always occupies
the final 20 pixels of the rendered image, plus whatever centred black border
exists below it. This preserves the accepted 448x280 appearance and restores
the same shallow visual target at 448x336 instead of allowing it to grow from
20 to 48 visible pixels.

In accepted F18 448x368 there is no black border below DOWN, so its visible and
tactile target is exactly the final 20 panel pixels. F17 448x336 preserves the
20-pixel image target plus its 16-pixel edge border as the rollback geometry.

The hardware test passed: fixed-zone menu navigation and score-screen
progression both work, and DOWN is shallow again. Alexander also found the old
menu swipe surprisingly enjoyable. That is retained as a future A/B candidate,
not dismissed: test a deliberately menu-only swipe option against F17's fixed
zones while keeping intermission on its native Fire mapping.

## Candidate C1 result: direction good, motion model rejected

The first physical test confirmed that horizontal touch turning felt responsive
and enjoyable, and touchscreen double-tap reliably opened a door. It rejected
two other parts of the model:

- proportional absolute tilt required an uncomfortably large movement from
  Alexander's natural roughly 11-o'clock grip toward 9 o'clock before walking
  became useful; and
- held PWR fired several shots, then the PMIC long-press event opened the menu;
  a longer hold subsequently appeared to restart the game.

That version is rejected. It waited for the PMIC's own long-press event and
allowed fire to repeat for the entire hold. F11 therefore returned PWR to a
tap-only one-shot fire input while Escape/menu remained unresolved. Candidate
F12 revisits the interaction with a much shorter software timer and explicit
release suppression; it does not restore held automatic fire.

## Candidate C2 result: stateful tilt gearbox rejected

Movement is now a three-state controller rather than an analogue accelerator:

```text
reverse  <->  stopped  <->  forward
```

A physical test rejected this interpretation. Forward tilt initially selected
backward movement, state changes arrived late, walking continued unexpectedly,
and returning the device did not give the player a dependable stop. Rebasing
every settled pose made the reference move underneath the player; the low
threshold and filter delay compounded the unpredictability.

## Candidate C3 result: fixed-neutral pitch still rejected

The direct-position version was more understandable than the gearbox and could
move forward, but it still failed the experience goal. A narrow activation
point was difficult to find, small accidental tilts could start sustained
walking, and an attempted stop could cross neutral and immediately become
reverse. Alexander described the result as controlling water: technically
possible to learn, but too difficult to trust during play.

This is consistent with published mobile-game research. Touch generally wins
on speed and accuracy, while tilt can remain engaging; a dual-control shooter
study found tilt useful for movement but also recorded continuous unintended
movement when players rested just outside its dead zone. Wrist-dexterity work
identifies pronation/supination (device roll) as one of the strongest tilt axes,
and order-of-control studies favour a direct held position over a velocity or
latched interpretation.

Primary references:

- https://www.yorku.ca/mack/ec2017.pdf
- https://www.yorku.ca/mack/mhci2013h.html
- https://hci.cs.umanitoba.ca/publications/details/tilt-techniques-investigating-the-dexterity-of-wrist-based-input
- https://www.yorku.ca/mack/ie2014.html

## Candidate F1 result: touch direction good, roll strafe rejected

Essential navigation moves back to touch. A single anchored drag supplies two
simultaneous Doom intentions:

```text
vertical drag = forward/back       horizontal drag = turn
```

Releasing the finger guarantees both movement and turning stop. Device roll is
demoted to optional strafing: roll and hold to step laterally, then return near
the calibrated grip to stop. Ignoring motion entirely leaves the essential game
controls intact.

| Input | Action | Current response |
|---|---|---|
| Vertical touch hold | Forward/back | Anchor-relative; 10px dead zone; fixed normal Doom walk speed |
| Horizontal touch hold | Turn | Simultaneous with movement; 8px dead zone; reduced quadratic response; full turn rate at 96px |
| Device roll position | Strafe left/right | Calibrated Y/Z gravity plane; start near 6 degrees after two tics; stop inside roughly 3 degrees; normal Doom strafe speed |
| Touch double-tap | Use/Open | Each tap <=260ms and <=12px movement; taps <=340ms and <=52px apart |
| PWR tap | Fire once | Immediate PMIC press-edge pulse; release/hold ignored by gameplay |

Menus deliberately retain the established swipe navigation and PWR selection
behavior. The hybrid model is only active in a running level.

## Technical boundaries

- Build switch: `DOOM_HYBRID_CONTROLS`.
- Sensor: QMI8658 accelerometer only, +/-2g, 62.5Hz, low-pass enabled.
- Sampling: one six-byte XYZ accelerometer burst per Doom tic on the existing
  core-0 input path.
- Calibration: require 18 consecutive stable tics whose samples remain within
  a bounded total span, preventing a changing grip from becoming neutral.
- Roll signal: signed cross/dot comparison in the calibrated Y/Z gravity plane,
  so the angle thresholds do not assume a flat grip or exactly 1g projection.
- Filtering: retain the sensor's hardware low pass and require two consecutive
  start samples; remove the delayed software low pass and stop directly.
- Hysteresis: strafe starts around 6 degrees and stops inside roughly 3 degrees.
- Output: simultaneous bounded `forwardmove`, `sidemove`, and `angleturn` at the
  `ticcmd_t` boundary.
- Static memory delta: 72 bytes; full-width zone headroom remains 224,464 bytes.
- Fallback: compile without the switch to recover the proven floating
  swipe-and-hold model.

## F1 physical test

Keep the first test short and isolate one question at a time:

1. Enter E1M1 and hold the device comfortably for the first second.
2. Drag upward, downward, left, and right. Release after each and confirm an
   immediate stop.
3. Hold a diagonal drag and confirm forward/back and turning work together.
4. Check whether the reduced turn response makes aiming easier.
5. Roll left, return to neutral, then roll right. Confirm the physical direction,
   whether 6 degrees feels deliberate, and whether neutral stops dependably.
6. Fight one nearby enemy using touch navigation first; add roll strafing only
   if it helps rather than distracts.

The first physical test established the important result: simultaneous touch
navigation feels better. However, turning still accelerated too quickly on the
small display, and roll strafing repeatedly moved left/right while no finger was
down. That no-touch movement can only come from the `sidemove` motion path; it
is not touch jitter. Roll is therefore disabled rather than tuned in parallel
with the promising touch model.

## Candidate F2: touch-only tuning baseline

The next build isolates touch completely:

| Input | Action | Current response |
|---|---|---|
| Vertical touch hold | Forward/back | Unchanged 10px dead zone and normal Doom walk speed |
| Horizontal touch hold | Turn | 10px dead zone; quadratic response; full scale at 120px; maximum reduced from 960 to vanilla normal turn rate 640 |
| Motion sensors | None | IMU is not initialised or polled |
| Touch double-tap | Use/Open | Unchanged |
| PWR tap | Fire once | Unchanged |

`DOOM_ROLL_STRAFE` preserves F1 as an opt-in engineering experiment, but it is
off by default and must remain off until touch movement and turning are locked.
The F2 full-width build ends at `0x20049308`, leaving 224,504 zone bytes.

The short test should judge only touch: forward/back direction, immediate stop
on release, combined diagonal move-plus-turn, and whether turning is now slow
enough for precise aiming without becoming tedious.

## Candidate F2 result: easiest play so far, but too much screen travel

Alexander could play more easily than with any previous mapping and was able to
progress with materially better control. The remaining interaction cost was
physical rather than directional: reaching useful turn rates required moving a
finger across too much of the display, obscuring the game view.

## Candidate F3: compact floating control in the bottom-left

The touch-down point is already a floating neutral anchor, so no fixed on-screen
widget is required. The player can deliberately land in the bottom-left and
keep the finger there. F3 changes only the horizontal transfer function:

- useful turning now fits inside 56px instead of 120px;
- the dead zone is 8px and the initial turn rate is 64;
- a cubic curve stays gentle near neutral, then reaches the unchanged maximum
  normal Doom turn rate of 640 near the compact edge; and
- vertical forward/back, release-to-stop, double-tap Use, PWR fire, and the
  completely disabled IMU remain unchanged.

The entire screen still accepts the floating control for recovery and the
double-tap action. This test asks the player to choose the bottom-left as the
preferred grip; software restriction or a visible control region should be
considered only after the compact response feels correct.

## Candidate F3 result: compact enough, but short combat swipes under-respond

F3 remained playable and Alexander completed E1M1, confirming forward/back and
the overall touch-only direction. In E1M2 combat, however, short repeated
left/right swipes often produced too little turning. The cubic curve spent too
much of the compact range near its minimum output; this was response-curve
calibration, not a need to change forward movement or reintroduce motion.

## Candidate F4: stronger compact turning

Keep the accepted 56px control range and 640 maximum. Reduce the horizontal
dead zone from 8 to 6px, raise minimum turn output from 64 to 80, and use a
quadratic curve. This makes small and medium combat swipes register notably
sooner while retaining the same maximum bound. Every other input remains
identical to F3.

## Candidate F5: immediate progressive movement

The F4 physical test preferred the stronger left/right response, but revealed
that forward/back was fundamentally different: it waited for 10px and then
switched directly to one fixed 25-unit speed. F5 leaves F4 turning untouched
and changes only vertical displacement:

- movement begins just beyond a 4px jitter guard;
- the first output is a gentle 8 movement units;
- output rises linearly with thumb displacement;
- full 50-unit Doom run speed is reached at 44px; and
- releasing the screen still clears both axes immediately.

Diagonal displacement remains valid. It combines forward/back with turning at
the tic-command boundary; it does not reduce either axis or enable strafing.
Removing it would force the player to stop moving whenever they need to steer,
so any future diagonal suppression must be justified by a physical test rather
than assumed to improve purity.

## Candidate F6: near-zero dead zones

F5 confirmed that progressive forward/back is directionally correct, but both
axes still felt delayed around the anchor. F6 reduces both dead zones to a
one-pixel jitter guard. Output begins on the next reported coordinate step.

To keep first activation gentle, vertical minimum output falls from 8 to 4 and
horizontal minimum output from 80 to 48. The established full-scale distances,
curves, maximum speeds, diagonal composition, and release-to-stop behaviour do
not change. This should preserve familiar mid/high response while replacing
the silent centre plus abrupt jump with a much more continuous start.

## Candidate F7: active coherent touch reporting

F6 still required far more physical travel than a one-pixel software guard can
explain. F7 therefore holds every gameplay mapping constant and repairs the
shared FT3168 driver:

- Point mode selects continuous Active tracking instead of Monitor wake mode.
- Finger count and first-touch X/Y are read together from `0x02..0x06`.
- Each Doom tic consumes one coherent controller snapshot rather than four
  separate I2C transactions.
- Gesture mode retains Monitor mode for its intended low-power behaviour.

The physical test is intentionally smaller than a gameplay run. Touch down,
move left/right and up/down by roughly 1–2mm, and judge whether Doom responds
without harder pressure or a long swipe. Only after coordinate delivery is
trusted should the relative mapping be compared with an absolute eight-way
bottom-left D-pad.

## Candidate F8: pointing-finger precision

F7 makes subtle motion available with a smaller pointing-finger contact, but
the old compact ranges turn that precision into excessive speed. F8 preserves
the driver repair, one-pixel guards, initial outputs, response curves, maximum
outputs, diagonals, and release-to-stop. It changes only where maximum is
reached:

- forward/back full scale doubles from 44px to 88px; and
- turn full scale doubles from 56px to 112px.

The first few millimetres should therefore remain immediate but produce gentle
movement and rotation. Full run and normal maximum turn are still available at
the outer edge of the preferred bottom-left quadrant. Test with the pointing
finger first. Thumb suitability remains a separate control-model question.

## Candidate F9: faster outer turn

F8's small pointing-finger turns feel appropriately precise, but large turns
are too laborious. F9 changes only horizontal maximum output from 640 to 960.
The one-pixel guard, 48-unit initial output, quadratic curve, and 112px range
remain unchanged, so the extra response concentrates toward the outer edge.

Forward/back remains exactly F8. The test should compare small enemy-tracking
adjustments against deliberate 90/180-degree turns; success requires the latter
to become easier without making the former twitchy.

The physical test accepted this pointing-finger direction. Touch is now the
primary navigation baseline rather than an unresolved prerequisite for motion.

## Candidate F10: deliberate touch-gated roll strafe

F10 keeps every F9 touch value unchanged and adds Mario-Kart-like device roll
as a secondary combat assist. It deliberately avoids the failure mode of F1:

| Input/state | Response |
|---|---|
| Finger released | Strafe is always zero; stable grip samples update neutral |
| Finger held near neutral | Touch move/turn works exactly as F9; no strafe |
| Roll past about 10 degrees | After two tics, begin strafe at 8 units |
| Continue toward about 22 degrees | Scale strafe progressively up to 32 units |
| Return inside about 5 degrees | Stop strafe |
| Release while tilted | Stop immediately; no latch |

The accelerometer is the correct sensor for this held-angle mapping because it
measures the direction of gravity. The gyro reports rotation rate and would
require integration and drift correction without improving this interaction.

### F10 physical test

1. Enter E1M1, leave the finger off the screen, and hold the device in the
   comfortable play grip for about half a second so neutral can settle.
2. Touch and hold with the pointing finger, then deliberately roll left and
   right. Confirm the Doom player strafes in the same physical direction.
3. Confirm a modest roll gives a gentle sidestep and a larger roll gives a
   stronger one.
4. Return near the original grip and confirm strafe stops without hunting.
5. While still rolled, release the touchscreen and confirm movement stops
   immediately.
6. Hold the device tilted with no touch and confirm it never moves the player.
7. Briefly combine forward/turn touch input with roll during one fight; keep
   the test short and judge whether strafe helps rather than distracts.

The physical test confirmed that the mechanism works, but rejected the
interaction. Roll did nothing until touch was held, contrary to expectation;
more seriously, touching after changing grip could instantly enable strafe
against an older reference. Removing touch gating would reintroduce continuous
accidental movement. F10 remains available as an engineering build but is not
the active UX candidate.

## Candidate F11: corner dodge bursts and a slower movement ramp

F11 removes sensor ambiguity and reserves two explicit touch regions:

| Input | Action |
|---|---|
| Double-tap bottom-left 96x72px corner | Strafe left at 32 units for 6 tics, about 170ms |
| Double-tap bottom-right 96x72px corner | Strafe right at 32 units for 6 tics, about 170ms |
| Repeat either double tap | Repeat the short dodge; no persistent state |
| Double-tap elsewhere | Use/Open, unchanged |

The corner action is deliberately a burst rather than a hold. A single-touch
panel cannot provide a second continuous control while the pointing finger is
already navigating; a short completed gesture lets the player dodge, release,
and immediately resume aiming. It also cannot drift or activate because of
device posture.

Forward/back keeps immediate one-pixel activation and the same 4-to-50 output
bounds, but now follows a quadratic curve over 140px instead of a linear curve
over 88px. This makes small and middle displacements substantially calmer and
reserves maximum run for deliberate far travel. Horizontal F9 turning is
unchanged.

The first test should verify corner direction and whether one burst is enough
to dodge without overshooting, then compare slow, middle, and far forward
travel. Also confirm that a double tap near a door outside the corners still
activates Use/Open.

## Candidate F12: short PWR hold for Escape/menu

F12 keeps F11 touch controls and immediate tap-fire. A PWR press fires exactly
one shot immediately, then a continuous 450ms hold emits one Escape/menu pulse.
This differs from the rejected C1 experiment in three important ways:

- it uses the AXP2101 press and release edges plus a software timer rather than
  waiting for the PMIC long-press IRQ;
- fire remains a single one-tic pulse, so holding cannot produce a burst; and
- once Escape is sent, all PWR events are swallowed until physical release, so
  the completed short-press event cannot select an item in the newly opened
  menu.

The AXP2101 datasheet defines its configurable long-press IRQ at 1–2.5 seconds
and physical power-off threshold at 4–10 seconds. The 450ms software action is
therefore deliberately earlier and does not modify PMIC registers. Firmware
still cannot cancel the PMIC's physical power authority: continuing to hold
for several seconds may power off or restart the board.

Hardware acceptance is intentionally narrow: tap must still fire immediately;
hold should open the menu once at about half a second; release must not activate
a menu item; and another press after release must work normally. Release as soon
as the menu appears during the first test.

The in-level hardware test passed, but revealed the unavoidable ambiguity:
committing Fire on press means every hold fires before becoming Escape. F12.1
therefore resolves a tap only on release and extends the same hold recogniser
to menus and title/intermission screens:

| Context | PWR tap | PWR hold |
|---|---|---|
| Running level | Fire once on release | Escape/open menu at 450ms; release ignored |
| Menu already open | Enter/Select on release | Escape/back at 450ms; release ignored |

No action is posted on press. When a hold wins, it clears any pending click
decision before suppressing release. The old double-click path is bypassed in
the hybrid model because hold now owns Back; the fallback model is unchanged.

The physical test found F12.1 very comfortable for one-shot Fire and Escape in
both contexts. Very fast click-click Fire could collapse into one shot because
the first pulse's key-up and the second pulse's key-down could enter Doom's
event queue in the same tic; `gamekeydown[key_fire]` therefore never sampled a
released state between them.

F12.2 adds a four-entry fixed pulse queue. The first tap remains immediate on
release. If another tap arrives while the previous pulse is being released, it
waits through one complete low tic and is emitted on the following tic. This
supports up to 17.5 distinct trigger pulses per second—faster than Doom's weapon
animations—without heap allocation or changing the 450ms hold. A hold flushes
the queue before Escape so no stale shot can follow the menu action.

The physical F12.2 test still produced only one shot for Alexander's fastest
click-click, although a slightly paced pair produced two. This rules out the
virtual-key pulse boundary as the only loss point. The AXP2101 exposes PWR
events as read-write-one-to-clear flags rather than an event counter, and one
poll can contain both the release of an active tap and the press of the next.
F12.2 treated that combined mask as one completed tap and discarded the new
press.

F12.3 preserves that ordering. If a press was already active and the next PMIC
read contains both release and press, it completes the known first tap and
re-arms the hold recogniser for the second press. The later release can then
emit the second shot through the existing pulse queue. A combined press and
release with no earlier active press remains one fast tap, avoiding a ghost
held state. Two complete press/release cycles that both begin and end between
polls are still indistinguishable because the PMIC flags record occurrence,
not multiplicity.

The physical F12.3 test produced no practical improvement at the pistol. Source
inspection explains the middle-speed loss: the port emits a one-tic Fire pulse,
while Doom checks for the next pistol shot only at `A_ReFire` after its
4+6+4-tic attack sequence. A pulse occurring during that recovery is not
buffered. Slow taps work when the pistol is ready again; very fast complete
cycles can additionally remain indistinguishable at the PMIC. Do not change
weapon cadence or add delayed-shot buffering merely to make the pistol appear
faster.

## Candidate F13: fixed eight-way bottom-left D-pad

F13 preserves the accepted pointing-finger mapping as the default and adds
`DOOM_TOUCH_DPAD` as a compile-time comparison. It replaces only in-level
floating movement/turning; PWR tap/hold and menu swipes are unchanged.

The invisible pad occupies logical `[0,160)x[208,368)`, centred at `(80,288)`.
A 12px central radius is neutral. Eight directional sectors map up/down to
forward/back, left/right to turn, and corners to simultaneous movement plus
turn. Displacement below 56px uses normal 25-unit movement and 640-unit turn;
the outer ring uses 50-unit run and 960-unit fast turn. Touch-down activates
immediately from absolute position, and lifting guarantees zero movement.

This is intentionally a fair interaction comparison rather than a replacement:
test whether the fixed centre is easy to find without an overlay, whether the
12px neutral area prevents drift without feeling delayed, whether diagonals are
predictable, and whether the outer speed step helps combat or feels abrupt.
The pad owns its full square, so repeated direction taps cannot accidentally
become double-tap gestures. Double-tap Use and the bottom-right strafe remain
available outside it; the overlapping bottom-left strafe is deliberately absent
from this first D-pad comparison.

### F13 hardware result and candidate F14: asymmetric thumb zones

F13 is directionally successful. Its absolute touch-and-hold interaction feels
closer to keyboard-controlled Doom, gives Alexander more control, and remains
usable with a broad thumb. The fixed model therefore earns further investment;
the radial centre, dead zone, and speed ring do not yet.

F14 preserves F13 behind its existing switch and adds
`DOOM_TOUCH_DPAD_THUMB_ZONES`. The active logical landscape area follows
Alexander's sketch and is deliberately asymmetric:

| Zone | Bounds | Command | Rationale |
|---|---|---|---|
| LEFT | `x=0..23`, `y=70..303` | turn left at 640 | the physical left edge makes a narrow target discoverable |
| UP | `x=24..135`, `y=70..303` | move forward at 25 | approximately one broad fingertip wide |
| RIGHT | `x=136..339`, `y=70..303` | turn right at 640 | large combat-aiming target |
| DOWN | `x=0..339`, final 20 image rows plus bottom border | move back at 25 | the physical bottom edge makes a shallow target discoverable at every 448-wide height |

There are no dead zones between these boxes. Touch-down commands the absolute
zone immediately, moving a held thumb changes zone, and release is the only
neutral action. The first comparison deliberately removes F13's inner/outer
speed step so geometry can be assessed without a simultaneous speed change.

Diagonals remain valuable because moving forward while turning is basic Doom
combat. F14 does not devote large ambiguous corner zones to them. Instead, a
12px band centred on each vertical boundary commands UP+LEFT or UP+RIGHT. The
active overlay highlights both constituent boxes. Backward diagonals are
omitted because backing up is less frequent and the bottom edge should remain
an unambiguous stop-and-retreat target.

`DOOM_TOUCH_DPAD_OVERLAY` is a temporary calibration aid. It draws dim one-pixel
box outlines and a two-pixel amber active outline directly into the existing
packed display tile after scaling. It performs no transparency blend, adds no
framebuffer, and leaves the accepted builds unchanged when disabled. Its small
CPU cost must still pass a physical sound/pacing check. Keep it enabled through
extended testing; after the mapping is locked, decide between a polished
permanent treatment and disabling it for normal play.

### F14 hardware result and F14.1 in-zone Use

F14 is the strongest physical control result so far. All asymmetric cardinal
zones worked with Alexander's thumb, release remained a dependable stop, both
forward-turn bands worked adequately, and the outlines materially improved
discoverability. Keep the overlay enabled for extended testing instead of
hiding it merely because it began as a diagnostic.

The initial F14 deliberately made its control area gesture-exclusive, inherited
from F13. That prevented a door double tap wherever the thumb naturally rested.
F14.1 removes that restriction only for the asymmetric layout. The existing tap
qualification remains unchanged: each contact must last at most 260ms, move at
most 12px, occur within 340ms of the other, and land within 52px of it. Any such
pair whose first contact begins inside the F14 area resolves to Use/Open rather
than corner strafe.

Movement still activates immediately on both touch-downs. Deferring the first
movement command until the double-tap window expired would add up to 340ms of
latency to every normal hold, which would damage the accepted control feel.
F14.1 therefore accepts two tiny directional pulses during a door gesture; the
second release stops movement and emits Use. Hardware testing must establish
that this compromise is unobtrusive.

### F15: accepted BOOT next-weapon control

Weapon selection now has a dedicated physical action rather than another
overlapping touch gesture. One short BOOT press/release during a local single-
player level cycles forward through Doom's existing selectable owned-weapon
order. The action occurs only after release and does nothing in menus or network
play. BOOT hold and double-click are deliberately undefined.

Because BOOT shares flash CS, F15 followed a three-stage hardware admission:
isolated single-core probe, silent two-core Doom, then normal effects-enabled
Doom. All passed. The accepted implementation uses the SDK flash-safe
coordinator, an SRAM callback, and SRAM-only active DMA sources. This makes
weapon switching a closed essential-control decision while retaining F14.1 as
the exact rollback.

### F16: accepted BOOT-hold strafe modifier

The remaining weak action was symmetric, sustained strafing. F11's corner
double-tap bursts remain available, but they require a repeated gesture during
combat and do not combine naturally with the accepted thumb zones. F15.1 tests
the classic Doom modifier model instead:

| Input | Normal | While BOOT is held |
|---|---|---|
| LEFT zone | Turn left | Strafe left at 32 units |
| RIGHT zone | Turn right | Strafe right at 32 units |
| UP / DOWN | Forward / backward | Unchanged |
| UP-LEFT / UP-RIGHT band | Forward plus turn | Forward plus strafe |
| Release BOOT | — | Restore turning immediately after debounce |

A short release before the hold threshold still selects next weapon. Crossing
the hold threshold suppresses weapon selection on release. The threshold is
250 ms after F15's debounced pressed state—about 300 ms from the physical
press—so it is intentional without feeling like a menu long-press. Touch used
during modifier mode is excluded from double-tap Use recognition to avoid an
action firing when the player finishes a strafe.

The overlay continues to highlight the touched zone rather than adding another
visual mode. The bounded hardware test passed: short BOOT still changed weapon,
held BOOT produced left/right strafing, release restored ordinary turning, and
Alexander reported that the interaction worked great. Accept the candidate as
F16. F15 remains byte-identical when the hold option is disabled.

## Later experiments

- A/B test F17 fixed-zone menus against an explicitly menu-only swipe mode;
  keep gameplay and intermission routing unchanged.
- Add an explicit recenter gesture only if a later motion control earns a place.
- Compare a velocity-style horizontal swipe if anchor-relative turning feels
  tiring or requires repeated re-anchoring.
- If F15.1 passes, tune only its hold threshold or strafe speed after longer
  combat; do not add previous-weapon or another BOOT gesture without evidence.
- Avoid tilt or gyro aiming unless touch demonstrably fails after tuning; the
  research and hardware tests both argue against making motion primary.
