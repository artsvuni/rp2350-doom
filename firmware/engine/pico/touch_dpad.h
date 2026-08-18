#ifndef PICO_TOUCH_DPAD_H
#define PICO_TOUCH_DPAD_H

// Logical landscape coordinates for the asymmetric thumb layout. These are
// proportional to Alexander's 2026-08-18 sketch on the 448x368 panel:
// narrow edge strips for less-used left/back, much larger forward/right zones,
// no gaps between zones, and release as the only neutral action.
#define TOUCH_DPAD_X_MAX 340
#define TOUCH_DPAD_Y_MIN 70
// Keep the part of DOWN drawn over the game image 20 pixels tall. The earlier
// fixed y=304 happened to do exactly that at 448x280, but exposed 48 pixels
// after the image grew to 448x336. The remaining bottom border stays part of
// the physical target, preserving an easy-to-find edge control.
#if DOOM_DISPLAY_WIDTH == 448
#define TOUCH_DPAD_VISIBLE_DOWN_HEIGHT 20
#define TOUCH_DPAD_DOWN_Y \
    (((AMOLED_1IN8_WIDTH - DOOM_DISPLAY_HEIGHT) / 2) \
     + DOOM_DISPLAY_HEIGHT - TOUCH_DPAD_VISIBLE_DOWN_HEIGHT)
#else
#define TOUCH_DPAD_DOWN_Y 304
#endif
#define TOUCH_DPAD_LEFT_X 24
#define TOUCH_DPAD_UP_X 136
#define TOUCH_DPAD_DIAGONAL_HALF_WIDTH 6

typedef enum {
    TOUCH_DPAD_ZONE_NONE = 0,
    TOUCH_DPAD_ZONE_LEFT,
    TOUCH_DPAD_ZONE_UP,
    TOUCH_DPAD_ZONE_RIGHT,
    TOUCH_DPAD_ZONE_DOWN,
    TOUCH_DPAD_ZONE_UP_LEFT,
    TOUCH_DPAD_ZONE_UP_RIGHT,
} touch_dpad_zone_t;

touch_dpad_zone_t I_GetTouchDpadZone(void);

#endif
