//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2021-2022 Graham Sanderson
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//     SDL implementation of system-specific input interface.
//


//#include "SDL.h"
//#include "SDL_keycode.h"
#include <doom/sounds.h>
#include <doom/s_sound.h>
#include "pico.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "doomstat.h"
#include "d_event.h"
#include "i_input.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "hardware/uart.h"
#include "pico/time.h"
#include "pwr_button.h"
#include "DEV_Config.h"
#include "FT3168.h"
#include "qmi8658.h"
#include "AMOLED_1in8.h"
#include "bootlog.h"
#include <stdlib.h>
#if USB_SUPPORT
#include "pico/binary_info.h"
#include "tusb.h"
#include "hardware/irq.h"
bi_decl(bi_program_feature("USB keyboard support"));
#endif

static const int scancode_translate_table[] = SCANCODE_TO_KEYS_ARRAY;

// Lookup table for mapping ASCII characters to their equivalent when
// shift is pressed on a US layout keyboard. This is the original table
// as found in the Doom sources, comments and all.
static const char shiftxform[] =
        {
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                31, ' ', '!', '"', '#', '$', '%', '&',
                '"', // shift-'
                '(', ')', '*', '+',
                '<', // shift-,
                '_', // shift--
                '>', // shift-.
                '?', // shift-/
                ')', // shift-0
                '!', // shift-1
                '@', // shift-2
                '#', // shift-3
                '$', // shift-4
                '%', // shift-5
                '^', // shift-6
                '&', // shift-7
                '*', // shift-8
                '(', // shift-9
                ':',
                ':', // shift-;
                '<',
                '+', // shift-=
                '>', '?', '@',
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                '[', // shift-[
                '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
                ']', // shift-]
                '"', '_',
                '\'', // shift-`
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                '{', '|', '}', '~', 127
        };

// If true, I_StartTextInput() has been called, and we are populating
// the data3 field of ev_keydown events.
static boolean text_input_enabled = true;

// Bit mask of mouse button state.
static unsigned int mouse_button_state = 0;

// Disallow mouse and joystick movement to cause forward/backward
// motion.  Specified with the '-novert' command line parameter.
// This is an int to allow saving to config file
int novert = 0;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

#if !USE_VANILLA_KEYBOARD_MAPPING_ONLY
int vanilla_keyboard_mapping = true;
#endif

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.
#if !NO_USE_MOUSE
float mouse_acceleration = 2.0;
int mouse_threshold = 10;
#endif

enum {
    SDL_SCANCODE_SPACE = 44,
    SDL_SCANCODE_LCTRL = 224,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_LALT = 226, /**< alt, option */
    SDL_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
    SDL_SCANCODE_RCTRL = 228,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_SCANCODE_RALT = 230, /**< alt gr, option */
    SDL_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */
};

// Translates the SDL key to a value of the type found in doomkeys.h
int TranslateKey(int scancode)
{
    switch (scancode)
    {
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return KEY_RCTRL;

        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return KEY_RSHIFT;

        case SDL_SCANCODE_LALT:
            return KEY_LALT;

        case SDL_SCANCODE_RALT:
            return KEY_RALT;

        default:
            if (scancode >= 0 && scancode < arrlen(scancode_translate_table))
            {
                return scancode_translate_table[scancode];
            }
            else
            {
                return 0;
            }
    }
}

// Get the localized version of the key press. This takes into account the
// keyboard layout, but does not apply any changes due to modifiers, (eg.
// shift-, alt-, etc.)
static int GetLocalizedKey(int scancode)
{
    // When using Vanilla mapping, we just base everything off the scancode
    // and always pretend the user is using a US layout keyboard.
    if (vanilla_keyboard_mapping)
    {
        return TranslateKey(scancode);
    }
    else
    {
        assert(false); return 0;
//        int result = sym->sym;
//
//        if (result < 0 || result >= 128)
//        {
//            result = 0;
//        }
//
//        return sym_<result;
    }
}

// Get the equivalent ASCII (Unicode?) character for a keypress.
int GetTypedChar(int scancode, boolean shiftdown)
{
    // We only return typed characters when entering text, after
    // I_StartTextInput() has been called. Otherwise we return nothing.
    if (!text_input_enabled)
    {
        return 0;
    }

    // If we're strictly emulating Vanilla, we should always act like
    // we're using a US layout keyboard (in ev_keydown, data1=data2).
    // Otherwise we should use the native key mapping.
    if (vanilla_keyboard_mapping)
    {
        int result = TranslateKey(scancode);

        // If shift is held down, apply the original uppercase
        // translation table used under DOS.
        if (shiftdown
            && result >= 0 && result < arrlen(shiftxform))
        {
            result = shiftxform[result];
        }

        return result;
    }
    else
    {
#if 0
        SDL_Event next_event;

        // Special cases, where we always return a fixed value.
        switch (sym->sym)
        {
            case SDLK_BACKSPACE: return KEY_BACKSPACE;
            case SDLK_RETURN:    return KEY_ENTER;
            default:
                break;
        }

        // The following is a gross hack, but I don't see an easier way
        // of doing this within the SDL2 API (in SDL1 it was easier).
        // We want to get the fully transformed input character associated
        // with this keypress - correct keyboard layout, appropriately
        // transformed by any modifier keys, etc. So peek ahead in the SDL
        // event queue and see if the key press is immediately followed by
        // an SDL_TEXTINPUT event. If it is, it's reasonable to assume the
        // key press and the text input are connected. Technically the SDL
        // API does not guarantee anything of the sort, but in practice this
        // is what happens and I've verified it through manual inspect of
        // the SDL source code.
        //
        // In an ideal world we'd split out ev_keydown into a separate
        // ev_textinput event, as SDL2 has done. But this doesn't work
        // (I experimented with the idea), because lots of Doom's code is
        // based around different responders "eating" events to stop them
        // being passed on to another responder. If code is listening for
        // a text input, it cannot block the corresponding keydown events
        // which can affect other responders.
        //
        // So we're stuck with this as a rather fragile alternative.

        if (SDL_PeepEvents(&next_event, 1, SDL_PEEKEVENT,
                           SDL_FIRSTEVENT, SDL_LASTEVENT) == 1
            && next_event.type == SDL_TEXTINPUT)
        {
            // If an SDL_TEXTINPUT event is found, we always assume it
            // matches the key press. The input text must be a single
            // ASCII character - if it isn't, it's possible the input
            // char is a Unicode value instead; better to send a null
            // character than the unshifted key.
            if (strlen(next_event.text.text) == 1
                && (next_event.text.text[0] & 0x80) == 0)
            {
                return next_event.text.text[0];
            }
        }
#else
        assert(false);
#endif

        // Failed to find anything :/
        return 0;
    }
}

void I_StartTextInput(int x1, int y1, int x2, int y2)
{
    text_input_enabled = true;

    if (!vanilla_keyboard_mapping)
    {
#if !USE_VANILLA_KEYBOARD_MAPPING_ONLY
        // SDL2-TODO: SDL_SetTextInputRect(...);
        SDL_StartTextInput();
#endif
    }
}

void I_StopTextInput(void)
{
    text_input_enabled = false;

    if (!vanilla_keyboard_mapping)
    {
#if !USE_VANILLA_KEYBOARD_MAPPING_ONLY
        SDL_StopTextInput();
#endif
    }
}

#if !NO_USE_MOUSE
static void UpdateMouseButtonState(unsigned int button, boolean on)
{
    static event_t event;

    if (button < SDL_BUTTON_LEFT || button > MAX_MOUSE_BUTTONS)
    {
        return;
    }

    // Note: button "0" is left, button "1" is right,
    // button "2" is middle for Doom.  This is different
    // to how SDL sees things.

    switch (button)
    {
        case SDL_BUTTON_LEFT:
            button = 0;
            break;

        case SDL_BUTTON_RIGHT:
            button = 1;
            break;

        case SDL_BUTTON_MIDDLE:
            button = 2;
            break;

        default:
            // SDL buttons are indexed from 1.
            --button;
            break;
    }

    // Turn bit representing this button on or off.

    if (on)
    {
        mouse_button_state |= (1 << button);
    }
    else
    {
        mouse_button_state &= ~(1 << button);
    }

    // Post an event with the new button state.

    event.type = ev_mouse;
    event.data1 = mouse_button_state;
    event.data2 = event.data3 = 0;
    D_PostEvent(&event);
}

static void MapMouseWheelToButtons(SDL_MouseWheelEvent *wheel)
{
    // SDL2 distinguishes button events from mouse wheel events.
    // We want to treat the mouse wheel as two buttons, as per
    // SDL1
    static event_t up, down;
    int button;

    if (wheel->y <= 0)
    {   // scroll down
        button = 4;
    }
    else
    {   // scroll up
        button = 3;
    }

    // post a button down event
    mouse_button_state |= (1 << button);
    down.type = ev_mouse;
    down.data1 = mouse_button_state;
    down.data2 = down.data3 = 0;
    D_PostEvent(&down);

    // post a button up event
    mouse_button_state &= ~(1 << button);
    up.type = ev_mouse;
    up.data1 = mouse_button_state;
    up.data2 = up.data3 = 0;
    D_PostEvent(&up);
}

void I_HandleMouseEvent(SDL_Event *sdlevent)
{
    switch (sdlevent->type)
    {
        case SDL_MOUSEBUTTONDOWN:
            UpdateMouseButtonState(sdlevent->button.button, true);
            break;

        case SDL_MOUSEBUTTONUP:
            UpdateMouseButtonState(sdlevent->button.button, false);
            break;

        case SDL_MOUSEWHEEL:
            MapMouseWheelToButtons(&(sdlevent->wheel));
            break;

        default:
            break;
    }
}

static int AccelerateMouse(int val)
{
    if (val < 0)
        return -AccelerateMouse(-val);

    if (val > mouse_threshold)
    {
        return (int)((val - mouse_threshold) * mouse_acceleration + mouse_threshold);
    }
    else
    {
        return val;
    }
}

//
// Read the change in mouse state to generate mouse motion events
//
// This is to combine all mouse movement for a tic into one mouse
// motion event.
void I_ReadMouse(void)
{
    int x, y;
    event_t ev;

    SDL_GetRelativeMouseState(&x, &y);

    if (x != 0 || y != 0)
    {
        ev.type = ev_mouse;
        ev.data1 = mouse_button_state;
        ev.data2 = AccelerateMouse(x);

        if (!novert)
        {
            ev.data3 = -AccelerateMouse(y);
        }
        else
        {
            ev.data3 = 0;
        }

        // XXX: undefined behaviour since event is scoped to
        // this function
        D_PostEvent(&ev);
    }
}
#endif

// Bind all variables controlling input options.
void I_BindInputVariables(void)
{
#if !NO_USE_MOUSE
    M_BindFloatVariable("mouse_acceleration",      &mouse_acceleration);
    M_BindIntVariable("mouse_threshold",           &mouse_threshold);
#endif
#if !USE_VANILLA_KEYBOARD_MAPPING_ONLY
    M_BindIntVariable("vanilla_keyboard_mapping",  &vanilla_keyboard_mapping);
#endif
    M_BindIntVariable("novert",                    &novert);
}

#if PICO_NO_HARDWARE
#include "pico/scanvideo.h"
#else
#define WITH_SHIFT 0x8000
#endif

static void pico_key_down(int scancode, int keysym, int modifiers) {
    event_t event;
    event.type = ev_keydown;
    event.data1 = TranslateKey(scancode);
    event.data2 = GetLocalizedKey(scancode);
    event.data3 = GetTypedChar(scancode, modifiers & WITH_SHIFT ? 1 : 0);

    if (at_exit_screen) {
        handle_exit_key_down(scancode, modifiers & WITH_SHIFT ? 1 : 0, exit_screen_kb_buffer_80, 80);
        return;
    }
    if (event.data1 != 0)
    {
        D_PostEvent(&event);
    }
}

static void pico_key_up(int scancode, int keysym, int modifiers) {
    event_t event;
    event.type = ev_keyup;
    event.data1 = TranslateKey(scancode);
    // data2/data3 are initialized to zero for ev_keyup.
    // For ev_keydown it's the shifted Unicode character
    // that was typed, but if something wants to detect
    // key releases it should do so based on data1
    // (key ID), not the printable char.
    event.data2 = 0;
    event.data3 = 0;
    if (event.data1 != 0)
    {
        D_PostEvent(&event);
    }
}

#if PICO_NO_HARDWARE
static void pico_quit(void) {
    exit(0);
}
#endif

// --- Hardware controls: touch d-pad + PWR button --------------------------
//
// Design: doom/docs/DECISIONS.md, 2026-08-15 entry. Touch zones are held-
// to-move (mirrors key_up/down/left/right's own semantics exactly, so this
// just needs to track transitions and post the same keydown/keyup events a
// keyboard would). PWR is momentary (I2C-read chip event, not a live GPIO
// level) so single/double-press are synthesized here as brief keydown-then-
// keyup pulses spanning one tic - see PulseKey() below for why it takes two
// calls (one tic) to do that rather than posting both events at once.
//
// BOOT is deliberately not read during gameplay. It shares the external
// flash QSPI CS signal, and both hardware incidents occurred only in builds
// that sampled it at runtime. Keep it exclusively for entering BOOTSEL while
// powering/resetting the board.
//
// Control model selector. The original model uses four fixed invisible hold
// zones (preserved below for instant fallback). The experimental model is a
// floating four-way digital joystick: touch anywhere, drag past a dead zone,
// then hold or slide around the original anchor.
#define TOUCH_CONTROL_SWIPE_HOLD 1

// Original fixed-zone layout, copied verbatim from the calibration firmware.
typedef struct {
    const char *name;
    int x0, y0, x1, y1; // half-open [x0,x1) x [y0,y1) in logical landscape space
    key_type_t key;
} touch_zone_t;

static const touch_zone_t touch_zones[] = {
    { "LEFT",  0,   270, 65,  345, KEY_LEFTARROW },
    { "UP",    70,  90,  100, 300, KEY_UPARROW },
    { "DOWN",  70,  300, 100, 368, KEY_DOWNARROW },
    { "RIGHT", 105, 260, 290, 340, KEY_RIGHTARROW },
};
#define NUM_TOUCH_ZONES (sizeof(touch_zones) / sizeof(touch_zones[0]))

static void TouchToLogical(uint16_t raw_x, uint16_t raw_y, int *logical_x, int *logical_y)
{
    // Inverse of Paint_SetPixel's ROTATE_90 transform (X=W-y-1, Y=x).
    *logical_x = raw_y;
    *logical_y = AMOLED_1IN8_WIDTH - raw_x - 1;
}

// Returns the held zone's key, or 0 if no zone (or no finger) is down.
static key_type_t PollTouchZoneKey(void)
{
    uint8_t fingers = FT3168_Get_Point();
    if (fingers == 0) return 0;

    int lx, ly;
    TouchToLogical(FT3168.x_point, FT3168.y_point, &lx, &ly);

    for (size_t i = 0; i < NUM_TOUCH_ZONES; i++) {
        const touch_zone_t *z = &touch_zones[i];
        if (lx >= z->x0 && lx < z->x1 && ly >= z->y0 && ly < z->y1) return z->key;
    }
    return 0;
}

// Floating digital joystick / swipe-and-hold model. A gesture begins wherever
// the finger lands. Dominant-axis bias avoids rapid left/up or right/down
// changes near 45-degree diagonals; the caller adds a two-tic transition
// stability filter as a second line of defense against controller jitter.
#define SWIPE_DEAD_ZONE_PX 24
#define SWIPE_AXIS_BIAS_PX 8
static key_type_t PollTouchSwipeHoldKey(void)
{
    static bool gesture_active = false;
    static int anchor_x, anchor_y;
    static key_type_t chosen_key = 0;

    uint8_t fingers = FT3168_Get_Point();
    if (fingers == 0) {
        gesture_active = false;
        chosen_key = 0;
        return 0;
    }

    int lx, ly;
    TouchToLogical(FT3168.x_point, FT3168.y_point, &lx, &ly);

    if (!gesture_active) {
        gesture_active = true;
        anchor_x = lx;
        anchor_y = ly;
        chosen_key = 0;
        return 0;
    }

    int dx = lx - anchor_x;
    int dy = ly - anchor_y;
    int ax = abs(dx);
    int ay = abs(dy);
    if (ax < SWIPE_DEAD_ZONE_PX && ay < SWIPE_DEAD_ZONE_PX) {
        chosen_key = 0;
        return 0;
    }

    bool horizontal;
    if (ax > ay + SWIPE_AXIS_BIAS_PX) {
        horizontal = true;
    } else if (ay > ax + SWIPE_AXIS_BIAS_PX) {
        horizontal = false;
    } else if (chosen_key == KEY_LEFTARROW || chosen_key == KEY_RIGHTARROW) {
        horizontal = true;
    } else if (chosen_key == KEY_UPARROW || chosen_key == KEY_DOWNARROW) {
        horizontal = false;
    } else {
        horizontal = ax > ay;
    }

    chosen_key = horizontal
        ? (dx < 0 ? KEY_LEFTARROW : KEY_RIGHTARROW)
        : (dy < 0 ? KEY_UPARROW : KEY_DOWNARROW);
    return chosen_key;
}

static key_type_t PollTouchMovementKey(void)
{
#if TOUCH_CONTROL_SWIPE_HOLD
    return PollTouchSwipeHoldKey();
#else
    return PollTouchZoneKey();
#endif
}

#if DOOM_HYBRID_CONTROLS
// Touch-first model: a two-axis drag owns vertical movement and horizontal
// turning. Static state only; values are sampled once per Doom tic and applied
// at the ticcmd boundary by I_ApplyHardwareTiccmd().
#define HYBRID_TURN_DEAD_ZONE_PX 1
#define HYBRID_TURN_FULL_SCALE_PX 112
#define HYBRID_TURN_MIN 48
#define HYBRID_TURN_MAX 960
#define HYBRID_MOVE_DEAD_ZONE_PX 1
#define HYBRID_MOVE_FULL_SCALE_PX 140
#define HYBRID_MOVE_MIN 4
#define HYBRID_MOVE_MAX 50
#define HYBRID_TAP_MOVE_PX 12
#define HYBRID_TAP_MAX_US (260 * 1000)
#define HYBRID_DOUBLE_TAP_US (340 * 1000)
#define HYBRID_DOUBLE_TAP_DISTANCE_PX 52
#define HYBRID_STRAFE_CORNER_WIDTH_PX 96
#define HYBRID_STRAFE_CORNER_HEIGHT_PX 72
#define HYBRID_TOUCH_STRAFE_SPEED 32
#define HYBRID_TOUCH_STRAFE_TICS 6
static int16_t hybrid_turn;
static int16_t hybrid_forward;
static int16_t hybrid_touch_strafe;
static uint8_t hybrid_touch_strafe_tics;

typedef enum {
    HYBRID_TOUCH_ACTION_NONE,
    HYBRID_TOUCH_ACTION_USE,
    HYBRID_TOUCH_ACTION_STRAFE_LEFT,
    HYBRID_TOUCH_ACTION_STRAFE_RIGHT,
} hybrid_touch_action_t;

#if DOOM_ROLL_STRAFE
#define MOTION_CALIBRATION_SAMPLES 18
#define MOTION_CALIBRATION_STABLE_RAW 320
#define MOTION_CALIBRATION_SPAN_RAW 640
#define MOTION_STRAFE_START_TAN_Q10 181  // tan(10 degrees) * 1024
#define MOTION_STRAFE_STOP_TAN_Q10 90    // tan(5 degrees) * 1024
#define MOTION_STRAFE_FULL_TAN_Q10 414   // tan(22 degrees) * 1024
#define MOTION_STRAFE_CONFIRM_TICS 2
#define MOTION_STRAFE_MIN 8
#define MOTION_STRAFE_MAX 32

#ifndef DOOM_MOTION_ROLL_SIGN
#define DOOM_MOTION_ROLL_SIGN 1
#endif

static int16_t hybrid_motion_strafe;
static bool hybrid_touch_down;
static bool hybrid_imu_available;
#endif

static int16_t ScaleTouchTurn(int dx)
{
    int magnitude = abs(dx);
    if (magnitude <= HYBRID_TURN_DEAD_ZONE_PX) return 0;

    magnitude -= HYBRID_TURN_DEAD_ZONE_PX;
    int range = HYBRID_TURN_FULL_SCALE_PX - HYBRID_TURN_DEAD_ZONE_PX;
    if (magnitude > range) magnitude = range;

    // Keep small pointing-finger motions precise, then accelerate toward a
    // fast turn at the outer edge of the bottom-left control quadrant.
    int scaled = HYBRID_TURN_MIN
        + (HYBRID_TURN_MAX - HYBRID_TURN_MIN)
          * magnitude * magnitude / (range * range);
    return dx > 0 ? (int16_t)-scaled : (int16_t)scaled;
}

static int16_t ScaleTouchMove(int dy)
{
    int magnitude = abs(dy);
    if (magnitude <= HYBRID_MOVE_DEAD_ZONE_PX) return 0;

    magnitude -= HYBRID_MOVE_DEAD_ZONE_PX;
    int range = HYBRID_MOVE_FULL_SCALE_PX - HYBRID_MOVE_DEAD_ZONE_PX;
    if (magnitude > range) magnitude = range;

    // Keep the middle of the gesture near normal walking speed and reserve
    // Doom's run speed for a deliberate reach toward the far side of the
    // screen. The quadratic curve preserves immediate low-speed response.
    int scaled = HYBRID_MOVE_MIN
        + (HYBRID_MOVE_MAX - HYBRID_MOVE_MIN)
          * magnitude * magnitude / (range * range);
    return dy < 0 ? (int16_t)scaled : (int16_t)-scaled;
}

static hybrid_touch_action_t ClassifyHybridDoubleTap(int x, int y)
{
    if (y < AMOLED_1IN8_WIDTH - HYBRID_STRAFE_CORNER_HEIGHT_PX) {
        return HYBRID_TOUCH_ACTION_USE;
    }
    if (x < HYBRID_STRAFE_CORNER_WIDTH_PX) {
        return HYBRID_TOUCH_ACTION_STRAFE_LEFT;
    }
    if (x >= AMOLED_1IN8_HEIGHT - HYBRID_STRAFE_CORNER_WIDTH_PX) {
        return HYBRID_TOUCH_ACTION_STRAFE_RIGHT;
    }
    return HYBRID_TOUCH_ACTION_USE;
}

// Returns one action exactly once when a clean double tap completes. Bottom
// corners own bounded strafe bursts; the rest of the screen remains Use/Open.
static hybrid_touch_action_t PollHybridTouch(bool active)
{
    static bool was_down;
    static int anchor_x, anchor_y;
    static int max_motion;
    static uint32_t down_time_us;
    static bool first_tap_valid;
    static uint32_t first_tap_time_us;
    static int first_tap_x, first_tap_y;
    static hybrid_touch_action_t first_tap_action;

    if (!active) {
        was_down = false;
        first_tap_valid = false;
        hybrid_turn = 0;
        hybrid_forward = 0;
#if DOOM_ROLL_STRAFE
        hybrid_touch_down = false;
#endif
        hybrid_touch_strafe = 0;
        hybrid_touch_strafe_tics = 0;
        return HYBRID_TOUCH_ACTION_NONE;
    }

    uint8_t fingers = FT3168_Get_Point();
    uint32_t now = time_us_32();
    if (fingers != 0) {
#if DOOM_ROLL_STRAFE
        hybrid_touch_down = true;
#endif
        int lx, ly;
        TouchToLogical(FT3168.x_point, FT3168.y_point, &lx, &ly);

        if (!was_down) {
            was_down = true;
            anchor_x = lx;
            anchor_y = ly;
            max_motion = 0;
            down_time_us = now;
            hybrid_turn = 0;
            hybrid_forward = 0;
            return HYBRID_TOUCH_ACTION_NONE;
        }

        int dx = lx - anchor_x;
        int dy = ly - anchor_y;
        int motion = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
        if (motion > max_motion) max_motion = motion;
        hybrid_turn = ScaleTouchTurn(dx);
        hybrid_forward = ScaleTouchMove(dy);
        return HYBRID_TOUCH_ACTION_NONE;
    }

    hybrid_turn = 0;
    hybrid_forward = 0;
#if DOOM_ROLL_STRAFE
    hybrid_touch_down = false;
#endif
    if (!was_down) return HYBRID_TOUCH_ACTION_NONE;
    was_down = false;

    bool tap = max_motion <= HYBRID_TAP_MOVE_PX
        && (uint32_t)(now - down_time_us) <= HYBRID_TAP_MAX_US;
    if (!tap) {
        first_tap_valid = false;
        return HYBRID_TOUCH_ACTION_NONE;
    }

    hybrid_touch_action_t action = ClassifyHybridDoubleTap(anchor_x, anchor_y);

    if (first_tap_valid
        && (uint32_t)(now - first_tap_time_us) <= HYBRID_DOUBLE_TAP_US
        && abs(anchor_x - first_tap_x) <= HYBRID_DOUBLE_TAP_DISTANCE_PX
        && abs(anchor_y - first_tap_y) <= HYBRID_DOUBLE_TAP_DISTANCE_PX
        && action == first_tap_action) {
        first_tap_valid = false;
        return action;
    }

    first_tap_valid = true;
    first_tap_time_us = now;
    first_tap_x = anchor_x;
    first_tap_y = anchor_y;
    first_tap_action = action;
    return HYBRID_TOUCH_ACTION_NONE;
}

#if DOOM_ROLL_STRAFE
static int MaxAbsAxisDelta(const qmi8658_accel_raw_t *a,
                           const qmi8658_accel_raw_t *b)
{
    int dx = abs((int)a->x - b->x);
    int dy = abs((int)a->y - b->y);
    int dz = abs((int)a->z - b->z);
    int maximum = dx > dy ? dx : dy;
    return maximum > dz ? maximum : dz;
}

static void PollHybridMotion(bool active, bool touch_down)
{
    static bool was_active;
    static bool reference_valid;
    static uint8_t calibration_samples;
    static int32_t calibration_sum_y;
    static int32_t calibration_sum_z;
    static int16_t reference_y;
    static int16_t reference_z;
    static qmi8658_accel_raw_t calibration_previous;
    static qmi8658_accel_raw_t calibration_origin;
    static int8_t strafe_state;
    static int8_t strafe_candidate;
    static uint8_t strafe_candidate_tics;
    qmi8658_accel_raw_t raw;

    if (!active || !hybrid_imu_available) {
        hybrid_motion_strafe = 0;
        was_active = false;
        reference_valid = false;
        calibration_samples = 0;
        strafe_state = 0;
        return;
    }

    if (!was_active) {
        was_active = true;
        calibration_samples = 0;
        calibration_sum_y = 0;
        calibration_sum_z = 0;
        reference_valid = false;
        strafe_state = 0;
        strafe_candidate = 0;
        strafe_candidate_tics = 0;
        hybrid_motion_strafe = 0;
    }

    if (!qmi8658_read_accel(&raw)) {
        hybrid_motion_strafe = 0;
        was_active = false;
        return;
    }

    if (!touch_down) {
        // A released touchscreen always disables strafe. Use that safe period
        // to learn the current comfortable grip, but only after a genuinely
        // settled half-second. The last completed neutral remains available if
        // the next touch begins before another window completes.
        hybrid_motion_strafe = 0;
        strafe_state = 0;
        strafe_candidate = 0;
        strafe_candidate_tics = 0;

        if (calibration_samples == 0) {
            calibration_origin = raw;
        } else if (MaxAbsAxisDelta(&raw, &calibration_previous)
                       > MOTION_CALIBRATION_STABLE_RAW
                   || MaxAbsAxisDelta(&raw, &calibration_origin)
                          > MOTION_CALIBRATION_SPAN_RAW) {
            calibration_samples = 0;
            calibration_sum_y = 0;
            calibration_sum_z = 0;
            calibration_origin = raw;
        }

        calibration_sum_y += raw.y;
        calibration_sum_z += raw.z;
        calibration_previous = raw;
        calibration_samples++;
        if (calibration_samples == MOTION_CALIBRATION_SAMPLES) {
            reference_y = (int16_t)(calibration_sum_y / MOTION_CALIBRATION_SAMPLES);
            reference_z = (int16_t)(calibration_sum_z / MOTION_CALIBRATION_SAMPLES);
            reference_valid = true;
            calibration_samples = 0;
            calibration_sum_y = 0;
            calibration_sum_z = 0;
        }
        return;
    }

    // Never continue a partial neutral-calibration window through active play.
    calibration_samples = 0;
    calibration_sum_y = 0;
    calibration_sum_z = 0;
    if (!reference_valid) {
        hybrid_motion_strafe = 0;
        return;
    }

    // Compare the signed Y/Z cross product with the dot product. This measures
    // roll relative to the calibrated grip without assuming the device was
    // held flat or that the projected gravity vector has exactly 1g length.
    // The QMI8658's hardware low-pass plus a two-tic entry confirmation avoids
    // touch tremor without adding the delayed "watery" software low-pass used
    // by the rejected pitch experiment. Touch gating guarantees that a device
    // resting outside neutral cannot move the player on its own.
    int64_t cross_q10 = (((int64_t)reference_z * raw.y)
                       - ((int64_t)reference_y * raw.z)) * 1024;
    int64_t dot = ((int64_t)reference_y * raw.y)
                + ((int64_t)reference_z * raw.z);
    cross_q10 *= DOOM_MOTION_ROLL_SIGN;

    int8_t requested_strafe = 0;
    if (dot > 0) {
        int64_t start = dot * MOTION_STRAFE_START_TAN_Q10;
        if (cross_q10 >= start) requested_strafe = 1;
        else if (cross_q10 <= -start) requested_strafe = -1;
    }

    if (strafe_state == 0) {
        if (requested_strafe == 0) {
            strafe_candidate = 0;
            strafe_candidate_tics = 0;
        } else if (requested_strafe != strafe_candidate) {
            strafe_candidate = requested_strafe;
            strafe_candidate_tics = 1;
        } else if (++strafe_candidate_tics >= MOTION_STRAFE_CONFIRM_TICS) {
            strafe_state = strafe_candidate;
            strafe_candidate = 0;
            strafe_candidate_tics = 0;
        }
    } else {
        int64_t stop = dot > 0 ? dot * MOTION_STRAFE_STOP_TAN_Q10 : 0;
        bool returned_to_neutral = dot <= 0
            || (cross_q10 > -stop && cross_q10 < stop);
        bool crossed_center = (strafe_state > 0 && cross_q10 <= 0)
                           || (strafe_state < 0 && cross_q10 >= 0);
        bool crossed_to_opposite = requested_strafe == -strafe_state;
        if (returned_to_neutral || crossed_center || crossed_to_opposite) {
            strafe_state = 0;
            strafe_candidate = crossed_to_opposite ? requested_strafe : 0;
            strafe_candidate_tics = crossed_to_opposite ? 1 : 0;
        }
    }

    if (strafe_state == 0 || dot <= 0) {
        hybrid_motion_strafe = 0;
        return;
    }

    int64_t magnitude = cross_q10 < 0 ? -cross_q10 : cross_q10;
    int64_t start = dot * MOTION_STRAFE_START_TAN_Q10;
    int64_t full = dot * MOTION_STRAFE_FULL_TAN_Q10;
    int speed = MOTION_STRAFE_MIN;
    if (magnitude > start) {
        int64_t progress = magnitude - start;
        int64_t range = full - start;
        if (progress > range) progress = range;
        speed += (int)(((int64_t)(MOTION_STRAFE_MAX - MOTION_STRAFE_MIN)
                          * progress) / range);
    }
    hybrid_motion_strafe = strafe_state * speed;
}
#endif
#endif

#define DOUBLE_PRESS_WINDOW_US (400 * 1000)

// Returns 0=none, 1=single, 2=double, 3=long. Verbatim port of the
// calibration firmware's poll_pwr_button() (main.c) - already validated.
static int PollPwrButton(pwr_button_event_t ev)
{
    static absolute_time_t last_short_press;
    static bool awaiting_double = false;

    if (ev & PWR_BUTTON_LONG_PRESS) {
        // The AXP2101 already handles the actual power-toggle in hardware
        // for a long press - nothing for us to do with this event.
        awaiting_double = false;
        return 3;
    }

    if (ev & PWR_BUTTON_SHORT_PRESS) {
        if (awaiting_double &&
            absolute_time_diff_us(last_short_press, get_absolute_time()) < DOUBLE_PRESS_WINDOW_US) {
            awaiting_double = false;
            return 2;
        }
        awaiting_double = true;
        last_short_press = get_absolute_time();
        return 0;
    }

    if (awaiting_double &&
        absolute_time_diff_us(last_short_press, get_absolute_time()) > DOUBLE_PRESS_WINDOW_US) {
        awaiting_double = false;
        return 1;
    }
    return 0;
}

static void PostKeyEvent(evtype_t type, key_type_t key) {
    event_t event = { .type = type, .data1 = key, .data2 = 0, .data3 = 0 };
    D_PostEvent(&event);
}

// A momentary chip-reported press (unlike a held GPIO level) can't be
// expressed as a single keydown - Doom samples gamekeydown[] once per tic
// in G_BuildTiccmd, so a keydown immediately followed by a keyup within the
// same tic's event queue would never be observed as "pressed" at all. Hold
// it down for exactly one full tic instead: post keydown now, remember to
// post keyup on the *next* poll call (one tic later). Tracks which key was
// actually sent (not just whether one is pending), since PWR sends a
// different key depending on menuactive - see PollHardwareControls().
static void PulseKey(key_type_t *pending_key)
{
    if (*pending_key) {
        PostKeyEvent(ev_keyup, *pending_key);
        *pending_key = 0;
    }
}

// Called once per tic (from I_GetEvent(), gated on I_InitGraphics() having
// completed - see I_StartTic() in i_video.c). Lazily initializes the touch
// controller on first call rather than in I_InputInit(): that runs at
// I_Init() time (bootlog checkpoint 3), long before I_InitGraphics() has
// called DEV_Module_Init() to bring up the shared I2C bus - too early.
static void PollHardwareControls(void)
{
    static bool ready = false;
    if (!ready) {
        FT3168_Init(FT3168_Point_Mode);
#if DOOM_HYBRID_CONTROLS
#if DOOM_ROLL_STRAFE
        hybrid_imu_available = qmi8658_init_accelerometer();
#endif
        pwr_button_enable_edges();
#endif
        ready = true;
    }

    // menuactive (m_menu.c) has no header declaration - it is a plain global,
    // not part of the public menu API. Motion is never allowed to navigate
    // menus: their proven swipe controls remain unchanged.
    extern boolean menuactive;
#if DOOM_HYBRID_CONTROLS
    bool hybrid_game = !menuactive && gamestate == GS_LEVEL;
#else
    bool hybrid_game = false;
#endif

    static key_type_t touch_pending_key = 0;
    static key_type_t pwr_pending_key = 0;
    PulseKey(&touch_pending_key);
    PulseKey(&pwr_pending_key);

    // The FT3168 can jitter between adjacent directions. Posting every sample
    // change produces an
    // alternating keyup/keydown burst (and used to redraw the bootlog for
    // every transition).  Require a new non-zero zone to be observed on two
    // consecutive tics before committing it.  A release is still immediate
    // so Doom can never be left believing a movement key is held.
    static key_type_t held_touch_key = 0;
    static key_type_t candidate_touch_key = 0;
    static uint8_t candidate_touch_tics = 0;

    if (hybrid_game) {
        if (held_touch_key) PostKeyEvent(ev_keyup, held_touch_key);
        held_touch_key = 0;
        candidate_touch_key = 0;
        candidate_touch_tics = 0;

#if DOOM_HYBRID_CONTROLS
        if (hybrid_touch_strafe_tics > 0
            && --hybrid_touch_strafe_tics == 0) {
            hybrid_touch_strafe = 0;
        }

        hybrid_touch_action_t touch_action = PollHybridTouch(true);
        if (touch_action == HYBRID_TOUCH_ACTION_USE) {
            PostKeyEvent(ev_keydown, key_use);
            touch_pending_key = key_use;
        } else if (touch_action == HYBRID_TOUCH_ACTION_STRAFE_LEFT) {
            hybrid_touch_strafe = -HYBRID_TOUCH_STRAFE_SPEED;
            hybrid_touch_strafe_tics = HYBRID_TOUCH_STRAFE_TICS;
        } else if (touch_action == HYBRID_TOUCH_ACTION_STRAFE_RIGHT) {
            hybrid_touch_strafe = HYBRID_TOUCH_STRAFE_SPEED;
            hybrid_touch_strafe_tics = HYBRID_TOUCH_STRAFE_TICS;
        }
#if DOOM_ROLL_STRAFE
        PollHybridMotion(true, hybrid_touch_down);
#endif
#endif
    } else {
#if DOOM_HYBRID_CONTROLS
        PollHybridTouch(false);
#if DOOM_ROLL_STRAFE
        PollHybridMotion(false, false);
#endif
#endif
        key_type_t touch_key = PollTouchMovementKey();

        if (touch_key == held_touch_key) {
            candidate_touch_key = 0;
            candidate_touch_tics = 0;
        } else if (touch_key == 0) {
            if (held_touch_key) PostKeyEvent(ev_keyup, held_touch_key);
            held_touch_key = 0;
            candidate_touch_key = 0;
            candidate_touch_tics = 0;
        } else {
            if (touch_key != candidate_touch_key) {
                candidate_touch_key = touch_key;
                candidate_touch_tics = 1;
            } else if (++candidate_touch_tics >= 2) {
                if (held_touch_key) PostKeyEvent(ev_keyup, held_touch_key);
                PostKeyEvent(ev_keydown, touch_key);
                held_touch_key = touch_key;
                candidate_touch_key = 0;
                candidate_touch_tics = 0;
            }
        }
    }

    pwr_button_event_t pwr_events = pwr_button_poll();
#if DOOM_HYBRID_CONTROLS
    if (hybrid_game) {
        if (pwr_events & PWR_BUTTON_PRESS_EDGE) {
            PostKeyEvent(ev_keydown, key_fire);
            pwr_pending_key = key_fire;
        }
        // Release and long-press events are intentionally ignored in-level.
        // PWR is the physical power control, so gameplay promises taps only.
        return;
    }
#endif

    int pwr = PollPwrButton(pwr_events);
    if (pwr == 1) {
        // Single-press: "confirm/select" in the menu (key_menu_forward,
        // vanilla default KEY_ENTER - menu navigation doesn't listen for
        // key_fire at all, so without this branch you could navigate with
        // touch but never actually select "New Game". See DECISIONS.md
        // 2026-08-16 (cont'd)), otherwise fire.
        key_type_t k = menuactive ? key_menu_forward : key_fire;
        PostKeyEvent(ev_keydown, k);
        pwr_pending_key = k;
        bootlog_print(menuactive ? "IN: PWR single (menu select)" : "IN: PWR single (fire)");
    } else if (pwr == 2) {
        // Double-press: "back" in the menu, otherwise use.
        key_type_t k = menuactive ? key_menu_back : key_use;
        PostKeyEvent(ev_keydown, k);
        pwr_pending_key = k;
        bootlog_print(menuactive ? "IN: PWR double (menu back)" : "IN: PWR double (use)");
    }
}

void I_ApplyHardwareTiccmd(ticcmd_t *cmd)
{
#if DOOM_HYBRID_CONTROLS
    if (cmd == NULL) return;

    int forward = (int)cmd->forwardmove + hybrid_forward;
    if (forward > 50) forward = 50;
    if (forward < -50) forward = -50;
    cmd->forwardmove = (signed char)forward;

    int strafe = (int)cmd->sidemove + hybrid_touch_strafe;
#if DOOM_ROLL_STRAFE
    strafe += hybrid_motion_strafe;
#endif
    if (strafe > 40) strafe = 40;
    if (strafe < -40) strafe = -40;
    cmd->sidemove = (signed char)strafe;

    int turn = (int)cmd->angleturn + hybrid_turn;
    if (turn > INT16_MAX) turn = INT16_MAX;
    if (turn < INT16_MIN) turn = INT16_MIN;
    cmd->angleturn = (short)turn;
#else
    (void)cmd;
#endif
}

void I_InputInit(void) {
#if PICO_NO_HARDWARE
    platform_key_down = pico_key_down;
    platform_key_up = pico_key_up;
    platform_quit = pico_quit;
#elif USB_SUPPORT
    tusb_init();
    irq_set_priority(USBCTRL_IRQ, 0xc0);
#endif
}

void I_GetEvent() {
#if USB_SUPPORT
    tuh_task();
#endif
#if PICO_ON_DEVICE
    PollHardwareControls();
#endif
    return I_GetEventTimeout(50);
}

void I_GetEventTimeout(int key_timeout) {
#if PICO_ON_DEVICE && !NO_USE_UART
    if (uart_is_readable(uart_default)) {
        char c = uart_getc(uart_default);
        if (c == 26 && uart_is_readable_within_us(uart_default, key_timeout)) {
            c = uart_getc(uart_default);
            static int modifiers = 0;
            switch (c) {
                case 0:
                    if (uart_is_readable_within_us(uart_default, key_timeout)) {
                        uint scancode = (uint8_t) uart_getc(uart_default);
                        if (scancode == SDL_SCANCODE_LSHIFT || scancode == SDL_SCANCODE_RSHIFT) {
                            modifiers |= WITH_SHIFT;
                        }
                        pico_key_down(scancode, 0, modifiers);
                    }
                    return;
                case 1:
                    if (uart_is_readable_within_us(uart_default, key_timeout)) {
                        uint scancode = (uint8_t) uart_getc(uart_default);
                        if (scancode == SDL_SCANCODE_LSHIFT || scancode == SDL_SCANCODE_RSHIFT) {
                            modifiers &= ~WITH_SHIFT;
                        }
                        pico_key_up(scancode, 0, modifiers);
                    }
                    return;
                case 2:
                case 3:
                case 5:
                    if (uart_is_readable_within_us(uart_default, key_timeout)) {
                        uint __unused scancode = (uint8_t) uart_getc(uart_default);
                    }
                    return;
                case 4:
                    if (uart_is_readable_within_us(uart_default, key_timeout)) {
                        uint __unused scancode = (uint8_t) uart_getc(uart_default);
                    }
                    if (uart_is_readable_within_us(uart_default, key_timeout)) {
                        uint __unused scancode = (uint8_t) uart_getc(uart_default);
                    }
                    return;
            }
        }
    }
#endif
}

#if USB_SUPPORT

#define MAX_REPORT  4
#define debug_printf(fmt,...) ((void)0)

// Each HID instance can has multiple reports
static struct
{
    uint8_t report_count;
    tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    debug_printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

    // Interface protocol (hid_interface_protocol_enum_t)
    const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    debug_printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);
//    printf("%d USB: device %d connected, protocol %s\n", time_us_32() - t0 , dev_addr, protocol_str[itf_protocol]);

    // By default host stack will use activate boot protocol on supported interface.
    // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
    if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
    {
        hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
        debug_printf("HID has %u reports \r\n", hid_info[instance].report_count);
    }

    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        debug_printf("Error: cannot request to receive report\r\n");
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    debug_printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
    printf("USB: device %d disconnected\n", dev_addr);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    switch (itf_protocol)
    {
        case HID_ITF_PROTOCOL_KEYBOARD:
            TU_LOG2("HID receive boot keyboard report\r\n");
            process_kbd_report( (hid_keyboard_report_t const*) report );
            break;

#if !NO_USE_MOUSE
            case HID_ITF_PROTOCOL_MOUSE:
      TU_LOG2("HID receive boot mouse report\r\n");
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;
#endif

        default:
            // Generic report requires matching ReportID and contents with previous parsed report info
            process_generic_report(dev_addr, instance, report, len);
            break;
    }

    // continue to request to receive report
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        debug_printf("Error: cannot request to receive report\r\n");
    }
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
    for(uint8_t i=0; i<6; i++)
    {
        if (report->keycode[i] == keycode)  return true;
    }

    return false;
}

static void check_mod(int mod, int prev_mod, int mask, int scancode) {
    if ((mod^prev_mod)&mask) {
        if (mod & mask)
            pico_key_down(scancode, 0, 0);
        else
            pico_key_up(scancode, 0, 0);
    }
}

static void process_kbd_report(hid_keyboard_report_t const *report)
{
    static hid_keyboard_report_t prev_report = { 0, 0, {0} }; // previous report to check key released

    //------------- example code ignore control (non-printable) key affects -------------//
    for(uint8_t i=0; i<6; i++)
    {
        if ( report->keycode[i] )
        {
            if ( find_key_in_report(&prev_report, report->keycode[i]) )
            {
                // exist in previous report means the current key is holding
            }else
            {
                // not existed in previous report means the current key is pressed
                bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                pico_key_down(report->keycode[i], 0, is_shift ? WITH_SHIFT : 0);
            }
        }
        // Check for key depresses (i.e. was present in prev report but not here)
        if (prev_report.keycode[i]) {
            // If not present in the current report then depressed
            if (!find_key_in_report(report, prev_report.keycode[i]))
            {
                bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                pico_key_up(prev_report.keycode[i], 0, is_shift ? WITH_SHIFT : 0);
            }
        }
    }
    // synthesize events for modifier keys
    static const uint8_t mods[] = {
            KEYBOARD_MODIFIER_LEFTCTRL, SDL_SCANCODE_LCTRL,
            KEYBOARD_MODIFIER_RIGHTCTRL, SDL_SCANCODE_RCTRL,
            KEYBOARD_MODIFIER_LEFTALT, SDL_SCANCODE_LALT,
            KEYBOARD_MODIFIER_RIGHTALT, SDL_SCANCODE_RALT,
            KEYBOARD_MODIFIER_LEFTSHIFT, SDL_SCANCODE_LSHIFT,
            KEYBOARD_MODIFIER_RIGHTSHIFT, SDL_SCANCODE_RSHIFT,
    };
    for(int i=0;i<count_of(mods); i+= 2) {
        check_mod(report->modifier, prev_report.modifier, mods[i], mods[i+1]);
    }
    prev_report = *report;
}

//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

#if !NO_USE_MOUSE
static void process_mouse_report(hid_mouse_report_t const * report)
{
    static hid_mouse_report_t prev_report = { 0 };

    uint8_t button_changed_mask = report->buttons ^ prev_report.buttons;
    if ( button_changed_mask & report->buttons)
    {
        debug_printf(" %c%c%c ",
                     report->buttons & MOUSE_BUTTON_LEFT   ? 'L' : '-',
                     report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
                     report->buttons & MOUSE_BUTTON_RIGHT  ? 'R' : '-');
    }

//    cursor_movement(report->x, report->y, report->wheel);
}
#endif

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void) dev_addr;

    uint8_t const rpt_count = hid_info[instance].report_count;
    tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
    tuh_hid_report_info_t* rpt_info = NULL;

    if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
    {
        // Simple report without report ID as 1st byte
        rpt_info = &rpt_info_arr[0];
    }else
    {
        // Composite report, 1st byte is report ID, data starts from 2nd byte
        uint8_t const rpt_id = report[0];

        // Find report id in the arrray
        for(uint8_t i=0; i<rpt_count; i++)
        {
            if (rpt_id == rpt_info_arr[i].report_id )
            {
                rpt_info = &rpt_info_arr[i];
                break;
            }
        }

        report++;
        len--;
    }

    if (!rpt_info)
    {
        debug_printf("Couldn't find the report info for this report !\r\n");
        return;
    }

    // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
    // - Keyboard                     : Desktop, Keyboard
    // - Mouse                        : Desktop, Mouse
    // - Gamepad                      : Desktop, Gamepad
    // - Consumer Control (Media Key) : Consumer, Consumer Control
    // - System Control (Power key)   : Desktop, System Control
    // - Generic (vendor)             : 0xFFxx, xx
    if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
    {
        switch (rpt_info->usage)
        {
            case HID_USAGE_DESKTOP_KEYBOARD:
                TU_LOG1("HID receive keyboard report\r\n");
                // Assume keyboard follow boot report layout
                process_kbd_report( (hid_keyboard_report_t const*) report );
                break;

#if !NO_USE_MOUSE
                case HID_USAGE_DESKTOP_MOUSE:
        TU_LOG1("HID receive mouse report\r\n");
        // Assume mouse follow boot report layout
        process_mouse_report( (hid_mouse_report_t const*) report );
      break;
#endif

            default: break;
        }
    }
}

#endif
