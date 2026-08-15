/*
 * Minimal AMOLED display support: just enough to show "PLAY" or "PAUSE"
 * as large centered text, so there's some visual feedback instead of a
 * blank screen. Init sequence follows Waveshare's C/01-LCD example
 * (QSPI PIO bring-up + GUI_Paint framebuffer setup); touch and the IMU
 * aren't used here.
 */
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "DEV_Config.h"
#include "qspi_pio.h"
#include "AMOLED_1in8.h"
#include "GUI_Paint.h"
#include "fonts.h"

static UWORD *framebuffer;

void display_init(void)
{
    QSPI_GPIO_Init(qspi);
    QSPI_PIO_Init(qspi);
    QSPI_4Wrie_Mode(&qspi);

    AMOLED_1IN8_Init();
    AMOLED_1IN8_SetBrightness(100);

    UDOUBLE image_size = (UDOUBLE)AMOLED_1IN8_HEIGHT * AMOLED_1IN8_WIDTH * 2;
    framebuffer = (UWORD *)malloc(image_size);
    if (!framebuffer) return;

    Paint_NewImage((UBYTE *)framebuffer, AMOLED_1IN8.WIDTH, AMOLED_1IN8.HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    // Landscape: physical buttons are on the panel's native left edge, so
    // after this rotation they end up effectively "on top" relative to
    // the rotated image - see docs/DECISIONS.md.
    Paint_SetRotate(ROTATE_90);
    Paint_Clear(WHITE);
    AMOLED_1IN8_Display(framebuffer);
}

void display_show_state(bool playing)
{
    if (!framebuffer) return;

    const char *text = playing ? "PAUSE" : "PLAY";
    uint16_t text_width = strlen(text) * Font24.Width;
    uint16_t x = (Paint.Width - text_width) / 2;
    uint16_t y = (Paint.Height - Font24.Height) / 2;

    Paint_Clear(WHITE);
    Paint_DrawString_EN(x, y, text, &Font24, BLACK, WHITE);
    AMOLED_1IN8_Display(framebuffer);
}

void display_show_lines(const char *line1, const char *line2)
{
    if (!framebuffer) return;

    uint16_t x1 = (Paint.Width - strlen(line1) * Font24.Width) / 2;
    uint16_t x2 = (Paint.Width - strlen(line2) * Font24.Width) / 2;
    uint16_t y1 = Paint.Height / 2 - Font24.Height - 8;
    uint16_t y2 = Paint.Height / 2 + 8;

    Paint_Clear(WHITE);
    Paint_DrawString_EN(x1, y1, line1, &Font24, BLACK, WHITE);
    Paint_DrawString_EN(x2, y2, line2, &Font24, BLACK, WHITE);
    AMOLED_1IN8_Display(framebuffer);
}
