# Start here

Before doing anything in this repo, read:

1. `CLAUDE.md` (this directory) — project goal, current status, immediate
   next steps.
2. `docs/DECISIONS.md` — full dated history: hardware gotchas, every bug
   root-caused so far (with the actual bisection reasoning, not just the
   fix), and the "Open questions" section at the bottom, which is the
   most current list of what's unresolved.

Also worth a skim: `../CLAUDE.md` (one level up) — this folder is one of
several projects sharing a hardware workspace; `../mp3player/` has proven
driver code for this exact board that this project reuses/adapts.

As of 2026-08-16: the game runs and is playable on hardware. The
immediate open bug is a freeze during actual gameplay after a burst of
touch-input events — see `docs/DECISIONS.md`'s most recent dated entry
and "Open questions" for exactly what's known and suspected. Don't
re-diagnose the two already-fixed freezes (zone corruption from a stray
`calloc()`, zone exhaustion from re-enabling the display) - they're
resolved; read the DECISIONS.md entries if curious about how, not to
redo the work.

Flashing/testing requires physical access to the board (BOOTSEL mode,
photographing the AMOLED screen) - if you can't do that, say so rather
than guessing at hardware behavior.

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
