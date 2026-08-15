#ifndef _PWR_BUTTON_H_
#define _PWR_BUTTON_H_

typedef enum {
    PWR_BUTTON_NONE = 0,
    PWR_BUTTON_SHORT_PRESS,
    PWR_BUTTON_LONG_PRESS,
} pwr_button_event_t;

/* Polls the AXP2101 power-management chip (I2C1, addr 0x34, reg 0x49)
   for a pending PWRON short/long-press IRQ, clearing it if found. The
   chip itself decides what counts as "short" vs "long" - there's no
   press-duration logic on our side for this button, unlike BOOTSEL. */
pwr_button_event_t pwr_button_poll(void);

#endif
