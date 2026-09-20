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

## 2026-09-20 — the gate printed GATE PASSED with 9 of its 10 stages never run

What broke:        A `git push` re-ran the full tier and printed `all 10 stage(s) passed in 0s`
                   while only `tree` had really run. `format` died with
                   `tools/ci.sh: sources: unbound variable` (stage_format, on a touched set with no C++ file) — `set -u` plus
                   `local -a sources` declared and never filled — and bash unwound out of the
                   stage function *and* out of the dispatch loop, so the run fell through to the
                   end, where the verdict is printed unconditionally. The trigger is the common
                   case, not an exotic one: any commit whose touched set holds no C++ file (a
                   docs, script or record change) takes that branch.
Check added:       `tools/ci.sh`: the arrays are declared `=()`, a stage that returns non-zero
                   without reporting a verdict fails the run, and the verdict is derived from
                   what RAN (`FAILED: N of M stage(s) did not run`). Carried into this repo as
                   `tools/kit-probes/gate-stage-guards.sh`, the kit's probe for exactly this fix
                   (card `t_0cc793fb`; the incident, the measurement and the rationale are in the
                   kit's INCIDENTS.md), and run by the `kitprobes` stage on every push.
Why it must stay:  Contract rule 1 is "a stage that did not run must never read as green". The
                   `BLOCK` lines cover the stages behind a *reported* failure; these guards cover
                   a stage that dies without reporting anything at all. Without them a green
                   verdict can sit over a gate that checked almost nothing — the failure the kit
                   exists to remove, and the one that hid a broken tidy baseline in four repos.

## 2026-09-20 — `tidy` is wired, against the analyzer-only rule set (the earlier decline, re-measured)

What broke:        This repo ran no clang-tidy: `tidy` was implemented but absent from
                   `CI_DEFAULT_STAGES`, from `.githooks/pre-push` and from `--help`, on the
                   recorded grounds that the rule set it had been measured with — the sibling
                   repo's then "house" `.clang-tidy`, `-*,bugprone-*,performance-*,
                   readability-*,modernize-*,portability-*` — produced 451 findings and 387 s
                   on this tree: style, not defects. That measurement was sound for THAT
                   config, and it is still the number for it. What it never settled is the
                   config that runs when there is no `.clang-tidy` at all: clang-tidy 19 then
                   analyses `clang-diagnostic-*,clang-analyzer-*` — the compiler diagnostics
                   plus the Clang Static Analyzer, a bug-finding set with no style opinion in
                   it — and on this tree that set reported exactly ONE finding, and it was
                   real (the entry below). "No `.clang-tidy`" was never what stopped the stage
                   from running; it was the reason the stage and its rule set were
                   undocumented.
Check added:       `.clang-tidy` pins the bug-finding set explicitly (`Checks:
                   'clang-diagnostic-*,clang-analyzer-*'`, plus a `HeaderFilterRegex` scoped
                   to this repo's own tree), `tidy` is in `CI_DEFAULT_STAGES` and in
                   `.githooks/pre-push`, and every surface that carried the old decision —
                   `CODING_STANDARDS.md`, `.ci.env.example`, the adaptation notes and
                   `--help` in `tools/ci.sh`, and this file — carries the new one. No baseline
                   file, on purpose: with this set the tree is clean, and a baseline tolerates
                   findings you inherited, it does not bless a rule set. The gate passes with
                   `tidy` in it (`tools/ci.sh --require-clean`, 9 stages).
Why it must stay:  Two failure modes, and this configuration avoids both. Wiring tidy with
                   the style families is a gate that is red the day it is wired — 451 findings
                   nobody agreed to tolerate — and an always-red stage gets `--no-verify`.
                   Wiring it with an exclusion list or a baseline file is a stage that
                   certifies nothing while printing green. The analyzer set is reachable at
                   zero findings with neither: it needs no exclusions, and on its first honest
                   run it found a real defect, so the only way to satisfy it is to fix the
                   code. Deleting `tidy` from the list, or deleting `.clang-tidy` (which
                   silently restores whatever the installed clang-tidy's default happens to
                   be), puts the repo back to "the analyzer never runs on a push".
                   Measured 2026-09-20, this tree, 17 translation units, 4 cores: 0 findings
                   with this set in 104-201 s (cache-dependent: 104 s warm, 201 s cold right
                   after a fresh build; the whole 9-stage gate ran in 141 s warm); 451
                   findings / 387 s for the stock families; 53
                   findings / 204 s for the suite's value-only set that Computo/JSOM/jsonTools
                   run — which stays an OPEN decision here, with its numbers, not a rejected
                   one (9 of its 53 are the known `bugprone-unchecked-optional-access` false
                   positives on GoogleTest `ASSERT_TRUE(opt.has_value())` guards, 11 are one
                   decision about `MissingKeyBehavior`'s base type). Cost, stated plainly:
                   roughly three times the rest of the gate (~104-201 s against ~37-66 s),
                   paid on push only (the fast tier is still `build tests`) — the cheapest
                   tidy in the suite.
                   Confirmed the same day that the stage catches a NEW finding: a probe header
                   inside `include/permuto/` with an unread store was reported at
                   `include/permuto/probe_tmp.hpp:10:9`, and probe files were removed again.

---

## 2026-09-20 — the example found a failed round trip, printed it, and then reported success

What broke:        `examples/api_example.cpp` verified its own reverse round trip, printed
                   "✗ Round-trip integrity failed!" when the reconstructed context did not
                   match — and then, because the flag it set was never read, printed
                   "All examples completed successfully!" and exited 0 anyway. Proved rather
                   than asserted: HEAD's version with one comparison forced to fail prints the
                   ✗ line AND the success banner and exits 0, while the fixed version prints
                   the ✗ line, says the round trip did not reconstruct the context, and exits
                   1 (both probe builds kept in the evidence bundle). The library was correct
                   in both runs — the happy path's output is unchanged — so the defect was the
                   example teaching readers that a failed invariant is nothing to act on.
                   The `tidy` stage found it on a tree nobody had edited, on the day it was
                   wired: `examples/api_example.cpp:152:13: warning: Value stored to
                   'round_trip_success' is never read [clang-analyzer-deadcode.DeadStores]`.
Check added:       The round-trip result is computed once into `round_trip_ok` and used for
                   BOTH the message and the exit status (`return 1` when it is false), so the
                   example can no longer claim success after failing its own check. The static
                   check behind that shape is the now-wired `tidy` stage (clang-analyzer
                   reports a value stored and never read); the behavioural check is the same
                   code path forced to fail in a throwaway probe build, not an assumption.
Why it must stay:  Deleting the `if (!round_trip_ok)` block restores the original bug exactly:
                   a demo that exits 0 on a broken invariant. Examples are the only
                   documentation a reader can run, so "it detected something and then said the
                   run was fine" is the most expensive kind of wrong answer this repo can ship
                   — and it is exactly the fails-open shape the rest of this tooling exists to
                   remove. The unread store that produced it is what the analyzer check
                   reports, on any file, not just this one.

---

## 2026-09-20 — the tidy baseline could never match, so the tidy stage could not pass

What broke:        `tools/ci.sh`'s tidy stage compared the baseline one-sided: the log side
                   had `:line:column` stripped (`sed 's/:[0-9]*:[0-9]*:/:/'`) while
                   `$CI_TIDY_BASELINE` was read raw — and clang-tidy names the file with the
                   ABSOLUTE path CMake wrote into the compile database. A baseline captured
                   in one checkout (with the recipe `.ci.env.example` documented: `tools/ci.sh
                   tidy && grep -E "warning:|error:" .ci-logs/tidy.log | sort -u >
                   .ci/tidy-baseline.txt`) therefore matched nothing once the same commit was
                   checked out at another path — the nightly clean checkout, a colleague's
                   machine — and every baselined finding read as new.
Check added:       `tidy_key()` in `tools/ci.sh` normalises BOTH operands (the repo-root
                   prefix and `:line:column` are stripped) and both are de-duplicated before
                   `comm -13`; and the baseline is produced by the gate itself,
                   `tools/ci.sh --write-tidy-baseline`, which runs the real build+tidy stages
                   as a child and writes their log through that same function, so the
                   documented way to accept findings cannot drift from the way they are
                   compared. `.ci.env.example` documents the flag instead of the raw capture.
Why it must stay:  This repo wires tidy now (see the entries above), so the defect is live
                   rather than latent here: with a baseline present, a one-sided comparison
                   would have made the stage red forever on a tree nobody edited, and an
                   always-red stage gets `--no-verify`, which is worse than no stage. The
                   port is not a licence to accept findings: no
                   `.ci/tidy-baseline.txt` is committed here, so the stage still demands a
                   clean run.

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

**SUPERSEDED later the same day — tidy is now WIRED, against the analyzer-only set (see the
two entries above). Read this entry for the reasoning about STYLE FAMILIES, which is why the
shipped rule set is not one; do not read the numbers below as describing what this repo's
`tidy` runs now, and note that the config they were measured with (the stock families) was
replaced in Computo by `aff17cf` the same day, so it is not what any repo in the suite runs
any more. The set that ships is `clang-diagnostic-*` + `clang-analyzer-*`: 0 findings /
104-201 s here.**

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

