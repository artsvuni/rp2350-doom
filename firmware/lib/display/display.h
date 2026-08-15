#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdbool.h>

void display_init(void);

// Shows large centered text: "PAUSE" while a track is actively playing
// (pressing the button would pause it), "PLAY" otherwise (idle, paused,
// or a track just finished - pressing the button would start/resume it).
void display_show_state(bool playing);

// Shows two lines of text, vertically stacked and centered.
void display_show_lines(const char *line1, const char *line2);

#endif
