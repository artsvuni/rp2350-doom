# Handheld Control Design

## Experience goal

The control system should make combat and exploration enjoyable on a 1.8-inch
device, not merely make every Doom command technically reachable. The player
must be able to stop, turn precisely, fire without delay, and open a door
without changing grip or covering much of the display.

## Candidate C1 result: direction good, motion model rejected

The first physical test confirmed that horizontal touch turning felt responsive
and enjoyable, and touchscreen double-tap reliably opened a door. It rejected
two other parts of the model:

- proportional absolute tilt required an uncomfortably large movement from
  Alexander's natural roughly 11-o'clock grip toward 9 o'clock before walking
  became useful; and
- held PWR fired several shots, then the PMIC long-press event opened the menu;
  a longer hold subsequently appeared to restart the game.

PWR is therefore a tap-only fire input. The software assigns no gameplay action
to release or long-press. Escape/menu remains deliberately unresolved.

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

## Later experiments

- Add an explicit recenter gesture only if a later motion control earns a place.
- Compare a velocity-style horizontal swipe if anchor-relative turning feels
  tiring or requires repeated re-anchoring.
- If continuous roll still causes accidental movement, test a deliberate
  one-shot weapon-switch gesture or remove motion control entirely.
- Avoid tilt or gyro aiming unless touch demonstrably fails after tuning; the
  research and hardware tests both argue against making motion primary.
