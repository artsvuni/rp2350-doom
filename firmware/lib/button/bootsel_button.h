#ifndef _BOOTSEL_BUTTON_H
#define _BOOTSEL_BUTTON_H

#include <stdbool.h>

/* Reads the physical BOOTSEL button while the firmware is running.
   Safe to poll in single-core firmware. Multicore callers must first ensure
   the other core cannot execute or read data from flash. */
bool bootsel_button_pressed(void);

#endif
