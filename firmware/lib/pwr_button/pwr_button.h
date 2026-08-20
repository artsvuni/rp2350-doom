#ifndef _PWR_BUTTON_H_
#define _PWR_BUTTON_H_

typedef enum {
    PWR_BUTTON_NONE = 0,
    PWR_BUTTON_SHORT_PRESS = 1 << 0,
    PWR_BUTTON_LONG_PRESS = 1 << 1,
    PWR_BUTTON_PRESS_EDGE = 1 << 2,
    PWR_BUTTON_RELEASE_EDGE = 1 << 3,
} pwr_button_event_t;

/* Enables the AXP2101's normally-disabled PWRON edge IRQs. This exposes an
   immediate press/release pair for held controls while preserving the PMIC's
   existing short/long events and physical power behavior. */
void pwr_button_enable_edges(void);

/* Polls the AXP2101 power-management chip (I2C1, addr 0x34, reg 0x49)
   for pending PWRON events, clearing exactly the bits returned. Multiple
   events may be returned together, so test the result as a bit mask. */
pwr_button_event_t pwr_button_poll(void);

/* Ask the AXP2101 PMIC to turn off every power rail except VRTC.  This is
   the chip's documented software power-off command (REG 0x10 bit 0); it does
   not alter rail voltages or the physical PWR-button timing configuration. */
void pwr_button_power_off(void);

#endif
