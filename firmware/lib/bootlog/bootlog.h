#ifndef _BOOTLOG_H_
#define _BOOTLOG_H_

// TEMPORARY diagnostic aid: prints short status lines directly to the
// AMOLED panel as boot proceeds, so progress is visible without a serial
// connection. See doom/docs/DECISIONS.md.
void bootlog_init(void);
void bootlog_print(const char *msg);

// Every bootlog_print() call counts (regardless of message content), but
// nothing actually renders until the count reaches `n` - use this to skip
// past already-confirmed-reached checkpoints so a crash shortly after a
// known point doesn't scroll its own new checkpoints off-screen before we
// can read them (no scrollback on this tiny diagnostic display).
void bootlog_skip_until(int n);

#endif
