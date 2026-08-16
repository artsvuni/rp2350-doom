/*
 * TEMPORARY diagnostic aid: prints short status lines directly to the
 * AMOLED panel as boot proceeds, so progress is visible without a serial
 * connection. See doom/docs/DECISIONS.md.
 */
#include "bootlog.h"
#include "DEV_Config.h"
#include "qspi_pio.h"
#include "AMOLED_1in8.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "pico/mutex.h"
#include <string.h>

#define BOOTLOG_HEIGHT 64 // grown back from 32: now shows a scrolling
                          // history (HISTORY_LINES, below) instead of a
                          // single line, so a freeze shows the last few
                          // checkpoints leading up to it, not just the
                          // very last one - and no single message gets
                          // overwritten before it can be read/photographed.
                          // RAM budget has had headroom for this again
                          // since panel_window shrank (~105KB) - see
                          // doom/docs/DECISIONS.md.
#define LINE_HEIGHT (Font16.Height + 2)
// Font16.Height is a runtime struct member access (Font16 is extern), not
// a real compile-time constant - fine for the runtime arithmetic below,
// but invalid as a static array's dimension. Font16 is a fixed 16px font
// (see lib/fonts/font16.c) - hardcode that fact here instead.
#define HISTORY_LINES (BOOTLOG_HEIGHT / (16 + 2)) // = 3
#define MSG_MAXLEN 40

// Full panel width (matches the buffer's natural stride - no repacking
// needed), Y-windowed, presented via AMOLED_1IN8_DisplayWindowPacked()'s
// single DMA transfer - AMOLED_1IN8_DisplayWindows() proved intermittently
// unreliable on this hardware. See doom/docs/DECISIONS.md.
static UWORD fb[AMOLED_1IN8_WIDTH * BOOTLOG_HEIGHT];
static char history[HISTORY_LINES][MSG_MAXLEN];
static int history_count; // valid entries so far, caps at HISTORY_LINES
static int print_count;
static int skip_count;

// bootlog_print() can genuinely be called from BOTH cores: most call
// sites are core0-only (game logic, menu, input), but i_picosound.c's
// I_Pico_UpdateSound() - which has its own checkpoints - is called from
// pd_render.cpp's SafeUpdateSound() on EITHER core (whichever wins that
// function's own mutex_try_enter race). AMOLED_1in8.c's dma_tx_mutex only
// protects the actual DMA transfer; it says nothing about fb[]/history/
// print_count here, which get read and written BEFORE that call, with no
// protection at all. Two concurrent callers modifying fb[]/history at
// the same time is a very plausible explanation for bootlog text showing
// up in screen locations its own code can never target (its window is
// hardcoded to Y:[0,BOOTLOG_HEIGHT)) - found 2026-08-16 chasing a
// deterministic freeze right after the first real sound effect started
// playing (which is also the first time SafeUpdateSound's audio path
// actually does real work instead of a no-op silence push) - see
// doom/docs/DECISIONS.md. Initialized in bootlog_init(), which always
// runs single-core (before core1 exists), so lazy-init isn't even needed
// here - do it eagerly, unconditionally, to be safe regardless of call
// order between the two _init functions.
static mutex_t bootlog_mutex;

void bootlog_skip_until(int n)
{
    skip_count = n;
}

void bootlog_init(void)
{
    // dma_tx (the DMA channel the AMOLED driver uses) is only ever
    // assigned inside DEV_Module_Init() - without this call it's an
    // uninitialized global (defaults to 0, an unclaimed/unconfigured
    // channel), and dma_channel_is_busy() on it can spin forever. This is
    // what was actually causing the full hang - see doom/docs/DECISIONS.md.
    mutex_init(&bootlog_mutex);
    DEV_Module_Init();
    QSPI_GPIO_Init(qspi);
    QSPI_PIO_Init(qspi);
    QSPI_4Wrie_Mode(&qspi);
    AMOLED_1IN8_Init();
    AMOLED_1IN8_SetBrightness(100);

    Paint_NewImage((UBYTE *)fb, AMOLED_1IN8_WIDTH, BOOTLOG_HEIGHT, 0, WHITE);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    Paint_Clear(WHITE);
    AMOLED_1IN8_DisplayWindowPacked(0, 0, AMOLED_1IN8_WIDTH, BOOTLOG_HEIGHT, fb);
    history_count = 0;
}

void bootlog_print(const char *msg)
{
    mutex_enter_blocking(&bootlog_mutex);

    print_count++;
    if (print_count < skip_count) {
        mutex_exit(&bootlog_mutex);
        return;
    }

    // Shift older messages up, insert the new one at the bottom - always
    // shows the last HISTORY_LINES checkpoints, oldest at top.
    for (int i = 0; i < HISTORY_LINES - 1; i++) {
        memcpy(history[i], history[i + 1], MSG_MAXLEN);
    }
    strncpy(history[HISTORY_LINES - 1], msg, MSG_MAXLEN - 1);
    history[HISTORY_LINES - 1][MSG_MAXLEN - 1] = 0;
    if (history_count < HISTORY_LINES) history_count++;

    Paint_Clear(WHITE);
    int start = HISTORY_LINES - history_count;
    for (int i = start; i < HISTORY_LINES; i++) {
        Paint_DrawString_EN(2, (i - start) * LINE_HEIGHT, history[i], &Font16, BLACK, WHITE);
    }
    AMOLED_1IN8_DisplayWindowPacked(0, 0, AMOLED_1IN8_WIDTH, BOOTLOG_HEIGHT, fb);

    mutex_exit(&bootlog_mutex);
}
