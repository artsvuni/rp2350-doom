#ifndef _BOOTLOG_H_
#define _BOOTLOG_H_

// TEMPORARY diagnostic aid: prints short status lines directly to the
// AMOLED panel as boot proceeds, so progress is visible without a serial
// connection. See doom/docs/DECISIONS.md.
void bootlog_init(void);
void bootlog_print(const char *msg);

// Clear the complete panel through one streamed transaction while reusing the
// small bootlog strip buffer. Intended for standalone diagnostics that never
// hand the panel to Doom's normal renderer.
void bootlog_clear_panel(void);

// Stop normal diagnostic rendering once game graphics owns the panel.
// A fresh bootlog_init() call enables it again for early failures and OOM.
void bootlog_disable(void);

// Every bootlog_print() call counts (regardless of message content), but
// nothing actually renders until the count reaches `n` - use this to skip
// past already-confirmed-reached checkpoints so a crash shortly after a
// known point doesn't scroll its own new checkpoints off-screen before we
// can read them (no scrollback on this tiny diagnostic display).
void bootlog_skip_until(int n);

#endif
