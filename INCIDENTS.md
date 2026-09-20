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

## 2026-09-20 — a third copy of the version number existed and nothing compared it

What broke:        `write_basic_package_version_file(PermutoConfigVersion.cmake VERSION
                   ${PACKAGE_VERSION})` at the bottom of CMakeLists.txt reads like a bug
                   (`PACKAGE_VERSION` is never set in that file) but is not one: CMake's
                   module falls back to `PROJECT_VERSION`, so the generated
                   `build/PermutoConfigVersion.cmake` really does contain
                   `set(PACKAGE_VERSION "1.0.0")`. It is, though, a THIRD copy of the
                   number — the one a downstream `find_package(Permuto 1.0.0)` is answered
                   by — and nothing in the gate compared it to the other two.
Check added:       stage_version in `tools/ci.sh`: if CMakeLists.txt calls
                   `write_basic_package_version_file` at all, the top level of
                   `$CI_BUILD_DIR` must hold at least one `*ConfigVersion.cmake` and every
                   one of them must carry the same number as `project(VERSION)`. Not
                   finding the file is a FAIL, not a SKIP — "the copy was not checked" is
                   the state this stage exists to rule out. There is deliberately no knob
                   for it (a knob would be a way to leave the copy unchecked), and the
                   search is top-level-only so a FetchContent'd dependency's own
                   ConfigVersion.cmake can never be mistaken for ours.
Why it must stay:  Without the block the gate certifies "one version number" while a second
                   artifact makes the same claim to everyone who consumes the installed
                   package. Proved by desynchronising it on purpose: with
                   `build/PermutoConfigVersion.cmake` hand-edited to 9.9.9,
                   `tools/ci.sh version` fails with "1 generated package-version file(s)
                   disagree with project(VERSION) 1.0.0"; re-running the build stage
                   restores green.

---

## 2026-09-20 — `tidy` is absent from the gate for measured reasons, not by omission

What broke:        Nothing broke. This entry exists because "the stage is missing" and
                   "the stage was deliberately declined" look identical from the outside —
                   the only difference is whether somebody wrote down the measurement.
                   Measured first, with the sibling repo's house `.clang-tidy` (the closest
                   thing to this project's standard that exists): 451 findings over the 17
                   translation units, 387 s of wall clock on 4 cores — about six times this
                   repo's entire gate. The two biggest groups are house-style choices rather
                   than defects (`modernize-use-trailing-return-type` 220,
                   `modernize-use-nodiscard` 136), and the set's most safety-relevant check,
                   `bugprone-unchecked-optional-access` (9 findings), is a false positive in
                   all 9 cases: they are GoogleTest `ASSERT_TRUE(opt.has_value())` guards,
                   which that check cannot see through. The rest is subjective style or a
                   decision about the published API (`performance-enum-size` wants a
                   different underlying type for `MissingKeyBehavior`).
Check added:       The stage stays implemented (`tools/ci.sh tidy` reports the truth on
                   demand) and is deliberately absent from `CI_DEFAULT_STAGES`, from
                   `.githooks/pre-push`, and from the default list `--help` prints — with
                   the measurement recorded in CODING_STANDARDS.md's Tooling status, in
                   `.ci.env.example`, and in the adaptation notes at the top of
                   `tools/ci.sh`, which is where a future reader looks first. There is no
                   baseline file, on purpose: a baseline tolerates findings you inherited,
                   it does not bless a rule set that was never adopted.
Why it must stay:  Two failure modes are prevented at once. Adding tidy with the house rule
                   set makes every push seven times slower for findings nobody agreed to
                   tolerate; adding it with the exclusions that would make today's tree
                   pass is a stage that certifies nothing while printing green — the
                   fails-open shape this kit exists to remove. When someone writes a
                   `.clang-tidy` this repo can live with, wiring it is a one-line change to
                   the stage list, and the gate grows.

---

## 2026-09-20 — CODING_STANDARDS.md required a .clang-format that did not exist, so nothing checked formatting

What broke:        CODING_STANDARDS.md's Style section says "clang-format per the repo's
                   .clang-format" and its Definition of done implies formatting is checked,
                   but the file did not exist: the gate's `format` stage could not enforce
                   anything, and with no configuration clang-format applies its own defaults,
                   which are not this project's style. Measured before the fix, against the
                   suite's house style: all 23 sources drifted — 1162 insertions / 1169
                   deletions. Every source in the repo, which is why the stage had been left
                   out of the stage list with a note instead of being fixed.
Check added:       `.clang-format` (byte-for-byte the suite file: LLVM base, 4-space indent,
                   100 columns — the same file Computo, JSOM and jsonTools use), the whole
                   tree formatted in one mechanical commit, and `format` wired into
                   CI_DEFAULT_STAGES and .githooks/pre-push, so a push whose branch leaves
                   any file it touched unformatted now fails.
                   The mechanical commit was verified to be layout-only rather than assumed
                   to be: the pre-format sources and the formatted sources were built at the
                   SAME path with the SAME flags, and all 18 object files plus libpermuto.a
                   were byte-identical (58/58 tests pass either way). The same path matters —
                   a Google Test object embeds __FILE__/__LINE__, so building the two
                   versions in different directories makes test objects differ for reasons
                   that have nothing to do with the reformat.
Why it must stay:  Removing `.clang-format` or dropping `format` from the stage list puts
                   the repo back where it was: a written standard with nothing behind it, and
                   every file free to drift toward whatever the next editor prefers. The
                   whole-tree reformat is only safe to keep BECAUSE the object-file check
                   says it changed no code — if .clang-format is ever replaced, redo that
                   check instead of trusting the size of the diff.

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

