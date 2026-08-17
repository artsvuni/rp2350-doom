# Start here

Before doing anything in this repo, read:

1. `CLAUDE.md` (this directory) — project goal, current status, immediate
   next steps.
2. `docs/DECISIONS.md` — full dated history: hardware gotchas, every bug
   root-caused so far (with the actual bisection reasoning, not just the
   fix), and the "Open questions" section at the bottom, which is the
   most current list of what's unresolved.

Also worth a skim: the sibling
`../rp2350-touch-amoled-1.8-knowledge-base/` repository contains reusable
hardware/development findings, and `../mp3player/` has proven driver code for
this exact board that this project reuses/adapts.

As of 2026-08-17: the game runs and is playable on hardware, and the latest
effects-only run progressed into E1M2 without freezing. The immediate priority
is a measured performance pass toward a larger view, followed by motion-control
experiments; see `docs/ROADMAP.md`. The earlier combat freeze remains a watch
condition, not the default explanation for every problem. Don't re-diagnose the
two already-fixed freezes (zone corruption from a stray `calloc()`, zone
exhaustion from re-enabling the display)—they're resolved; read the dated
`docs/DECISIONS.md` entries for the evidence.

When compatible USB firmware is running, `picotool -f --ser ...` can reboot the
board into ROM BOOTSEL, flash/verify it, and return to the application without a
physical BOOT press. Manual BOOTSEL remains the recovery fallback. A successful
tool write does not prove display, audio, touch, motion, or gameplay behaviour;
obtain physical confirmation rather than guessing.

## Git workflow

- Make frequent local commits after meaningful file changes so experiments
  and working milestones are easy to recover.
- Never push to GitHub or another remote unless Alexander explicitly asks for
  a push. A request to build or flash the device is not permission to push.
- Before a requested push, inspect every commit ahead of `origin/main` and
  report them. By default, squash the unpublished work into one coherent
  milestone commit unless Alexander asks to preserve separate commits.
- Never rewrite commits that have already been pushed unless Alexander
  explicitly requests a history rewrite and acknowledges the force-push risk.
- Treat `README.md` as publication-level documentation, not a per-commit log.
  Update it only for a genuinely major milestone or during the final cleanup
  immediately before a requested GitHub push. Record routine local changes in
  `.Codex/project-log.md`, project context, TODOs, and technical decision docs
  instead.
