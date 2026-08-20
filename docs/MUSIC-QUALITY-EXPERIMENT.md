# Doom music quality experiment

## Goal

Find out whether Doom music can be pleasant on the RP2350-Touch-AMOLED-1.8,
not merely whether the firmware can emit the notes. Effects-only remains the
accepted pocket-play baseline. Music is optional and must earn its CPU time and
audible space without weakening the excellent sound effects.

## What the first experiment proved

The original optional backend successfully reads the real MUSX event streams
from the converted WAD and renders nine simultaneous voices through the same
44.1kHz mono DMA queue as the sound effects. Its full-panel build uses only
1,324 more static bytes and 6,536 more text bytes than effects-only F18, so the
bad listening result was not caused by a large memory burden.

The problem is the renderer itself. It reduces all General MIDI programs to
four raw oscillator families, resets phase abruptly on every note, ends notes
without release, and represents percussion as full-band noise. Square, saw,
and narrow-pulse discontinuities generate strong high harmonics. That is an
especially poor source for this board's 12x10mm, 8-ohm, 1W speaker and explains
the brittle chiptune character and clicks better than memory pressure does.

## Hardware implications

The board uses an ES8311 mono codec feeding an NS4150B class-D amplifier and the
small onboard speaker. The codec supports global DAC volume, EQ, and dynamic
range controls, but applying those first would also alter the already accepted
sound effects. The first candidate therefore leaves codec and amplifier setup
unchanged and masters only the generated music before SFX mixing.

The current audio queue is already the right transport: two fixed buffers,
non-blocking DMA/IRQ output, silence on underflow, and no music heap allocation.
Replacing that driver would add risk without addressing the source timbre.

## Options considered

### Volume-only tuning

Necessary but insufficient. Lower level can make an unpleasant source quieter;
it cannot remove waveform discontinuities or full-band noise.

### Global codec EQ or compressor

Deferred. It could protect the speaker, but it would also colour gunshots,
doors, pickups, and menu sounds that already work well.

### Speaker-mastered fixed-memory synthesis

Selected as the first reversible test. It retains the proven MUSX scheduler and
nine fixed voices, but adds:

- continuous sine-like, triangle, and restrained harmonic timbres instead of
  raw square/saw/pulse edges;
- approximately 5ms note attack and 62ms release to remove edge clicks;
- per-voice filtering of percussion noise;
- music-only approximately 90Hz high-pass and 7kHz low-pass filtering;
- a gentle music-only peak knee;
- Doom's proven audible starting menu level of 8/15;
- 2.5dB music ducking while any sound effect is active.

The mastered full-panel build has `__end__=0x2004a14c`, leaving approximately
218,804 zone bytes. It adds 48 static bytes over the old music build and 7,088
text bytes over effects-only F18. Its UF2 SHA-256 is
`2535e3600cdfbe9ac44577405ef276f1ef85a397fabd9e34a7b3e92a7ce51875`.

`DOOM_MUSIC_SPEAKER_MASTERING=OFF` reproduces the original music experiment
byte-for-byte at
`d3e3859aba25eb7dcf71d57aaee4ee663a423822ed3d268cf643673a22968d6b`.
`DOOM_ENABLE_MUSIC=OFF` reproduces accepted F18 at
`5290ec3a571113cc2bb5c701a1c299413d7b9743183c5ecdd4769c26e0e6ac3a`.

### Authentic OPL2

This is the next engineering route if the lighter candidate remains too
synthetic. Doom's original score was commonly rendered through AdLib/OPL, and
the upstream RP2040 Doom project demonstrates nine-channel OPL2 at playable
frame rates. A port must replace its dynamic `calloc` state with fixed static
storage before it is safe here: this project manually claims Doom's zone and
has already seen corruption from allocations outside that contract.

References:

- [Chocolate Doom music documentation](https://github.com/chocolate-doom/chocolate-doom/blob/master/README.Music.md)
- [Upstream RP2040 Doom OPL implementation](https://github.com/kilograham/rp2040-doom)
- [Waveshare board schematic](https://files.waveshare.com/wiki/RP2350-Touch-AMOLED-1.8/RP2350-Touch-AMOLED-1.8.pdf)
- [ES8311 datasheet](https://files.waveshare.com/wiki/common/ES8311.DS.pdf)

### Pre-rendered mastered audio

This offers the highest predictable fidelity and the 16MiB flash has substantial
unused capacity, but it introduces a new asset pipeline and can consume several
megabytes for a complete soundtrack even with mono ADPCM. It is the fallback if
authentic real-time synthesis is either too expensive or still sounds poor.
No copyrighted music data belongs in this repository; assets must be generated
from the user's legally obtained WAD.

## Physical listening gate

Do not run a long profiler first. Listen for roughly 30–60 seconds:

1. Let the title/menu music establish whether the timbre is immediately tiring.
2. Start E1M1 and listen while walking, firing, opening a door, and collecting
   an item.
3. Check that effects stay clear and in front of the score.
4. Check for clicks, crackle, stretched timing, obvious audio lag, or a visible
   gameplay slowdown.
5. Try music level 6, 7, and 8 only if the default balance is close.

Accept this candidate only if it is genuinely pleasant enough to choose over
effects-only. If the timbre remains obviously toy-like, stop tuning arbitrary
numbers and move to the fixed-memory OPL2 gate. If it sounds good but gameplay
feels slower, use the existing bounded flight recorder to quantify the cost.

## Hardware result

Rejected. The first mastered build was effectively inaudible. Restoring the
old 8/15 gain and relaxing the high-pass did not reveal a coherent score; in
gameplay the speaker instead emitted clearly distorted intermittent "puf"
bursts. The host also could not reach the busy application through its normal
USB reset interface immediately afterward, so no further music test is safe or
useful on this image.

The burst pattern is consistent with isolated generated blocks separated by
audio underflow, although that remains a diagnosis rather than a measured
counter. The mastered inner loop added per-voice envelope/harmonic work and two
per-sample filters to a queue that deliberately emits silence whenever the
producer misses a refill. Regardless of whether starvation or the synthesized
signal is the immediate cause, this design fails the audible acceptance gate.

Do not continue gain/filter tuning. Restore exact effects-only F18 and treat
the lightweight mastered backend as rejected evidence. The next music attempt,
if pursued, is a separately gated fixed-memory OPL2 port with audio refill cost
instrumented before physical listening.
