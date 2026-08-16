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

#define BOOTLOG_HEIGHT 32 // shrunk again: pd_render.cpp's real (non-stub) static state pushed .bss
                          // over RAM by ~11KB - combined with bootlog_skip_until() in i_main.c to
                          // skip straight to the new checkpoints, 1 line is enough. See DECISIONS.md.
#define LINE_HEIGHT (Font16.Height + 2)
#define MAX_LINES (BOOTLOG_HEIGHT / LINE_HEIGHT)

// Full panel width (matches the buffer's natural stride - no repacking
// needed), Y-windowed, presented via AMOLED_1IN8_DisplayWindowPacked()'s
// single DMA transfer - AMOLED_1IN8_DisplayWindows() proved intermittently
// unreliable on this hardware. See doom/docs/DECISIONS.md.
static UWORD fb[AMOLED_1IN8_WIDTH * BOOTLOG_HEIGHT];
static int next_line;
static int print_count;
static int skip_count;

// bootlog_print() can genuinely be called from BOTH cores: most call
// sites are core0-only (game logic, menu, input), but i_picosound.c's
// I_Pico_UpdateSound() - which has its own checkpoints - is called from
// pd_render.cpp's SafeUpdateSound() on EITHER core (whichever wins that
// function's own mutex_try_enter race). AMOLED_1in8.c's dma_tx_mutex only
// protects the actual DMA transfer; it says nothing about fb[]/next_line/
// print_count here, which get read and written BEFORE that call, with no
// protection at all. Two concurrent callers modifying fb[]/next_line at
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
    next_line = 0;
}

void bootlog_print(const char *msg)
{
    mutex_enter_blocking(&bootlog_mutex);

    print_count++;
    if (print_count < skip_count) {
        mutex_exit(&bootlog_mutex);
        return;
    }

    if (next_line >= MAX_LINES) {
        Paint_Clear(WHITE);
        next_line = 0;
    }
    Paint_DrawString_EN(2, next_line * LINE_HEIGHT, msg, &Font16, BLACK, WHITE);
    next_line++;
    AMOLED_1IN8_DisplayWindowPacked(0, 0, AMOLED_1IN8_WIDTH, BOOTLOG_HEIGHT, fb);

    mutex_exit(&bootlog_mutex);
}
