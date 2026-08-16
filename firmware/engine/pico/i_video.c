//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2021-2022 Graham Sanderson
// Copyright(C) 2026 Alexander (AMOLED presentation layer)
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
//  DOOM graphics stuff, adapted for the Waveshare RP2350-Touch-AMOLED-1.8.
//
//  Upstream (kilograham/rp2040-doom) drives a continuously-scanned DVI/VGA
//  signal: a scanvideo IRQ pulls one scanline at a time out of frame_buffer[]
//  on demand, forever, in real time. Our AMOLED is a static panel we push
//  full frames to over QSPI on our own schedule - there's no display to keep
//  fed, so that whole pull/IRQ/timing apparatus is gone.
//
//  What's kept, verbatim in spirit: frame_buffer[2][], the palette build,
//  the scanline_func_*/palette_convert_scanline pixel composition, and the
//  draw_vpatch overlay compositor (status bar/HUD/menu sprites drawn onto
//  scanlines during gameplay) - none of it actually touches scanvideo, it
//  just writes RGB565 into a plain pointer. What's new: present_frame_to_amoled()
//  walks all 200 scanlines through that same pixeline once per completed
//  frame and blits the result in one QSPI DMA transfer, and core1() drops
//  the DVI IRQ setup but keeps pd_core1_loop() (core1's half of the frame
//  render handshake - unrelated to display output) and the semaphore
//  handshake with pd_render.cpp.
//
//  See doom/docs/DECISIONS.md for the full writeup.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <doom/r_data.h>
#include "doom/f_wipe.h"
#include "pico.h"

#include "config.h"
#include "d_loop.h"
#include "deh_str.h"
#include "doomtype.h"
#include "i_input.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_diskicon.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#if PICO_ON_DEVICE
#include "bootlog.h"
#endif

#include "pico/multicore.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "picodoom.h"
#include "image_decoder.h"

#include "DEV_Config.h"
#include "qspi_pio.h"
#include "AMOLED_1in8.h"
#if PICO_RP2350
#include "hardware/structs/accessctrl.h"
#endif

static const patch_t *stbar;

volatile uint8_t interp_in_use; // referenced by pd_render.cpp; we never set it, so it's always "not in use"

// display has been set up?
static boolean initialized = false;

boolean screenvisible = true;

boolean screensaver_mode = false;

isb_int8_t usegamma = 0;

// Joystick/gamepad hysteresis
unsigned int joywait = 0;

pixel_t *I_VideoBuffer; // unused by DOOM_TINY rendering; kept only because some shared code declares it extern

// i_system.c's on-device "exit key" debug console (a fake DOS-prompt easter
// egg, type DOOM/CLS/exit) and D_Endoom (ENDOOM credits screen, disabled via
// NO_USE_ENDDOOM=1) both reference these unconditionally regardless of that
// define. Neither is reachable in normal play; keep them linkable but inert
// rather than pull back in the SUPPORT_TEXT machinery upstream used to
// actually render this (scanvideo-specific, and not worth it for a debug
// easter egg).
static uint8_t text_screen_data_backing[80 * 25 * 2];
uint8_t *text_screen_data = text_screen_data_backing;
void D_Endoom(void) {}

uint8_t __aligned(4) frame_buffer[2][SCREENWIDTH*MAIN_VIEWHEIGHT];
static uint16_t palette[256];
static uint16_t shared_pal[NUM_SHARED_PALETTES][16];
static int8_t next_pal=-1;

semaphore_t render_frame_ready, display_frame_freed;
semaphore_t core1_launch;

//
// I_SetPalette / I_FinishUpdate: presentation is driven from core1's loop
// (see core1() below), not from a per-call hook - matches upstream, where
// I_FinishUpdate is also an empty stub for this platform.
//
void I_SetPaletteNum(int doompalette)
{
    next_pal = doompalette;
}

void I_FinishUpdate (void)
{
}

void I_ShutdownGraphics(void)
{
}

void I_StartFrame (void)
{
}

void I_SetWindowTitle(const char *title)
{
}

uint8_t display_frame_index;
uint8_t display_overlay_index;
uint8_t display_video_type;

typedef void (*scanline_func)(uint32_t *dest, int scanline);

static void scanline_func_none(uint32_t *dest, int scanline);
static void scanline_func_double(uint32_t *dest, int scanline);
static void scanline_func_single(uint32_t *dest, int scanline);
static void scanline_func_wipe(uint32_t *dest, int scanline);

scanline_func scanline_funcs[] = {
        scanline_func_none,     // VIDEO_TYPE_NONE
        NULL,                   // VIDEO_TYPE_TEXT (unused - NO_USE_ENDDOOM)
        scanline_func_single,   // VIDEO_TYPE_SAVING
        scanline_func_double,   // VIDEO_TYPE_DOUBLE
        scanline_func_single,   // VIDEO_TYPE_SINGLE
        scanline_func_wipe,     // VIDEO_TYPE_WIPE
};

uint8_t *wipe_yoffsets;
int16_t *wipe_yoffsets_raw;
uint32_t *wipe_linelookup;
uint8_t next_video_type;
uint8_t next_frame_index;
uint8_t next_overlay_index;
#if !DEMO1_ONLY
uint8_t *next_video_scroll;
uint8_t *video_scroll;
#endif
volatile uint8_t wipe_min;

// Own RGB565 packer, matching GUI_Paint.h's RED=0xF800/GREEN=0x07E0/BLUE=0x001F
// convention exactly (standard RGB565), instead of pico-extras'
// PICO_SCANVIDEO_PIXEL_FROM_RGB8 (which we don't want as a dependency at all -
// we don't link pico_scanvideo_dpi or pico_audio_i2s from this file, which is
// exactly the errata-triggering RP2350 dependency the original two platform
// files hit; see DECISIONS.md).
#define RGB565_FROM_RGB8(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

static inline void palette_convert_scanline(uint32_t *dest, const uint8_t *src) {
    for (int i = 0; i < SCREENWIDTH; i += 2) {
        uint32_t val = palette[*src++];
        val |= (palette[*src++]) << 16;
        *dest++ = val;
    }
}

static void scanline_func_none(uint32_t *dest, int scanline) {
    memset(dest, 0, SCREENWIDTH * 2);
}

static void scanline_func_double(uint32_t *dest, int scanline) {
    if (scanline < MAIN_VIEWHEIGHT) {
        const uint8_t *src = frame_buffer[display_frame_index] + scanline * SCREENWIDTH;
        palette_convert_scanline(dest, src);
    } else {
        // we expect everything to be overdrawn by statusbar so we do nothing
    }
}

static void scanline_func_single(uint32_t *dest, int scanline) {
    uint8_t *src;
    if (scanline < MAIN_VIEWHEIGHT) {
        src = frame_buffer[display_frame_index] + scanline * SCREENWIDTH;
    } else {
        src = frame_buffer[display_frame_index^1] + (scanline - 32) * SCREENWIDTH;
    }
#if !DEMO1_ONLY
    if (video_scroll) {
        for(int i=SCREENWIDTH-1;i>0;i--) {
            src[i] = src[i-1];
        }
        src[0] = video_scroll[scanline];
    }
#endif
    palette_convert_scanline(dest, src);
}

static void scanline_func_wipe(uint32_t *dest, int scanline) {
    const uint8_t *src;
    if (scanline < MAIN_VIEWHEIGHT) {
        src = frame_buffer[display_frame_index];
    } else {
        src = frame_buffer[display_frame_index^1] - 32 * SCREENWIDTH;
    }
    assert(wipe_yoffsets && wipe_linelookup);
    uint16_t *d = (uint16_t *)dest;
    src += scanline * SCREENWIDTH;
    for (int i = 0; i < SCREENWIDTH; i++) {
        int rel = scanline - wipe_yoffsets[i];
        if (rel < 0) {
            d[i] = palette[src[i]];
        } else {
            const uint8_t *flip = (const uint8_t *)wipe_linelookup[rel];
            if (flip >= &frame_buffer[0][0] && flip < &frame_buffer[0][0] + 2 * SCREENWIDTH * MAIN_VIEWHEIGHT) {
                d[i] = palette[flip[i]];
            }
        }
    }
}

// draw_vpatch: composites a "virtual patch" (status bar digits/faces, HUD
// icons, menu text, intermission graphics - anything drawn as a sprite
// rather than baked into frame_buffer) onto one scanline. Copied verbatim
// from upstream except the stbar XIP-streaming-DMA fast path is dropped:
// it hardcoded DMA channel 11, which risks colliding with the channels our
// own qspi_pio/audio_pio drivers claim, for a speedup that matters for a
// continuously-scanned DVI signal but not for our once-per-frame blit.
static inline uint draw_vpatch(uint16_t *dest, patch_t *patch, vpatchlist_t *vp, uint off) {
    int repeat = vp->entry.repeat;
    dest += vp->entry.x;
    int w = vpatch_width(patch);
    const uint8_t *data0 = vpatch_data(patch);
    const uint8_t *data = data0 + off;
    if (!vpatch_has_shared_palette(patch)) {
        const uint8_t *pal = vpatch_palette(patch);
        switch (vpatch_type(patch)) {
            case vp4_runs: {
                uint16_t *p = dest;
                uint16_t *pend = dest + w;
                uint8_t gap;
                while (0xff != (gap = *data++)) {
                    p += gap;
                    int len = *data++;
                    for (int i = 1; i < len; i += 2) {
                        uint v = *data++;
                        *p++ = palette[pal[v & 0xf]];
                        *p++ = palette[pal[v >> 4]];
                    }
                    if (len & 1) {
                        *p++ = palette[pal[(*data++) & 0xf]];
                    }
                    assert(p <= pend);
                    if (p == pend) break;
                }
                break;
            }
            case vp4_alpha: {
                uint16_t *p = dest;
                for (int i = 0; i < w / 2; i++) {
                    uint v = *data++;
                    if (v & 0xf) p[0] = palette[pal[v & 0xf]];
                    if (v >> 4) p[1] = palette[pal[v >> 4]];
                    p += 2;
                }
                if (w & 1) {
                    uint v = *data++;
                    if (v & 0xf) p[0] = palette[pal[v & 0xf]];
                }
                break;
            }
            case vp4_solid: {
                uint16_t *p = dest;
                for (int i = 0; i < w / 2; i++) {
                    uint v = *data++;
                    p[0] = palette[pal[v & 0xf]];
                    p[1] = palette[pal[v >> 4]];
                    p += 2;
                }
                if (w & 1) {
                    uint v = *data++;
                    p[0] = palette[pal[v & 0xf]];
                }
                break;
            }
            case vp6_runs: {
                uint16_t *p = dest;
                uint16_t *pend = dest + w;
                uint8_t gap;
                while (0xff != (gap = *data++)) {
                    p += gap;
                    int len = *data++;
                    for (int i = 3; i < len; i += 4) {
                        uint v = *data++;
                        v |= (*data++) << 8;
                        v |= (*data++) << 16;
                        *p++ = palette[pal[v & 0x3f]];
                        *p++ = palette[pal[(v >> 6) & 0x3f]];
                        *p++ = palette[pal[(v >> 12) & 0x3f]];
                        *p++ = palette[pal[(v >> 18) & 0x3f]];
                    }
                    len &= 3;
                    if (len--) {
                        uint v = *data++;
                        *p++ = palette[pal[v & 0x3f]];
                        if (len--) {
                            v >>= 6;
                            v |= (*data++) << 2;
                            *p++ = palette[pal[v & 0x3f]];
                            if (len--) {
                                v >>= 6;
                                v |= (*data++) << 4;
                                *p++ = palette[pal[v & 0x3f]];
                                assert(!len);
                            }
                        }
                    }
                    assert(p <= pend);
                    if (p == pend) break;
                }
                break;
            }
            case vp8_runs: {
                uint16_t *p = dest;
                uint16_t *pend = dest + w;
                uint8_t gap;
                while (0xff != (gap = *data++)) {
                    p += gap;
                    int len = *data++;
                    for (int i = 0; i < len; i++) {
                        *p++ = palette[pal[*data++]];
                    }
                    assert(p <= pend);
                    if (p == pend) break;
                }
                break;
            }
            case vp_border: {
                dest[0] = palette[*data++];
                uint16_t col = palette[*data++];
                for (int i = 1; i < w - 1; i++) dest[i] = col;
                dest[w-1] = palette[*data++];
                break;
            }
            default:
                assert(false);
                break;
        }
    } else {
        uint sp = vpatch_shared_palette(patch);
        uint16_t *pal16 = shared_pal[sp];
        assert(sp < NUM_SHARED_PALETTES);
        switch (vpatch_type(patch)) {
            case vp4_solid: {
                if (((uintptr_t)dest)&3) {
                    uint16_t *p = dest;
                    for (int i = 0; i < w / 2; i++) {
                        uint v = *data++;
                        p[0] = pal16[v & 0xf];
                        p[1] = pal16[v >> 4];
                        p += 2;
                    }
                } else {
                    uint32_t *wide = (uint32_t *) dest;
                    for (int i = 0; i < w / 2; i++) {
                        uint v = *data++;
                        wide[i] = pal16[v & 0xf] | (pal16[v >> 4] << 16);
                    }
                }
                if (w & 1) {
                    uint v = *data++;
                    dest[w-1] = pal16[v & 0xf];
                }
                break;
            }
            case vp4_alpha: {
                uint16_t *p = dest;
                for (int i = 0; i < w / 2; i++) {
                    uint v = *data++;
                    if (v & 0xf) p[0] = pal16[v & 0xf];
                    if (v >> 4) p[1] = pal16[v >> 4];
                    p += 2;
                }
                if (w & 1) {
                    uint v = *data++;
                    if (v & 0xf) p[0] = pal16[v & 0xf];
                }
                break;
            }
            default:
                assert(false);
        }
    }
    if (repeat) {
        if (vp->entry.patch_handle == VPATCH_M_THERMM) w--; // hackity hack
        for(int i=0;i<repeat*w;i++) {
            dest[w+i] = dest[i];
        }
    }
    return data - data0;
}

// Re-initializes overlay draw lists / rebuilds the palette (once per frame,
// only when it actually changed) / advances the wipe transition state.
// Copied verbatim from upstream, minus the DOOM_TINY-vs-scanvideo interop
// that isn't relevant here; RGB8->RGB565 now goes through our own macro.
static void new_frame_init_overlays_palette_and_wipe(void) {
    if (display_video_type >= FIRST_VIDEO_TYPE_WITH_OVERLAYS) {
        memset(vpatchlists->vpatch_next, 0, sizeof(vpatchlists->vpatch_next));
        memset(vpatchlists->vpatch_starters, 0, sizeof(vpatchlists->vpatch_starters));
        memset(vpatchlists->vpatch_doff, 0, sizeof(vpatchlists->vpatch_doff));
        vpatchlist_t *overlays = vpatchlists->overlays[display_overlay_index];
        for (int i = overlays->header.size - 1; i > 0; i--) {
            assert(overlays[i].entry.y < count_of(vpatchlists->vpatch_starters));
            vpatchlists->vpatch_next[i] = vpatchlists->vpatch_starters[overlays[i].entry.y];
            vpatchlists->vpatch_starters[overlays[i].entry.y] = i;
        }
        if (next_pal != -1) {
            static const uint8_t *playpal;
            static bool calculate_palettes;
            if (!playpal) {
                lumpindex_t l = W_GetNumForName("PLAYPAL");
                playpal = W_CacheLumpNum(l, PU_STATIC);
                calculate_palettes = W_LumpLength(l) == 768;
            }
            if (!calculate_palettes || !next_pal) {
                const uint8_t *doompalette = playpal + next_pal * 768;
                for (int i = 0; i < 256; i++) {
                    int r = *doompalette++;
                    int g = *doompalette++;
                    int b = *doompalette++;
                    if (usegamma) {
                        r = gammatable[usegamma-1][r];
                        g = gammatable[usegamma-1][g];
                        b = gammatable[usegamma-1][b];
                    }
                    palette[i] = RGB565_FROM_RGB8(r, g, b);
                }
            } else {
                int mul, r0, g0, b0;
                if (next_pal < 9) {
                    mul = next_pal * 65536 / 9;
                    r0 = 255; g0 = b0 = 0;
                } else if (next_pal < 13) {
                    mul = (next_pal - 8) * 65536 / 8;
                    r0 = 215; g0 = 186; b0 = 69;
                } else {
                    mul = 65536 / 8;
                    r0 = b0 = 0; g0 = 256;
                }
                const uint8_t *doompalette = playpal;
                for (int i = 0; i < 256; i++) {
                    int r = *doompalette++;
                    int g = *doompalette++;
                    int b = *doompalette++;
                    r += ((r0 - r) * mul) >> 16;
                    g += ((g0 - g) * mul) >> 16;
                    b += ((b0 - b) * mul) >> 16;
                    palette[i] = RGB565_FROM_RGB8(r, g, b);
                }
            }
            next_pal = -1;
            assert(vpatch_type(stbar) == vp4_solid); // no transparent, no runs, 4 bpp
            for (int i = 0; i < NUM_SHARED_PALETTES; i++) {
                patch_t *patch = resolve_vpatch_handle(vpatch_for_shared_palette[i]);
                assert(vpatch_colorcount(patch) <= 16);
                assert(vpatch_has_shared_palette(patch));
                for (int j = 0; j < 16; j++) {
                    shared_pal[i][j] = palette[vpatch_palette(patch)[j]];
                }
            }
        }
        if (display_video_type == VIDEO_TYPE_WIPE) {
            if (wipe_min <= 200) {
                bool regular = display_overlay_index;
                int new_wipe_min = 200;
                for (int i = 0; i < SCREENWIDTH; i++) {
                    int v;
                    if (wipe_yoffsets_raw[i] < 0) {
                        if (regular) {
                            wipe_yoffsets_raw[i]++;
                        }
                        v = 0;
                    } else {
                        int dy = (wipe_yoffsets_raw[i] < 16) ? (1 + wipe_yoffsets_raw[i] + regular) / 2 : 4;
                        if (wipe_yoffsets_raw[i] + dy > 200) {
                            v = 200;
                        } else {
                            wipe_yoffsets_raw[i] += dy;
                            v = wipe_yoffsets_raw[i];
                        }
                    }
                    wipe_yoffsets[i] = v;
                    if (v < new_wipe_min) new_wipe_min = v;
                }
                assert(new_wipe_min >= wipe_min);
                wipe_min = new_wipe_min;
            }
        }
    }
}

// Pulls the next completed frame's identity from pd_render.cpp (core0's
// render code) and refreshes palette/overlay/wipe state for it. Unlike
// upstream (which only updates when a new frame happens to be ready, since
// the DVI signal must show *something* every refresh regardless), we
// block: our loop only ever wants to present a freshly rendered frame.
static void new_frame_stuff(void) {
    sem_acquire_blocking(&render_frame_ready);
    display_video_type = next_video_type;
    display_frame_index = next_frame_index;
    display_overlay_index = next_overlay_index;
#if !DEMO1_ONLY
    video_scroll = next_video_scroll;
#endif
    sem_release(&display_frame_freed);
    if (display_video_type != VIDEO_TYPE_SAVING) {
        new_frame_init_overlays_palette_and_wipe();
    }
}

// --- AMOLED presentation -------------------------------------------------
//
// Doom renders at 320x200; our panel is 368x448, driven landscape-rotated
// (see doom/firmware/lib/display) - so the game view is 448x368 logical
// space with the 320x200 image centered, matching the layout already
// worked out for the control-calibration firmware.
//
// GUI_Paint's ROTATE_90 case (Paint_SetPixel, Scale==65) maps logical
// (Xpoint,Ypoint) to a physical pixel index of
// physical_idx = (PANEL_WIDTH - Ypoint - 1) + Xpoint * PANEL_WIDTH,
// stored big-endian (high byte first) in a PANEL_WIDTH-stride buffer. We
// don't call Paint_SetPixel per pixel (64000 calls/frame is wasteful) -
// this reimplements that exact placement as a direct, byte-swapped store
// so a scanline's worth of pixels can be scattered in one tight loop.

#define DISPLAY_WIDTH       448
#define DISPLAY_HEIGHT      280
#define DOOM_VIEW_X_OFFSET  ((AMOLED_1IN8_HEIGHT - DISPLAY_WIDTH) / 2) // 0
#define DOOM_VIEW_Y_OFFSET  ((AMOLED_1IN8_WIDTH - DISPLAY_HEIGHT) / 2) // 44

// A full 368-wide physical strip (368x320, matching the Y-windowed DMA
// transfer's other dimension) would be 235KB, but the rotated Doom image
// only ever occupies a SCREENHEIGHT(200)-wide sub-band of that 368 (the
// rest is letterbox padding either side - DOOM_VIEW_Y_OFFSET on each edge).
// Sized to exactly that used band instead (320*200*2 = 128000 bytes, zero
// slack) - this is what actually fixed a RAM-budget regression from
// restoring pd_render.cpp's real (non-stub) static state: see the
// 2026-08-16 entry in doom/docs/DECISIONS.md for the __end__/SHORTPTR_BASE
// arithmetic that made this necessary, not just nice-to-have.
// "base" below is relative to this narrower buffer (no + DOOM_VIEW_Y_OFFSET)
// - that offset now lives in present_frame_to_amoled()'s DisplayWindowPacked
// Xstart/Xend instead. Presented via AMOLED_1IN8_DisplayWindowPacked()
// (single DMA transfer) rather than AMOLED_1IN8_DisplayWindows(), which
// proved intermittently unreliable on this hardware - see DECISIONS.md.
// Static, not malloc'd: Z_Init's zone claims everything from the linker's
// __end__ symbol up to a fixed address (SHORTPTR_BASE+0x40000, required by
// this engine's "short pointer" scheme - see i_system.c's AutoAllocMemory),
// leaving only a thin sliver of the C library heap above that for
// malloc(). A static array is accounted for in bss *before* __end__ is
// computed, so it doesn't compete with the zone for that sliver at all.
//
// NOTE: the letterbox padding area (outside this band, and outside
// bootlog's own window) is never explicitly cleared by anything - it's
// whatever was on the panel before. Cosmetic; fine for bring-up, revisit
// once something is actually rendering.
// Re-enabled (2026-08-16 cont'd): was temporarily disabled (panel_window's
// 128000 bytes freed for a bigger bootlog history) while chasing the
// zone-corruption bug. Root cause found and fixed (audio_pio.c's
// data_treating() was calloc()'ing into the same address range the zone
// manually claims from __end__ - see DECISIONS.md) - back to presenting
// real frames.
#define PRESENT_FRAME_TO_AMOLED_DISABLED 0
#define PANEL_CHUNK_ROWS 40
#if !PRESENT_FRAME_TO_AMOLED_DISABLED
static UWORD panel_chunk[PANEL_CHUNK_ROWS * DISPLAY_WIDTH];
#endif

static void clear_letterbox_bands(void)
{
#if !PRESENT_FRAME_TO_AMOLED_DISABLED
    // Logical Y:[0,44) and [324,368) become physical X strips after the
    // software ROTATE_90 mapping. Reuse the frame tile: 40+4 rows per band,
    // four reliable packed transfers, and no extra framebuffer allocation.
    memset(panel_chunk, 0, sizeof(panel_chunk));
    AMOLED_1IN8_DisplayWindowPacked(0, 0, 40, DISPLAY_WIDTH, panel_chunk);
    AMOLED_1IN8_DisplayWindowPacked(40, 0, 44, DISPLAY_WIDTH, panel_chunk);
    AMOLED_1IN8_DisplayWindowPacked(AMOLED_1IN8_WIDTH - 44, 0,
                                     AMOLED_1IN8_WIDTH - 4, DISPLAY_WIDTH,
                                     panel_chunk);
    AMOLED_1IN8_DisplayWindowPacked(AMOLED_1IN8_WIDTH - 4, 0,
                                     AMOLED_1IN8_WIDTH, DISPLAY_WIDTH,
                                     panel_chunk);
#endif
}

static void present_frame_to_amoled(void) {
#if !PRESENT_FRAME_TO_AMOLED_DISABLED
    uint32_t row_buf[SCREENWIDTH / 2]; // 2 pixels/word, matches scanline_func_*/draw_vpatch's dest convention

    for (int scanline = 0; scanline < SCREENHEIGHT; scanline++) {
        scanline_funcs[display_video_type](row_buf, scanline);
        if (display_video_type >= FIRST_VIDEO_TYPE_WITH_OVERLAYS) {
            assert(scanline < (int)count_of(vpatchlists->vpatch_starters));
            int prev = 0;
            for (int vp = vpatchlists->vpatch_starters[scanline]; vp;) {
                int next = vpatchlists->vpatch_next[vp];
                while (vpatchlists->vpatch_next[prev] && vpatchlists->vpatch_next[prev] < vp) {
                    prev = vpatchlists->vpatch_next[prev];
                }
                assert(prev != vp);
                assert(vpatchlists->vpatch_next[prev] != vp);
                vpatchlists->vpatch_next[vp] = vpatchlists->vpatch_next[prev];
                vpatchlists->vpatch_next[prev] = vp;
                prev = vp;
                vp = next;
            }
            vpatchlist_t *overlays = vpatchlists->overlays[display_overlay_index];
            prev = 0;
            for (int vp = vpatchlists->vpatch_next[prev]; vp; vp = vpatchlists->vpatch_next[prev]) {
                patch_t *patch = resolve_vpatch_handle(overlays[vp].entry.patch_handle);
                int yoff = scanline - overlays[vp].entry.y;
                if (yoff < vpatch_height(patch)) {
                    vpatchlists->vpatch_doff[vp] = draw_vpatch((uint16_t*)row_buf, patch, &overlays[vp],
                                                               vpatchlists->vpatch_doff[vp]);
                    prev = vp;
                } else {
                    vpatchlists->vpatch_next[prev] = vpatchlists->vpatch_next[vp];
                }
            }
        }
        uint16_t *pixels = (uint16_t *)row_buf;

        // Exact 7:5 nearest-neighbour scale: 320x200 -> 448x280. Each
        // source row produces one or two output rows; compose overlays only
        // once, then duplicate the completed RGB565 row as needed.
        int output_first = scanline * 7 / 5;
        int output_end = (scanline + 1) * 7 / 5;
        for (int output_y = output_first; output_y < output_end; output_y++) {
            int chunk_row = output_y % PANEL_CHUNK_ROWS;
            for (int output_x = 0; output_x < DISPLAY_WIDTH; output_x++) {
                int source_x = output_x * 5 / 7;
                panel_chunk[output_x * PANEL_CHUNK_ROWS
                            + (PANEL_CHUNK_ROWS - chunk_row - 1)] =
                    __builtin_bswap16(pixels[source_x]);
            }

            if (chunk_row == PANEL_CHUNK_ROWS - 1) {
                int first_row = output_y - chunk_row;
                int physical_x = DOOM_VIEW_Y_OFFSET
                               + DISPLAY_HEIGHT - first_row - PANEL_CHUNK_ROWS;
                AMOLED_1IN8_DisplayWindowPacked(physical_x,
                                                 DOOM_VIEW_X_OFFSET,
                                                 physical_x + PANEL_CHUNK_ROWS,
                                                 DOOM_VIEW_X_OFFSET + DISPLAY_WIDTH,
                                                 panel_chunk);
            }
        }
    }
#endif
}

// core1: keeps pd_core1_loop() (core1's half of the per-frame render
// handshake with core0 - flats/columns work when USE_CORE1_FOR_FLATS/
// USE_CORE1_FOR_REGULAR are enabled, otherwise just the core0_done/
// core1_done rendezvous, servicing audio via SafeUpdateSound() while it
// waits) completely unchanged. What's gone is scanvideo_setup()/the DVI
// IRQ - replaced with a direct call to present the frame once it's ready.
static void core1(void) {
    sem_release(&core1_launch);
    while (true) {
        pd_core1_loop();
#if PICO_ON_DEVICE
        // Bisecting core1's per-frame work (2026-08-16 cont'd): the zone
        // list breaks during the first D_RunFrame() tic on core0 even
        // though zero Z_Malloc calls happen in that window (confirmed:
        // zm#1 is the only ever call, and it's clean, from P_Init long
        // before the loop starts) - so something writes into zone
        // memory directly, not through the allocator API. core1 runs
        // concurrently every frame via this same loop; bracket its two
        // halves once each to see which (if either) breaks it first.
        // See DECISIONS.md.
        {
            static boolean printed;
            if (!printed) {
                printed = true;
                char buf[40];
                snprintf(buf, sizeof(buf), "c1a: after core1_loop free=%d", Z_FreeMemory());
                bootlog_print(buf);
            }
        }
#endif
        new_frame_stuff();
#if PICO_ON_DEVICE
        {
            static boolean printed2;
            if (!printed2) {
                printed2 = true;
                char buf[40];
                snprintf(buf, sizeof(buf), "c1b: after frame_stuff free=%d", Z_FreeMemory());
                bootlog_print(buf);
            }
        }
#endif
        present_frame_to_amoled();
    }
}

void I_InitGraphics(void)
{
    // dma_tx (used by AMOLED_1IN8_Display()/DisplayWindows()) is only ever
    // assigned inside DEV_Module_Init() - without this call it's an
    // uninitialized global defaulting to channel 0 (unclaimed/
    // unconfigured), and the DMA busy-wait inside the display driver can
    // hang forever. See doom/docs/DECISIONS.md.
    DEV_Module_Init();
    QSPI_GPIO_Init(qspi);
    QSPI_PIO_Init(qspi);
    QSPI_4Wrie_Mode(&qspi);
    AMOLED_1IN8_Init();
    AMOLED_1IN8_SetBrightness(100);
    // SH8601 MADCTL has no row/column exchange bit (unlike ST77xx); landscape
    // rotation is performed in small software-transposed tiles above.
    AMOLED_1IN8_SetMemoryAccessControl(0x00);
    clear_letterbox_bands();

    stbar = resolve_vpatch_handle(VPATCH_STBAR);
    sem_init(&render_frame_ready, 0, 2);
    sem_init(&display_frame_freed, 1, 2);
    sem_init(&core1_launch, 0, 1);
    pd_init();
    multicore_launch_core1(core1);
    // wait for core1 launch as it may do malloc and we have no mutex around that
    sem_acquire_blocking(&core1_launch);
#if USE_ZONE_FOR_MALLOC
    disallow_core1_malloc = true;
#endif
#if PICO_RP2350
    hw_set_bits(&accessctrl_hw->xip_ctrl, ACCESSCTRL_PASSWORD_BITS | 0xff);
#endif
    initialized = true;
}

void I_BindVideoVariables(void)
{
}

void I_StartTic (void)
{
    if (!initialized)
    {
        return;
    }
    I_GetEvent();
}

void I_UpdateNoBlit (void)
{
}

int I_GetPaletteIndex(int r, int g, int b)
{
    return 0;
}

void I_GraphicsCheckCommandLine(void)
{
}

void I_CheckIsScreensaver(void)
{
}

void I_DisplayFPSDots(boolean dots_on)
{
}
