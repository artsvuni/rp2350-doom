# TODO

## Controls

- [x] Research established one-finger directional controls for small touchscreens.
- [x] Document the current fixed hold-zone model.
- [x] Implement swipe-direction-and-hold movement anywhere on screen.
- [ ] Hardware-test and tune swipe dead zone/axis bias.
- [ ] If swipe-and-hold is unsatisfactory, redesign the fixed touch-zone sizes and placement.
- [ ] Resolve BOOT long-press/menu mapping without unsafe cross-core flash access.

## Stability and performance

- [ ] Confirm extended-play stability with the 227,944-byte zone.
- [x] Add a persistent exact-allocation OOM diagnostic.
- [ ] If the next freeze is not OOM, instrument thinker/block lists and multicore rendezvous.
- [ ] Reassess scaled-display lag after removing per-pixel division.
- [ ] Hardware-test driver stall recovery and true no-audio performance.
- [ ] Make audio output non-blocking and re-enable sound.

## Completed

- [x] Filter touch boundary chatter and remove blocking per-transition logs.
- [x] Scale video to aspect-correct 448x280 output.
- [x] Replace the 128KB rotated framebuffer with a tiled transpose buffer.
- [x] Clear noisy letterbox panel RAM.
- [x] Expand the RP2350 short-pointer zone by 64KB.
