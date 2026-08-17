# TODO

## Controls

- [x] Research established one-finger directional controls for small touchscreens.
- [x] Document the current fixed hold-zone model.
- [x] Implement swipe-direction-and-hold movement anywhere on screen.
- [ ] Hardware-test and tune swipe dead zone/axis bias.
- [ ] If swipe-and-hold is unsatisfactory, redesign the fixed touch-zone sizes and placement.
- [x] Map BOOT long-press to Escape using bounded cross-core flash lockout.
- [x] Replace BOOT long-press with a debounced single press that emits Escape once per hold.
- [x] Abandon and remove all runtime BOOT polling after both BOOT-enabled tests behaved abnormally.
- [ ] Design an Escape/menu gesture using PWR or touch without reading BOOT at runtime.
- [ ] Later, if still useful, isolate BOOT-as-input in a single-core audio-disabled test using the SDK's `flash_safe_execute()` API; not a current priority.
- [x] Preserve the floating swipe-and-hold controls as documented Control Model A.
- [ ] Introduce a selectable control-model boundary without changing Model A's behavior.
- [ ] Prototype QMI8658 tilt controls with neutral calibration, dead zone, hysteresis, fixed-point filtering, and proportional output.
- [ ] Compare full-tilt and hybrid tilt-plus-touch models for combat accuracy, screen readability, fatigue, and accidental input.
- [ ] Provide an explicit in-game recenter action for every motion-control model.

## Stability and performance

- [ ] Inspect the unpowered board for speaker/amplifier, regulator, USB-power, or other visible heat damage before any further deployment.
- [x] Confirm that the pre-BOOT-polling effects-only firmware boots and operates normally again.
- [ ] Confirm extended-play stability with the current 234,776-byte effects-only zone.
- [x] Add a persistent exact-allocation OOM diagnostic.
- [x] Hardware-test centered pixel-exact 320x200 video during active combat.
- [ ] If 320x200 still freezes, add persistent core/render/game-tic heartbeat stages.
- [x] Make SFX output DMA/IRQ-driven and non-blocking, then re-enable sound effects.
- [x] Hardware-test menu/gameplay SFX and immediate frame pacing in the effects-only build.
- [x] Implement a fixed-memory MUSX music backend and compatible named-lump playback path.
- [x] Hardware-test music playback and confirm the MUSX/WAD/backend path works.
- [ ] Experiment with higher-quality optional music synthesis before reconsidering the default.
- [ ] Establish a repeatable baseline for zone headroom, stack high-water marks, frame/tic timing, display waits, and audio queue pressure.
- [ ] After instrumentation isolates the remaining freeze, perform a phased refactor of display, input, audio, and diagnostics behind unchanged interfaces.
- [ ] Audit linker-map/static buffers and remove measured duplication or dead code one change at a time, recording SRAM and frame-time deltas.

## Completed

- [x] Filter touch boundary chatter and remove blocking per-transition logs.
- [x] Implement both aspect-correct 448x280 scaling and native 320x200 presentation.
- [x] Replace the 128KB rotated framebuffer with a tiled transpose buffer.
- [x] Clear noisy letterbox panel RAM.
- [x] Expand the RP2350 short-pointer zone by 64KB.
- [x] Disable on-screen boot diagnostics after graphics init and clear their panel remnants.
