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
#include <doom/doomstat.h>
#include "doom/f_wipe.h"
#include "pico.h"

#include "config.h"
#include "d_loop.h"
#include "deh_str.h"
#include "doomtype.h"
#include "i_input.h"
#include "i_joystick.h"
#include "i_sound.h"
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
#if DOOM_BOOT_NEXT_WEAPON
#include "pico/flash.h"
#endif
#include "hardware/gpio.h"
#if DOOM_ENABLE_PROFILING
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "pico/platform/sections.h"
#endif
#include "picodoom.h"
#include "image_decoder.h"

#include "DEV_Config.h"
#include "qspi_pio.h"
#include "AMOLED_1in8.h"
#if DOOM_TOUCH_DPAD_OVERLAY
#include "touch_dpad.h"
#endif
#if PICO_RP2350
#include "hardware/structs/accessctrl.h"
#endif

static const patch_t *stbar;

volatile uint8_t interp_in_use; // referenced by pd_render.cpp; we never set it, so it's always "not in use"

// display has been set up?
static boolean initialized = false;

#if DOOM_BOOT_NEXT_WEAPON
static volatile boolean boot_input_flash_safe_ready = false;

boolean I_BootInputFlashSafeReady(void)
{
    return boot_input_flash_safe_ready;
}
#endif

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
// Doom always renders one 320x200 indexed frame. This boundary composes the
// palette and overlays once per source row, optionally scales it, transposes it
// into portrait-addressed tiles, and sends tightly packed RGB565 over QSPI.
// Keeping output size here means the renderer and its memory-heavy tables stay
// unchanged while output sizes up to the panel's native 448x368 can be
// compared. Wider-than-16:10 modes deliberately expand the vertical scale;
// they do not increase the renderer's 320x200 memory footprint.

#define DISPLAY_WIDTH       DOOM_DISPLAY_WIDTH
#define DISPLAY_HEIGHT      DOOM_DISPLAY_HEIGHT
#define DOOM_VIEW_X_OFFSET  ((AMOLED_1IN8_HEIGHT - DISPLAY_WIDTH) / 2)
#define DOOM_VIEW_Y_OFFSET  ((AMOLED_1IN8_WIDTH - DISPLAY_HEIGHT) / 2)

static_assert(DISPLAY_WIDTH >= SCREENWIDTH, "AMOLED output cannot downscale");
static_assert(DISPLAY_HEIGHT >= SCREENHEIGHT, "AMOLED output cannot downscale");
static_assert(DISPLAY_WIDTH <= AMOLED_1IN8_HEIGHT, "AMOLED output too wide");
static_assert(DISPLAY_HEIGHT <= AMOLED_1IN8_WIDTH, "AMOLED output too tall");

// Forty rows is the hardware-proven default. Smaller experimental tiles can
// shorten the strided writes and recover SRAM, but the first 8-row hardware
// build produced a black panel. Keep tile height selectable for controlled
// driver work without exposing that failed candidate as the default.
#define PANEL_CHUNK_ROWS DOOM_PANEL_CHUNK_ROWS
#define PANEL_FINAL_ROWS (DISPLAY_HEIGHT % PANEL_CHUNK_ROWS)
#define PANEL_FINAL_PADDING \
    ((PANEL_CHUNK_ROWS - PANEL_FINAL_ROWS) % PANEL_CHUNK_ROWS)
static_assert(PANEL_CHUNK_ROWS > 0, "transpose tile must contain rows");
static_assert(PANEL_FINAL_ROWS == 0 ||
              DOOM_VIEW_Y_OFFSET >= PANEL_FINAL_PADDING ||
              (DISPLAY_HEIGHT == AMOLED_1IN8_WIDTH &&
               PANEL_CHUNK_ROWS == 20 && DOOM_ASYNC_AMOLED),
              "partial tile needs border padding or async full-panel overlap");

#if DOOM_ASYNC_AMOLED
static_assert(PANEL_CHUNK_ROWS == 20,
              "memory-neutral async presentation requires 20-row buffers");
// Two 20-row buffers use exactly the same SRAM as the proven single 40-row
// tile. While DMA reads one, core1 packs the next.
static UWORD panel_chunks[2][PANEL_CHUNK_ROWS * DISPLAY_WIDTH];
#else
static UWORD panel_chunks[1][PANEL_CHUNK_ROWS * DISPLAY_WIDTH];
#endif

#if DOOM_TOUCH_DPAD_OVERLAY
#define DPAD_OVERLAY_DIM RGB565_FROM_RGB8(92, 92, 92)
#define DPAD_OVERLAY_ACTIVE RGB565_FROM_RGB8(224, 194, 64)

static inline void dpad_overlay_pixel(int pack_buffer, int output_y,
                                      int x, UWORD color)
{
    if (x < 0 || x >= DISPLAY_WIDTH) return;
    int chunk_row = output_y % PANEL_CHUNK_ROWS;
    panel_chunks[pack_buffer][x * PANEL_CHUNK_ROWS
                              + (PANEL_CHUNK_ROWS - chunk_row - 1)] =
        __builtin_bswap16(color);
}

static void dpad_overlay_rect_row(int pack_buffer, int output_y,
                                  int x0, int y0, int x1, int y1,
                                  int thickness, UWORD color)
{
    int y = DOOM_VIEW_Y_OFFSET + output_y;
    if (y < y0 || y >= y1) return;

    if (y < y0 + thickness || y >= y1 - thickness) {
        for (int x = x0; x < x1; x++) {
            dpad_overlay_pixel(pack_buffer, output_y, x, color);
        }
        return;
    }

    for (int edge = 0; edge < thickness; edge++) {
        dpad_overlay_pixel(pack_buffer, output_y, x0 + edge, color);
        dpad_overlay_pixel(pack_buffer, output_y, x1 - edge - 1, color);
    }
}

static bool dpad_zone_uses(touch_dpad_zone_t active,
                           touch_dpad_zone_t primary)
{
    if (active == primary) return true;
    if (active == TOUCH_DPAD_ZONE_UP_LEFT) {
        return primary == TOUCH_DPAD_ZONE_UP || primary == TOUCH_DPAD_ZONE_LEFT;
    }
    if (active == TOUCH_DPAD_ZONE_UP_RIGHT) {
        return primary == TOUCH_DPAD_ZONE_UP || primary == TOUCH_DPAD_ZONE_RIGHT;
    }
    return false;
}

static void apply_dpad_overlay_row(int pack_buffer, int output_y)
{
    touch_dpad_zone_t active = I_GetTouchDpadZone();
    const struct {
        int x0, y0, x1, y1;
        touch_dpad_zone_t zone;
    } boxes[] = {
        { 0, TOUCH_DPAD_Y_MIN, TOUCH_DPAD_LEFT_X, TOUCH_DPAD_DOWN_Y,
          TOUCH_DPAD_ZONE_LEFT },
        { TOUCH_DPAD_LEFT_X, TOUCH_DPAD_Y_MIN,
          TOUCH_DPAD_UP_X, TOUCH_DPAD_DOWN_Y, TOUCH_DPAD_ZONE_UP },
        { TOUCH_DPAD_UP_X, TOUCH_DPAD_Y_MIN,
          TOUCH_DPAD_X_MAX, TOUCH_DPAD_DOWN_Y, TOUCH_DPAD_ZONE_RIGHT },
        { 0, TOUCH_DPAD_DOWN_Y, TOUCH_DPAD_X_MAX, AMOLED_1IN8_WIDTH,
          TOUCH_DPAD_ZONE_DOWN },
    };

    for (unsigned i = 0; i < sizeof(boxes) / sizeof(boxes[0]); i++) {
        dpad_overlay_rect_row(pack_buffer, output_y,
                              boxes[i].x0, boxes[i].y0,
                              boxes[i].x1, boxes[i].y1,
                              1, DPAD_OVERLAY_DIM);
    }
    for (unsigned i = 0; i < sizeof(boxes) / sizeof(boxes[0]); i++) {
        if (dpad_zone_uses(active, boxes[i].zone)) {
            dpad_overlay_rect_row(pack_buffer, output_y,
                                  boxes[i].x0, boxes[i].y0,
                                  boxes[i].x1, boxes[i].y1,
                                  2, DPAD_OVERLAY_ACTIVE);
        }
    }
}
#endif

#if DOOM_ENABLE_PROFILING
#define PROFILE_WARMUP_US (3u * 1000u * 1000u)
#define PROFILE_CAPTURE_DURATION_US \
    (DOOM_PROFILE_CAPTURE_SECONDS * 1000u * 1000u)
#define PROFILE_REPORT_MAGIC 0x50455246u // "PERF"
#define PROFILE_FLASH_OFFSET ((uint32_t)TINY_WAD_ADDR - XIP_BASE - FLASH_SECTOR_SIZE)

static_assert((PROFILE_FLASH_OFFSET % FLASH_SECTOR_SIZE) == 0,
              "profile log must start on a flash erase-sector boundary");
static_assert(PROFILE_FLASH_OFFSET + FLASH_SECTOR_SIZE ==
              (uint32_t)TINY_WAD_ADDR - XIP_BASE,
              "profile log must occupy only the sector immediately before WHD");

static volatile uint32_t profile_game_frame_last_us;
static volatile uint32_t profile_game_frame_max_us;
static volatile uint32_t profile_render_last_us;
static volatile uint32_t profile_render_max_us;
static volatile uint32_t profile_display_wait_last_us;
static volatile uint32_t profile_display_wait_max_us;

void I_ProfileRecordGameFrame(uint32_t frame_us)
{
    profile_game_frame_last_us = frame_us;
    if (frame_us > profile_game_frame_max_us) profile_game_frame_max_us = frame_us;
}

void I_ProfileRecordRender(uint32_t render_us, uint32_t display_wait_us)
{
    profile_render_last_us = render_us;
    profile_display_wait_last_us = display_wait_us;
    if (render_us > profile_render_max_us) profile_render_max_us = render_us;
    if (display_wait_us > profile_display_wait_max_us) {
        profile_display_wait_max_us = display_wait_us;
    }
}

typedef struct {
    uint32_t samples;
    uint32_t capture_started_us;
    uint32_t cadence_samples;
    uint32_t cadence_sum_us;
    uint32_t cadence_max_us;
    uint32_t core1_sum_us;
    uint32_t core1_max_us;
    uint32_t frame_wait_sum_us;
    uint32_t frame_wait_max_us;
    uint32_t present_sum_us;
    uint32_t present_max_us;
    uint32_t transfer_sum_us;
    uint32_t transfer_max_us;
    uint32_t previous_frame_start_us;
} video_profile_t;

static video_profile_t video_profile;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    uint32_t duration_us;
    uint32_t cadence_avg_us;
    uint32_t cadence_max_us;
    uint32_t core1_avg_us;
    uint32_t core1_max_us;
    uint32_t frame_wait_avg_us;
    uint32_t frame_wait_max_us;
    uint32_t present_avg_us;
    uint32_t present_max_us;
    uint32_t prep_avg_us;
    uint32_t transfer_avg_us;
    uint32_t transfer_max_us;
    uint32_t game_last_us;
    uint32_t game_max_us;
    uint32_t render_last_us;
    uint32_t render_max_us;
    uint32_t display_wait_last_us;
    uint32_t display_wait_max_us;
    uint32_t dma_timeout_count;
    uint32_t checksum;
} profile_report_t;

// The linker deliberately does not clear this section at reset. The watchdog
// scratch magic is the authority for whether its contents are a real report.
static profile_report_t __uninitialized_ram(doom_profile_report);

static uint32_t profile_report_checksum(const profile_report_t *report)
{
    const uint32_t *words = (const uint32_t *)report;
    uint32_t checksum = 0x6d657472u;
    for (size_t i = 0; i < (sizeof(*report) / sizeof(*words)) - 1; i++) {
        checksum ^= words[i] + (uint32_t)(i * 0x9e3779b9u);
    }
    return checksum;
}

static void profile_persist_to_reserved_flash(void)
{
    // This is called only on the single-core report boot, before stdio, audio,
    // or Doom starts. Gameplay itself never erases/programs flash. The page is
    // in core0 SRAM so flash can be unavailable throughout the operation.
    uint8_t page[FLASH_PAGE_SIZE] __attribute__((aligned(4)));
    memset(page, 0xff, sizeof(page));
    memcpy(page, &doom_profile_report, sizeof(doom_profile_report));

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(PROFILE_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(PROFILE_FLASH_OFFSET, page, sizeof(page));
    restore_interrupts(interrupts);
}

static void profile_store_and_reboot(uint32_t samples, uint32_t duration_us)
{
    profile_report_t report = {
        .magic = PROFILE_REPORT_MAGIC,
        .version = 2,
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .samples = samples,
        .duration_us = duration_us,
        .cadence_avg_us = video_profile.cadence_sum_us /
                          video_profile.cadence_samples,
        .cadence_max_us = video_profile.cadence_max_us,
        .core1_avg_us = video_profile.core1_sum_us / samples,
        .core1_max_us = video_profile.core1_max_us,
        .frame_wait_avg_us = video_profile.frame_wait_sum_us / samples,
        .frame_wait_max_us = video_profile.frame_wait_max_us,
        .present_avg_us = video_profile.present_sum_us / samples,
        .present_max_us = video_profile.present_max_us,
        .prep_avg_us = (video_profile.present_sum_us -
                        video_profile.transfer_sum_us) / samples,
        .transfer_avg_us = video_profile.transfer_sum_us / samples,
        .transfer_max_us = video_profile.transfer_max_us,
        .game_last_us = profile_game_frame_last_us,
        .game_max_us = profile_game_frame_max_us,
        .render_last_us = profile_render_last_us,
        .render_max_us = profile_render_max_us,
        .display_wait_last_us = profile_display_wait_last_us,
        .display_wait_max_us = profile_display_wait_max_us,
        .dma_timeout_count = AMOLED_1IN8_GetDmaTimeoutCount(),
    };
    report.checksum = profile_report_checksum(&report);
    doom_profile_report = report;
    watchdog_hw->scratch[0] = PROFILE_REPORT_MAGIC;
    __dmb();
    watchdog_reboot(0, 0, 10);
    while (true) tight_loop_contents();
}

boolean I_ProfileBootReportPending(void)
{
    if (watchdog_hw->scratch[0] != PROFILE_REPORT_MAGIC) return false;
    watchdog_hw->scratch[0] = 0;
    boolean valid = doom_profile_report.magic == PROFILE_REPORT_MAGIC &&
                    doom_profile_report.version == 2 &&
                    doom_profile_report.checksum ==
                        profile_report_checksum(&doom_profile_report);
    if (valid) profile_persist_to_reserved_flash();
    return valid;
}

void I_ProfilePrintBootReport(void)
{
    const profile_report_t *r = &doom_profile_report;
    static unsigned screen_line;
    char line[40];

    printf("PERFLOG v=%lu mode=%lux%lu samples=%lu duration=%lu us\n",
           (unsigned long)r->version, (unsigned long)r->width,
           (unsigned long)r->height, (unsigned long)r->samples,
           (unsigned long)r->duration_us);
    printf("PERFLOG cadence avg=%lu max=%lu us\n",
           (unsigned long)r->cadence_avg_us,
           (unsigned long)r->cadence_max_us);
    printf("PERFLOG core1 avg=%lu max=%lu wait avg=%lu max=%lu us\n",
           (unsigned long)r->core1_avg_us, (unsigned long)r->core1_max_us,
           (unsigned long)r->frame_wait_avg_us,
           (unsigned long)r->frame_wait_max_us);
    printf("PERFLOG present avg=%lu max=%lu prep=%lu us\n",
           (unsigned long)r->present_avg_us,
           (unsigned long)r->present_max_us,
           (unsigned long)r->prep_avg_us);
    printf("PERFLOG transfer avg=%lu max=%lu us\n",
           (unsigned long)r->transfer_avg_us,
           (unsigned long)r->transfer_max_us);
    printf("PERFLOG game last=%lu max=%lu render last=%lu max=%lu us\n",
           (unsigned long)r->game_last_us, (unsigned long)r->game_max_us,
           (unsigned long)r->render_last_us,
           (unsigned long)r->render_max_us);
    printf("PERFLOG displaywait last=%lu max=%lu dma_to=%lu\n",
           (unsigned long)r->display_wait_last_us,
           (unsigned long)r->display_wait_max_us,
           (unsigned long)r->dma_timeout_count);

    switch (screen_line++ % 5u) {
        case 0:
            snprintf(line, sizeof(line), "PERF SAVED %lux%lu",
                     (unsigned long)r->width, (unsigned long)r->height);
            break;
        case 1:
            snprintf(line, sizeof(line), "cad %lu/%lu us",
                     (unsigned long)r->cadence_avg_us,
                     (unsigned long)r->cadence_max_us);
            break;
        case 2:
            snprintf(line, sizeof(line), "present %lu/%lu us",
                     (unsigned long)r->present_avg_us,
                     (unsigned long)r->present_max_us);
            break;
        case 3:
            snprintf(line, sizeof(line), "prep %lu xfer %lu us",
                     (unsigned long)r->prep_avg_us,
                     (unsigned long)r->transfer_avg_us);
            break;
        default:
            snprintf(line, sizeof(line), "render %lu/%lu us",
                     (unsigned long)r->render_last_us,
                     (unsigned long)r->render_max_us);
            break;
    }
    bootlog_print(line);
}

static inline void profile_accumulate(uint32_t value,
                                      uint32_t *sum, uint32_t *maximum)
{
    *sum += value;
    if (value > *maximum) *maximum = value;
}

static void profile_finish_frame(uint32_t frame_start_us,
                                 uint32_t core1_us,
                                 uint32_t frame_wait_us,
                                 uint32_t present_us,
                                 uint32_t transfer_us)
{
    static uint32_t level_started_us;

    // Do not measure menus, attract-mode demos, or level loading. The capture
    // starts automatically only when the player is controlling a real level.
    if (gamestate != GS_LEVEL || !usergame || demoplayback) {
        level_started_us = 0;
        memset(&video_profile, 0, sizeof(video_profile));
        profile_game_frame_last_us = 0;
        profile_game_frame_max_us = 0;
        profile_render_last_us = 0;
        profile_render_max_us = 0;
        profile_display_wait_last_us = 0;
        profile_display_wait_max_us = 0;
        return;
    }

    // Let level entry, first-frame asset work, and the player's initial grip
    // settle before starting the timed minute. This keeps one-off load spikes
    // from defining the maxima intended to represent real movement/combat.
    if (!level_started_us) level_started_us = frame_start_us;
    if (frame_start_us - level_started_us < PROFILE_WARMUP_US) {
        memset(&video_profile, 0, sizeof(video_profile));
        profile_game_frame_last_us = 0;
        profile_game_frame_max_us = 0;
        profile_render_last_us = 0;
        profile_render_max_us = 0;
        profile_display_wait_last_us = 0;
        profile_display_wait_max_us = 0;
        return;
    }

    if (!video_profile.capture_started_us) {
        video_profile.capture_started_us = frame_start_us;
    }

    if (video_profile.previous_frame_start_us) {
        profile_accumulate(frame_start_us - video_profile.previous_frame_start_us,
                           &video_profile.cadence_sum_us,
                           &video_profile.cadence_max_us);
        video_profile.cadence_samples++;
    }
    video_profile.previous_frame_start_us = frame_start_us;
    profile_accumulate(core1_us, &video_profile.core1_sum_us,
                       &video_profile.core1_max_us);
    profile_accumulate(frame_wait_us, &video_profile.frame_wait_sum_us,
                       &video_profile.frame_wait_max_us);
    profile_accumulate(present_us, &video_profile.present_sum_us,
                       &video_profile.present_max_us);
    profile_accumulate(transfer_us, &video_profile.transfer_sum_us,
                       &video_profile.transfer_max_us);
    video_profile.samples++;

    uint32_t duration_us = frame_start_us - video_profile.capture_started_us;
    if (duration_us < PROFILE_CAPTURE_DURATION_US) return;

    const uint32_t samples = video_profile.samples;
    profile_store_and_reboot(samples, duration_us);
}
#endif

static void clear_panel_background(void)
{
    // Clear all panel GRAM once so the larger previous build cannot remain
    // visible around the game window. Reuse the runtime tile and split the
    // portrait panel into the widest full-height stripes that fit. Always end
    // with a full-size transaction, overlapping the previous stripe when the
    // 368-pixel panel axis is not divisible by the stripe width. The controller
    // has already produced a black screen with an 8-row transaction; avoiding
    // the same short tail here also prevents stale GRAM at the panel edge.
    UWORD *panel_chunk = panel_chunks[0];
    memset(panel_chunk, 0, sizeof(panel_chunks[0]));
    const int stripe_width = (PANEL_CHUNK_ROWS * DISPLAY_WIDTH) / AMOLED_1IN8_HEIGHT;
    int x = 0;
    while (x < AMOLED_1IN8_WIDTH) {
        int xend = x + stripe_width;
        if (xend >= AMOLED_1IN8_WIDTH) {
            x = AMOLED_1IN8_WIDTH - stripe_width;
            xend = AMOLED_1IN8_WIDTH;
        }
        AMOLED_1IN8_DisplayWindowPacked(x, 0, xend, AMOLED_1IN8_HEIGHT,
                                         panel_chunk);
        if (xend == AMOLED_1IN8_WIDTH) break;
        x = xend;
    }
}

static inline int flush_completed_chunk(int output_y, int pack_buffer
#if DOOM_ASYNC_AMOLED
                                        , bool *transfer_pending
#endif
#if DOOM_ENABLE_PROFILING
                                         , uint32_t *transfer_us
#endif
)
{
    int chunk_row = output_y % PANEL_CHUNK_ROWS;
    if (chunk_row != PANEL_CHUNK_ROWS - 1) return pack_buffer;

    int first_row = output_y - chunk_row;
    int physical_x = DOOM_VIEW_Y_OFFSET
                   + DISPLAY_HEIGHT - first_row - PANEL_CHUNK_ROWS;
#if DOOM_ENABLE_PROFILING
    uint32_t started_us = time_us_32();
#endif
#if DOOM_ASYNC_AMOLED
    if (*transfer_pending) {
        AMOLED_1IN8_DisplayWindowPackedWait();
        I_UpdateSound();
    }
    AMOLED_1IN8_DisplayWindowPackedStart(physical_x,
                                          DOOM_VIEW_X_OFFSET,
                                          physical_x + PANEL_CHUNK_ROWS,
                                          DOOM_VIEW_X_OFFSET + DISPLAY_WIDTH,
                                          panel_chunks[pack_buffer]);
    *transfer_pending = true;
#else
    AMOLED_1IN8_DisplayWindowPacked(physical_x,
                                     DOOM_VIEW_X_OFFSET,
                                     physical_x + PANEL_CHUNK_ROWS,
                                     DOOM_VIEW_X_OFFSET + DISPLAY_WIDTH,
                                     panel_chunks[pack_buffer]);
#if DOOM_ENABLE_PROFILING
    *transfer_us += time_us_32() - started_us;
#endif

    // A 512-sample audio block lasts 11.6ms and the two-buffer queue covers
    // 23.2ms. Full-width presentation takes about 29ms, so relying only on
    // renderer/tic callbacks can underflow during simple menu frames and make
    // SFX sound stretched. The packed transfer has completed and released its
    // DMA mutex here; give the non-blocking mixer a refill opportunity between
    // the seven proven 40-row transfers. I_UpdateSound() uses a try-lock and
    // returns immediately when the queue is already full or no SFX is active.
    I_UpdateSound();
#endif

#if DOOM_ENABLE_PROFILING && DOOM_ASYNC_AMOLED
    // In the pipelined build this is deliberately host-blocking display time:
    // residual wait plus command/submission work. DMA time hidden behind CPU
    // packing is excluded, so preparation + transfer still explains cadence.
    *transfer_us += time_us_32() - started_us;
#endif

#if DOOM_ASYNC_AMOLED
    return pack_buffer ^ 1;
#else
    return pack_buffer;
#endif
}

#if DOOM_ENABLE_PROFILING
static uint32_t present_frame_to_amoled(uint32_t *transfer_us_out) {
#else
static void present_frame_to_amoled(void) {
#endif
    uint32_t row_buf[SCREENWIDTH / 2]; // 2 pixels/word, matches scanline_func_*/draw_vpatch's dest convention
#if DISPLAY_WIDTH != SCREENWIDTH
    // Only scaled builds pay for this temporary row. It lives on core1's
    // stack after pd_core1_loop() has returned, so it does not reduce Doom's
    // short-pointer zone or overlap the renderer's deepest call paths.
    uint16_t scaled_row[DISPLAY_WIDTH];
    int output_y = 0;
    int vertical_accumulator = SCREENHEIGHT - 1;
#endif
#if DOOM_ENABLE_PROFILING
    uint32_t present_started_us = time_us_32();
    uint32_t transfer_us = 0;
#endif
    int pack_buffer = 0;
#if DOOM_ASYNC_AMOLED
    bool transfer_pending = false;
#endif

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

#if DISPLAY_WIDTH == SCREENWIDTH
        int chunk_row = scanline % PANEL_CHUNK_ROWS;
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            panel_chunks[pack_buffer][x * PANEL_CHUNK_ROWS
                                      + (PANEL_CHUNK_ROWS - chunk_row - 1)] =
                __builtin_bswap16(pixels[x]);
        }
        pack_buffer = flush_completed_chunk(scanline, pack_buffer
#if DOOM_ASYNC_AMOLED
                                             , &transfer_pending
#endif
#if DOOM_ENABLE_PROFILING
                                             , &transfer_us
#endif
        );
#else
        // Reverse-map the horizontal nearest-neighbour scale by emitting each
        // source pixel once or twice. Initial SCREENWIDTH-1 bias exactly
        // matches floor(output_x * SCREENWIDTH / DISPLAY_WIDTH), while loading
        // and byte-swapping each source pixel only once.
        int output_x = 0;
        int horizontal_accumulator = SCREENWIDTH - 1;
        for (int source_x = 0; source_x < SCREENWIDTH; source_x++) {
            UWORD pixel = __builtin_bswap16(pixels[source_x]);
            horizontal_accumulator += DISPLAY_WIDTH;
            while (horizontal_accumulator >= SCREENWIDTH) {
                scaled_row[output_x++] = pixel;
                horizontal_accumulator -= SCREENWIDTH;
            }
        }
        assert(output_x == DISPLAY_WIDTH);

        vertical_accumulator += DISPLAY_HEIGHT;
        // For the current scaled modes, some source rows are emitted twice
        // (80 at 448x280 and 120 at 448x320). Pack both adjacent output rows in
        // one x loop whenever they stay in the same tile. This loads each
        // scaled pixel once and removes one complete 448-iteration strided
        // loop, without another row buffer or any image change. Fall back to
        // the single-row path when the first copy closes the current tile.
        if (vertical_accumulator >= SCREENHEIGHT * 2 &&
            (output_y % PANEL_CHUNK_ROWS) != PANEL_CHUNK_ROWS - 1) {
            int chunk_row = output_y % PANEL_CHUNK_ROWS;
            UWORD *first = &panel_chunks[pack_buffer]
                                         [PANEL_CHUNK_ROWS - chunk_row - 1];
            UWORD *second = first - 1;
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                UWORD pixel = scaled_row[x];
                *first = pixel;
                *second = pixel;
                first += PANEL_CHUNK_ROWS;
                second += PANEL_CHUNK_ROWS;
            }
#if DOOM_TOUCH_DPAD_OVERLAY
            apply_dpad_overlay_row(pack_buffer, output_y);
            apply_dpad_overlay_row(pack_buffer, output_y + 1);
#endif
            output_y += 2;
            vertical_accumulator -= SCREENHEIGHT * 2;
            pack_buffer = flush_completed_chunk(output_y - 1, pack_buffer
#if DOOM_ASYNC_AMOLED
                                                 , &transfer_pending
#endif
#if DOOM_ENABLE_PROFILING
                                                 , &transfer_us
#endif
            );
            continue;
        }
        while (vertical_accumulator >= SCREENHEIGHT) {
            int chunk_row = output_y % PANEL_CHUNK_ROWS;
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                panel_chunks[pack_buffer][x * PANEL_CHUNK_ROWS
                                          + (PANEL_CHUNK_ROWS - chunk_row - 1)] =
                    scaled_row[x];
            }
#if DOOM_TOUCH_DPAD_OVERLAY
            apply_dpad_overlay_row(pack_buffer, output_y);
#endif
            pack_buffer = flush_completed_chunk(output_y, pack_buffer
#if DOOM_ASYNC_AMOLED
                                                 , &transfer_pending
#endif
#if DOOM_ENABLE_PROFILING
                                                 , &transfer_us
#endif
            );
            output_y++;
            vertical_accumulator -= SCREENHEIGHT;
        }
#endif
    }
#if DISPLAY_WIDTH != SCREENWIDTH
    assert(output_y == DISPLAY_HEIGHT);
#endif
#if PANEL_FINAL_ROWS != 0
#if DISPLAY_HEIGHT == AMOLED_1IN8_WIDTH
    // Native-height 448x368 has no border in which to hide 12 padding rows,
    // while short panel transactions are not hardware-proven. The preceding
    // completed buffer still holds output rows 340..359. Once its DMA finishes,
    // combine its last 12 rows with the current buffer's rows 360..367 and
    // resend rows 348..367 as one known-good 20-row transaction at panel x=0.
    // This overlaps 12 already-presented rows but requires no third tile buffer.
    static_assert(DOOM_ASYNC_AMOLED && PANEL_CHUNK_ROWS == 20 &&
                  PANEL_FINAL_ROWS == 8 && PANEL_FINAL_PADDING == 12,
                  "full-panel tail assembly is the bounded 448x368/20 case");
#if DOOM_ENABLE_PROFILING
    uint32_t tail_wait_started_us = time_us_32();
#endif
    assert(transfer_pending);
    AMOLED_1IN8_DisplayWindowPackedWait();
#if DOOM_ENABLE_PROFILING
    transfer_us += time_us_32() - tail_wait_started_us;
#endif
    I_UpdateSound();
    transfer_pending = false;

    int previous_buffer = pack_buffer ^ 1;
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        UWORD *current = &panel_chunks[pack_buffer][x * PANEL_CHUNK_ROWS];
        const UWORD *previous =
            &panel_chunks[previous_buffer][x * PANEL_CHUNK_ROWS];

        // Move output rows 360..367 from indices 19..12 to 7..0.
        for (int row = 0; row < PANEL_FINAL_ROWS; row++) {
            current[PANEL_FINAL_ROWS - row - 1] =
                current[PANEL_CHUNK_ROWS - row - 1];
        }
        // Copy output rows 348..359 from previous indices 11..0 to 19..8.
        for (int row = 0; row < PANEL_FINAL_PADDING; row++) {
            current[PANEL_CHUNK_ROWS - row - 1] =
                previous[PANEL_FINAL_PADDING - row - 1];
        }
    }

#if DOOM_ENABLE_PROFILING
    uint32_t tail_submit_started_us = time_us_32();
#endif
    AMOLED_1IN8_DisplayWindowPackedStart(
        0, DOOM_VIEW_X_OFFSET,
        PANEL_CHUNK_ROWS, DOOM_VIEW_X_OFFSET + DISPLAY_WIDTH,
        panel_chunks[pack_buffer]);
    transfer_pending = true;
#if DOOM_ENABLE_PROFILING
    transfer_us += time_us_32() - tail_submit_started_us;
#endif
#else
    // 448x336 ends with 16 image rows in a 20-row transpose buffer. Preserve
    // the proven panel transaction size: clear the four unused low indices,
    // then submit a normal 20-row tile beginning four pixels inside the black
    // border. The 16 reversed image rows still land exactly at view offset 16.
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        for (int padding = 0; padding < PANEL_FINAL_PADDING; padding++) {
            panel_chunks[pack_buffer][x * PANEL_CHUNK_ROWS + padding] = 0;
        }
    }
    pack_buffer = flush_completed_chunk(
        DISPLAY_HEIGHT + PANEL_FINAL_PADDING - 1, pack_buffer
#if DOOM_ASYNC_AMOLED
        , &transfer_pending
#endif
#if DOOM_ENABLE_PROFILING
        , &transfer_us
#endif
    );
#endif
#endif
#if DOOM_ASYNC_AMOLED
    if (transfer_pending) {
#if DOOM_ENABLE_PROFILING
        uint32_t started_us = time_us_32();
#endif
        AMOLED_1IN8_DisplayWindowPackedWait();
#if DOOM_ENABLE_PROFILING
        transfer_us += time_us_32() - started_us;
#endif
        I_UpdateSound();
    }
#endif
#if DOOM_ENABLE_PROFILING
    uint32_t present_us = time_us_32() - present_started_us;
    *transfer_us_out = transfer_us;
    return present_us;
#endif
}

// core1: keeps pd_core1_loop() (core1's half of the per-frame render
// handshake with core0 - flats/columns work when USE_CORE1_FOR_FLATS/
// USE_CORE1_FOR_REGULAR are enabled, otherwise just the core0_done/
// core1_done rendezvous, servicing audio via SafeUpdateSound() while it
// waits) completely unchanged. What's gone is scanvideo_setup()/the DVI
// IRQ - replaced with a direct call to present the frame once it's ready.
static void core1(void) {
#if DOOM_BOOT_NEXT_WEAPON
    // flash_safe_execute() on core0 may proceed only after the other core has
    // registered its lockout handler. A failure leaves BOOT input disabled.
    boot_input_flash_safe_ready = flash_safe_execute_core_init();
#endif
    sem_release(&core1_launch);
    while (true) {
#if DOOM_ENABLE_PROFILING
        uint32_t frame_started_us = time_us_32();
        uint32_t core1_started_us = frame_started_us;
#endif
        pd_core1_loop();
#if DOOM_ENABLE_PROFILING
        uint32_t core1_us = time_us_32() - core1_started_us;
        uint32_t wait_started_us = time_us_32();
#endif
        new_frame_stuff();
#if DOOM_ENABLE_PROFILING
        uint32_t frame_wait_us = time_us_32() - wait_started_us;
        uint32_t transfer_us;
        uint32_t present_us = present_frame_to_amoled(&transfer_us);
        profile_finish_frame(frame_started_us, core1_us, frame_wait_us,
                             present_us, transfer_us);
#else
        present_frame_to_amoled();
#endif
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
    // The boot log is useful until graphics initialization succeeds, but its
    // later checkpoints repaint a white strip outside the centered game view.
    // Disable its DMA output first, then erase every non-game panel pixel.
    // A fresh boot enables it again, preserving early-boot and OOM reports.
    bootlog_disable();
    clear_panel_background();

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
