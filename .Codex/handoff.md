# Handoff

## Status

The main firmware is back on the confirmed-safe effects-only baseline with no
runtime BOOT access. It builds to the exact same UF2 as pre-BOOT revision
`b6c35ff`. README has been publication-cleaned for the next GitHub push.

## Done this session

- Added and hardware-tested asynchronous sound effects; retained optional music but defaulted it off.
- Added a BOOT/Escape experiment, observed abnormal hardware behavior twice, and removed it completely.
- Confirmed the safe pre-BOOT firmware runs normally and rebuilt it byte-for-byte.
- Researched official BOOTSEL input support and deferred further investigation.
- Planned selectable tilt/hybrid control models and an instrumentation-first performance refactor.

## Carry-forward

- Runtime BOOT input is forbidden in the playable build; BOOT is for ROM BOOTSEL entry only.
- Escape/menu still needs a PWR or touch gesture.
- The sustained-combat freeze remains unresolved and is not ordinary zone OOM.
- README changes remain reserved for major milestones or final pre-push cleanup.
- Local commits must not be pushed unless Alexander explicitly requests it.

## Next actions

1. Confirm the safe UF2 is restored and normal on hardware.
2. Design an Escape/menu action using PWR or touch.
3. Add persistent core/render/game-tic heartbeat diagnostics if the combat freeze persists.

## Risks/blockers

- BOOT-enabled builds correlated with abnormal audio/odor; exact cause remains unproven.
- The board should be disconnected immediately if heat, odor, or abnormal continuous sound recurs.

## Start-next-session prompt

Continue from the safe effects-only firmware with no runtime BOOT polling.
Confirm hardware status, then choose a PWR/touch Escape gesture or instrument
the remaining combat freeze.
