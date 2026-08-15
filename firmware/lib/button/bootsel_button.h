#ifndef _BOOTSEL_BUTTON_H
#define _BOOTSEL_BUTTON_H

#include <stdbool.h>

/* Reads the physical BOOTSEL button while the firmware is running.
   Safe to call repeatedly from a polling loop; not interrupt-safe. */
bool bootsel_button_pressed(void);

#endif
