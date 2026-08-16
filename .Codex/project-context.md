# Project Context

## Summary

DOOM runs on the Waveshare RP2350-Touch-AMOLED-1.8 and is playable with touch movement plus the PWR button. The AMOLED output is aspect-correct at 448x280. The renderer uses a tiled software transpose because the SH8601 cannot rotate axes in hardware. Recent work recovered substantial SRAM and expanded Doom's zone to 227,944 bytes. Extended gameplay is improved but final memory stability is not yet proven.

## Active goals

- Make one-finger touch movement comfortable and efficient.
- Confirm extended-play memory stability and diagnose any remaining freeze.
- Eventually replace the blocking audio path and re-enable sound.

## Key decisions made

- Preserve Doom's 16:10 aspect ratio: scale 320x200 to 448x280 rather than stretching to 448x368.
- Use 40-row packed transpose tiles; SH8601 MADCTL has no row/column exchange.
- Place the RP2350 short-pointer window at `0x20040000..0x20080000`, ending at the linker stack limit.
- Keep BOOT input disabled until flash access can be synchronized across cores.

## Current state

- Game boots, renders, loads E1M1, and supports combat.
- Touch boundary chatter is filtered and no longer triggers blocking screen logs.
- Current movement uses four fixed hold zones and is difficult to operate by feel.
- A swipe-direction-and-hold control experiment is next.

## Open questions

- Does the 227,944-byte zone eliminate the eventual gameplay freeze?
- Which one-finger movement model feels best on this small display?
- How should BOOT/menu and weapon switching be safely integrated?
