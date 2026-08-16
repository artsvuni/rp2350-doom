//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2021-2022 Graham Sanderson
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
//	Main program, simply calls D_DoomMain high level loop.
//

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#if !LIB_PICO_STDLIB
#include "SDL.h"
#else
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/sem.h"
#include "pico/multicore.h"
#if PICO_ON_DEVICE
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#endif
#endif
#if USE_PICO_NET
#include "piconet.h"
#endif
#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h"
#if PICO_RP2350
#include "hardware/structs/qmi.h"
#endif
#include "DEV_Config.h"
#include "qspi_pio.h"
#include "AMOLED_1in8.h"
#include "bootlog.h"
//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
//

void D_DoomMain (void);

#if PICO_ON_DEVICE
#include "pico/binary_info.h"
// Upstream declares I2S pin binary-info here for their VGA-board reference
// hardware; we use the ES8311/PIO driver from mp3player instead (see
// lib/audio_pio, lib/es8311) - nothing to declare here.
#endif

int main(int argc, char **argv)
{
    // save arguments
#if !NO_USE_ARGS
    myargc = argc;
    myargv = argv;
#endif
#if PICO_ON_DEVICE
#if PICO_RP2350
    // This QMI (flash interface) clkdiv/rxdelay tweak is tuned to pair with
    // upstream's 270MHz overclock below (also disabled) - since RP2350
    // executes code directly from flash (XIP), mismatched timing here can
    // corrupt instruction fetches outright. Leaving flash interface timing
    // at its SDK-default (matched to our actual, stock clock) instead.
    // uint clkdiv = 3;
    // uint rxdelay = 2;
    // hw_write_masked(
    //         &qmi_hw->m[0].timing,
    //         ((clkdiv << QMI_M0_TIMING_CLKDIV_LSB) & QMI_M0_TIMING_CLKDIV_BITS) |
    //         ((rxdelay << QMI_M0_TIMING_RXDELAY_LSB) & QMI_M0_TIMING_RXDELAY_BITS),
    //         QMI_M0_TIMING_CLKDIV_BITS | QMI_M0_TIMING_RXDELAY_BITS
    // );
#endif
    // Upstream overclocks to 270MHz (with a voltage bump) here, tuned for
    // their RP2040 target - a silent hang here (no USB, no display, no
    // signs of life) with nothing past this point ever running pointed
    // straight at this. Our calibration firmware never does this and boots
    // fine at RP2350's stock clock; leaving performance tuning for later
    // once we've got something rendering. See doom/docs/DECISIONS.md.
    // vreg_set_voltage(VREG_VOLTAGE_1_30);
    // busy_wait_us(1000);
    // set_sys_clock_khz(270000, true);
#if !USE_PICO_NET
    // debug ?
//    gpio_debug_pins_init();
#endif
    // PICO_SMPS_MODE_PIN (GPIO23) comes from the generic Pico 2 board header
    // (we set PICO_BOARD=pico2 purely for toolchain/SDK purposes - this is a
    // completely different, custom Waveshare board). GPIO23 here is actually
    // our I2S LRCLK pin (see lib/audio_pio's PICO_AUDIO_LRCLK) - forcing it
    // to a GPIO output would collide with the audio codec wiring. Skipping
    // entirely; this board's own power supply doesn't need an SMPS mode pin
    // driven by us.
    // #ifdef PICO_SMPS_MODE_PIN
    //     gpio_init(PICO_SMPS_MODE_PIN);
    //     gpio_set_dir(PICO_SMPS_MODE_PIN, GPIO_OUT);
    //     gpio_put(PICO_SMPS_MODE_PIN, 1);
    // #endif
#endif
#if PICO_ON_DEVICE
    // TEMPORARY diagnostic: on-screen boot log (no serial needed) to find
    // where boot is hanging. See doom/docs/DECISIONS.md.
    bootlog_init();
    // NOT skipping ahead right now: first real hardware run of pd_render.cpp
    // (previously always stubbed out) produced a distorted/noisy screen
    // instead of a clean hang - need to see exactly which checkpoint was
    // last reached to tell where that's coming from. See DECISIONS.md.
    bootlog_print("1: bootlog init OK");
#endif
#if LIB_PICO_STDIO
    stdio_init_all();
#endif
#if PICO_ON_DEVICE
    bootlog_print("2: stdio_init_all OK");
#endif
#if PICO_BUILD
    I_Init();
#endif
#if PICO_ON_DEVICE
    bootlog_print("3: I_Init OK");
#endif
#if USE_PICO_NET
    // do init early to set pulls
    piconet_init();
#endif
#if PICO_ON_DEVICE
    bootlog_print("4: entering D_DoomMain");
#endif
//!
    // Print the program version and exit.
    //
    if (M_ParmExists("-version") || M_ParmExists("--version")) {
        puts(PACKAGE_STRING);
        exit(0);
    }

#if !NO_USE_ARGS
    M_FindResponseFile();
#endif

    #ifdef SDL_HINT_NO_SIGNAL_HANDLERS
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    #endif

    // start doom

    D_DoomMain ();

    return 0;
}

