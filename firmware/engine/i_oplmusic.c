/*
 * Music stub. Upstream's i_oplmusic.c drives a real OPL FM-synth emulator
 * (emu8950) through a PIO/I2S output path (opl/opl_pico.c) - deliberately
 * not vendored here. Getting the game rendering and controllable on the
 * AMOLED is this milestone's goal; music (and mixing it with sound
 * effects) is real, separable follow-up work, not a stub we'd accidentally
 * ship - see doom/docs/DECISIONS.md.
 *
 * This still needs to satisfy the music_module_t interface i_sound.c
 * expects so the rest of the engine links and runs unmodified.
 */
#include "doomtype.h"
#include "i_sound.h"

// pd_render.cpp (kept unmodified from upstream) reads/writes this directly -
// it's a bit flag protecting against restarting a song mid-render-call
// (could blow the stack). Harmless always-zero here since we never start
// a song to begin with.
uint8_t restart_song_state;

static const snddevice_t music_stub_devices[] = { SNDDEVICE_ADLIB };

static boolean I_OPLStub_InitMusic(void) { return false; }
static void I_OPLStub_ShutdownMusic(void) {}
static void I_OPLStub_SetMusicVolume(int volume) {}
static void I_OPLStub_PauseMusic(void) {}
static void I_OPLStub_ResumeMusic(void) {}
static void *I_OPLStub_RegisterSong(should_be_const void *data, int len) { return NULL; }
static void I_OPLStub_UnRegisterSong(void *handle) {}
static void I_OPLStub_PlaySong(void *handle, boolean looping) {}
static void I_OPLStub_StopSong(void) {}
static boolean I_OPLStub_MusicIsPlaying(void) { return false; }
static void I_OPLStub_Poll(void) {}

void I_SetOPLDriverVer(opl_driver_ver_t ver) {}

const music_module_t music_opl_module =
{
    music_stub_devices,
    arrlen(music_stub_devices),
    I_OPLStub_InitMusic,
    I_OPLStub_ShutdownMusic,
    I_OPLStub_SetMusicVolume,
    I_OPLStub_PauseMusic,
    I_OPLStub_ResumeMusic,
    I_OPLStub_RegisterSong,
    I_OPLStub_UnRegisterSong,
    I_OPLStub_PlaySong,
    I_OPLStub_StopSong,
    I_OPLStub_MusicIsPlaying,
    I_OPLStub_Poll,
};
