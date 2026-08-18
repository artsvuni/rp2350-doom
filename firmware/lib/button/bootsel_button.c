/*
 * Reads the physical BOOTSEL button while the firmware is running.
 *
 * The board has no dedicated user button, but BOOTSEL is wired to the
 * flash chip's QSPI CS pin rather than a normal GPIO. To sample it, flash
 * access is briefly floated (Hi-Z) and the SIO "QSPI CSN" input bit is
 * read directly - the button pulls it low when held. This must run from
 * RAM (no flash access is possible while CS is floated) and with
 * interrupts disabled (an interrupt handler living in flash would hang
 * the chip). Adapted from the standard technique used by e.g.
 * earlephilhower/arduino-pico's Bootsel.cpp. This low-level driver handles
 * only the calling core; multicore callers must pause the other core before
 * entering it (the Doom input path uses Pico SDK multicore lockout).
 */
#include "bootsel_button.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "pico/flash.h"
#include "pico/platform.h"

static void __no_inline_not_in_flash_func(read_bootsel_raw)(void *pressed_out)
{
    const uint CS_PIN_INDEX = 1;

    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                     GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                     IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    for (volatile int i = 0; i < 1000; i++) {}

    *(bool *)pressed_out =
        !(sio_hw->gpio_hi_in & SIO_GPIO_HI_IN_QSPI_CSN_BITS);

    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                     GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                     IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
}

bool bootsel_button_pressed(void)
{
    bool pressed = false;
    uint32_t flags = save_and_disable_interrupts();
    read_bootsel_raw(&pressed);
    restore_interrupts(flags);
    return pressed;
}

int bootsel_button_pressed_flash_safe(bool *pressed, uint32_t timeout_ms)
{
    if (pressed == NULL) {
        return PICO_ERROR_INVALID_ARG;
    }

    *pressed = false;
    return flash_safe_execute(read_bootsel_raw, pressed, timeout_ms);
}
