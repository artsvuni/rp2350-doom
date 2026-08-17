/*
 * PWR button, read via the AXP2101 power-management chip over I2C1
 * (the same shared bus as the ES8311 codec - see DEV_Config.h). PWR is
 * wired to the AXP2101's PWRON pin, not a plain RP2350 GPIO, so it can't
 * be read directly; the AXP2101 debounces/times the press itself and
 * reports short-press / long-press as IRQ status bits.
 *
 * Register reference: AXP2101 datasheet, REG 0x49 "IRQ Status 1" -
 * bit 3 = POWERON Short PRESS IRQ, bit 2 = POWERON Long PRESS IRQ, both
 * enabled by default (REG 0x41 bits 3/2). Status bits are RW1C - write
 * back what you read to clear exactly those bits.
 */
#include "pwr_button.h"
#include "DEV_Config.h"

#define AXP2101_I2C_ADDR    0x34
#define REG_IRQ_ENABLE1     0x41
#define REG_IRQ_STATUS1     0x49
#define BIT_SHORT_PRESS     (1 << 3)
#define BIT_LONG_PRESS      (1 << 2)
#define BIT_NEGATIVE_EDGE   (1 << 1)
#define BIT_POSITIVE_EDGE   (1 << 0)

void pwr_button_enable_edges(void)
{
    uint8_t enabled = DEV_I2C_ReadByte(AXP2101_I2C_ADDR, REG_IRQ_ENABLE1);
    DEV_I2C_Write(AXP2101_I2C_ADDR, REG_IRQ_ENABLE1,
                  enabled | BIT_NEGATIVE_EDGE | BIT_POSITIVE_EDGE);

    // Discard edges accumulated while the application was starting.  Keep
    // unrelated AXP2101 IRQ flags untouched.
    DEV_I2C_Write(AXP2101_I2C_ADDR, REG_IRQ_STATUS1,
                  BIT_SHORT_PRESS | BIT_LONG_PRESS
                  | BIT_NEGATIVE_EDGE | BIT_POSITIVE_EDGE);
}

pwr_button_event_t pwr_button_poll(void)
{
    uint8_t status = DEV_I2C_ReadByte(AXP2101_I2C_ADDR, REG_IRQ_STATUS1);
    uint8_t button_status = status & (BIT_SHORT_PRESS | BIT_LONG_PRESS
                                      | BIT_NEGATIVE_EDGE | BIT_POSITIVE_EDGE);

    if (button_status) {
        DEV_I2C_Write(AXP2101_I2C_ADDR, REG_IRQ_STATUS1, button_status);
    }

    uint8_t result = PWR_BUTTON_NONE;
    if (button_status & BIT_SHORT_PRESS) result |= PWR_BUTTON_SHORT_PRESS;
    if (button_status & BIT_LONG_PRESS) result |= PWR_BUTTON_LONG_PRESS;
    // PWRON is active-low on this board: falling edge is press, rising is
    // release. Hardware verification remains the final authority.
    if (button_status & BIT_NEGATIVE_EDGE) result |= PWR_BUTTON_PRESS_EDGE;
    if (button_status & BIT_POSITIVE_EDGE) result |= PWR_BUTTON_RELEASE_EDGE;
    return (pwr_button_event_t)result;
}
