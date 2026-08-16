//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2008 David Flater
// Copyright(C) 2021-2022 Graham Sanderson
// Copyright(C) 2026 Alexander (ES8311/PIO adaptation)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	System interface for sound.

#ifndef __I_PICO_SOUND__
#define __I_PICO_SOUND__

#include "pico.h"

// Opaque outside this file. Upstream's version is pico_audio_i2s's
// producer-pool buffer type; ours is our own flat mono sample buffer (see
// i_picosound.c) - nothing outside i_picosound.c touches its fields, since
// we don't vendor opl_pico.c (music is stubbed for now, see i_oplmusic.c).
typedef struct audio_buffer audio_buffer_t;

#define PICO_SOUND_SAMPLE_FREQ 44100

#ifndef NUM_SOUND_CHANNELS
#define NUM_SOUND_CHANNELS 8
#endif

void I_PicoSoundSetMusicGenerator(void (*generator)(audio_buffer_t *buffer));
bool I_PicoSoundIsInitialized(void);
void I_PicoSoundFade(bool in);
bool I_PicoSoundFading(void);
#endif
