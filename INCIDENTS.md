# INCIDENTS — every real failure, and the check that now catches it

Newest first. One entry per incident that changed how this repo works.

The rule: **when something breaks, the fix is not done until a check exists that would
have caught it, and the incident is written here next to that check.** A check with no
written rationale looks arbitrary to the next hurried contributor (or agent), and
arbitrary checks get deleted. The rationale is the load-bearing part.

    ## YYYY-MM-DD — <one-line failure>
    What broke:        <the user-visible symptom>
    Check added:       <file> + <gate stage that now catches it>
    Why it must stay:  <why deleting this check re-enables the bug>

---

## 2026-09-20 — the repo's own build tree was committed, so the gate could not check the tree

What broke:        `tools/ci.sh tree` could not pass at HEAD, and not for a reason the
                   gate could fix from its side: 130 of the repo's 169 tracked files WERE
                   the old CMake `build/` tree. With no `build/` rule in `.gitignore` the
                   stage reported `NOT ignored: build`; with one it reported `tracked files
                   matched by .gitignore (stale rules)` for 117 tracked files. The real
                   damage was bigger than the stage: a clone could not be trusted to build
                   from source, and the whole gate had grown three adaptations the kit does
                   not have (build in `build-ci*`, pristine checkout in `ci-build/`) purely
                   so a run would not rewrite committed files and dirty the tree it was
                   certifying.
Check added:       `.gitignore` covers `build/` and `build-*/`, and the same commit
                   untracked the tree (`git rm --cached` on the 130 files, they stay on
                   disk) and wired `tree` into `CI_DEFAULT_STAGES` and into
                   `.githooks/pre-push`. Every push now fails if build output is ever
                   committed again, if a file is neither committed nor ignored, or if the
                   gate's own footprint (build dirs, `.ci-logs/`, `.ci.env`) is not
                   ignored. The hook's hand-rolled "uncommitted changes to tracked files"
                   check was deleted at the same time: `--require-clean` on the tree stage
                   IS that check, and a stage can be run by hand, which the hook could not.
Why it must stay:  Re-committing `build/` brings every symptom back at once — a repo whose
                   build directory is in git cannot prove that its committed source builds,
                   and the gate quietly drifts back to build-dir workarounds. Deleting the
                   `build/` line from `.gitignore` or dropping `tree` from the stage list is
                   enough to re-enable it, and nothing else in the repo would complain:
                   this entry is the only place that says why they are there.

