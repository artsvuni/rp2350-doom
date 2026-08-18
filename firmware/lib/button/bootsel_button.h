#ifndef _BOOTSEL_BUTTON_H
#define _BOOTSEL_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

/* Reads the physical BOOTSEL button while the firmware is running.
   Safe to poll in single-core firmware. Multicore callers must first ensure
   the other core cannot execute or read data from flash. */
bool bootsel_button_pressed(void);

/* Uses the Pico SDK flash-safety coordinator before sampling BOOTSEL.
   On success, writes the raw pressed state and returns PICO_OK. Multicore
   programs must call flash_safe_execute_core_init() on the other core before
   this can succeed safely. The caller must also ensure no DMA/XIP streamer is
   reading flash; flash_safe_execute coordinates CPU cores and IRQs only. */
int bootsel_button_pressed_flash_safe(bool *pressed, uint32_t timeout_ms);

#endif
