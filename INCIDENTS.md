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

## 2026-10-06 — the 1087-line gate became gate.toml, run by kit-ci

What broke:        Not a break — a conversion with a measured cost, written down because the
                   alternative is a reader discovering it. `tools/ci.sh` (1087 lines, thirteen
                   stages, its own summary, its own `--help`) is gone. The gate is now
                   `gate.toml` — twelve stages, tiers `fast`/`full` — run by `kit-ci`, plus the
                   `scripts/*.sh` the policy names, which are the old stage bodies. Three
                   things MOVED rather than disappeared, a fourth lost a switch the engine has
                   no word for, and one did not survive:
                   * `--require-clean` was a FLAG, and kit-ci's vocabulary has no per-run
                     flags. The rule is unchanged and still push-only, but its caller now sets
                     `CI_REQUIRE_CLEAN=1` in the environment and `scripts/tree.sh` reads it. A
                     hand run still works on a dirty tree, exactly as before.
                   * `--write-tidy-baseline` was a MODE of the script. It is now
                     `scripts/write-tidy-baseline.sh`, which runs the real build and tidy stages
                     and captures the same file through the same `tidy_key()`.
                   * NAMING STAGES on the command line (`tools/ci.sh fuzz`) has NO equivalent:
                     kit-ci runs TIERS and has no stage-selection option at all (`--help` lists
                     `--tier`, `--gate`, `--list`, `--graph`, `--strict` — and nothing else).
                     The translation is to run that stage's script directly, which is what
                     gate.toml's own note now says for a long fuzz campaign:
                     `CI_FUZZ_SECONDS=1800 scripts/fuzz.sh`. The scripts take no arguments and
                     read the same environment, so this is the same run the stage would make.
                   * `CI_STRICT_TOOLS` (a missing clang-format/clang-tidy/clang++ FAILS instead
                     of skipping) has NO equivalent either: kit-ci skips by its own rule,
                     `when = "tool:<name>"` in gate.toml, and no key turns a skip into a
                     failure. Its default was 0, so the default behaviour is preserved; a
                     machine that wanted the switch loud loses it.
                   * The `kitprobes` stage did not survive — it is the next entry.
                   Two differences come from the engine and are worth knowing: kit-ci runs
                   EVERY stage and reports every failure, where the old script stopped at the
                   first one; and it prints the first 40 lines of a FAILED stage's output, but
                   nothing from a stage that passed — the old summary echoed each stage's own
                   count line (ctest's "100% tests passed"). Those lines are still written to
                   `.ci-logs/` by the stage scripts.
Check added:       `gate.toml` + `scripts/` (twelve stages, two tiers) and the two hooks, which
                   now NAME a tier instead of repeating a stage list. Every guarantee the old
                   script carried is held verbatim by the stage script that inherited it: the
                   staged-copy format check, the tidy baseline normaliser, the fuzz seed smoke
                   with its non-zero identity-check guard, the version stage's four copies of
                   the number, the compile-database coverage check, the tree footprint audit,
                   and the pristine clean-checkout proof.
Why it must stay:  A policy file that `kit-ci --list` reads is one diff away from saying what
                   the gate runs; the old arrangement kept the stage list in the script, a
                   second copy in the pre-push hook, and nothing compared them except a probe.
                   The three moved things each have exactly one caller now — which is why
                   each is recorded with its name.

---

## 2026-10-06 — the kit probes checked a file that no longer exists

What broke:        Measured, not argued. The kit's five probes are written against the shape of a
                   single bash gate file: each greps it (`stage_format()`, `CI_FAST_STAGES`,
                   `unset GIT_INDEX_FILE`) or SOURCES its environment block. All five were
                   GREEN against `tools/ci.sh` immediately before the conversion (`bash
                   tools/kit-probes/<probe>.sh tools/ci.sh .` exit 0, re-measured today). Run
                   after the conversion against the new entry point `scripts/gate.sh`:
                   `format-checks-staged` GREEN, `git-index-file` GREEN, `gate-stage-guards`
                   RED, `hook-tiers-agree` RED, `tidy-baseline` RED — and both GREENs are
                   accidents of how those two probes SEARCH, not measurements of the new gate.
                   A `kitprobes` stage whose verdict is accidental is the exact failure mode
                   this file exists for, so the stage and the five scripts are deleted rather
                   than kept as decoration.
Check added:       The five CONTRACTS are held in their new homes, and this entry is their index:
                   * `tidy-baseline`        -> `tidy_key()` in `scripts/gate-env.sh`, applied to
                                               both sides in `scripts/tidy.sh` and by
                                               `scripts/write-tidy-baseline.sh`
                   * `format-checks-staged` -> the staged-copy block in `scripts/format.sh`
                   * `git-index-file`       -> `unset GIT_INDEX_FILE` in `scripts/gate.sh` and
                                               `scripts/gate-env.sh` (both entry points, so a
                                               stage that sources neither is still covered)
                   * `hook-tiers-agree`     -> `[tier.fast]`/`[tier.full]` in `gate.toml`, named
                                               by `.githooks/pre-commit` and `.githooks/pre-push`,
                                               printed by `kit-ci --list`
                   * `gate-stage-guards`    -> kit-ci's own runner: it executes each declared
                                               stage and fails any command that does not exit 0,
                                               so a stage can no longer die silently behind a
                                               "GATE PASSED" line.
                   `.ai-dev-starter.json` says the same thing in its own words: the five are now
                   `declined_fixes` — not applicable, because the artifact they guarded (this
                   repo's fork of `templates/cpp/ci.sh`) is gone. That is the same statement
                   docsum's record carries for its own KitCI conversion.
Why it must stay:  The next sync card will see five probes in the kit and no copy here. Without
                   this entry and the record's five `declined_fixes` entries, the choice reads
                   as lag rather than as a decision; with them, the question is answered where
                   the convention says to answer it.

---

## 2026-10-06 — the pre-push hook carried its own copy of the stage list

What broke:        `.githooks/pre-push` named all thirteen stages, and so did `CI_DEFAULT_STAGES` in
                   tools/ci.sh. They agreed here; the drift showed in the siblings, which is why it
                   is worth an entry. Computo's default list had fallen two stages behind its hook
                   (`docs`, `release`), so a hand run and a push ran different gates. FSMTable's
                   pre-push still named the kit's original eleven while its gate defined thirteen, so
                   every push skipped the fuzzer — the stage that repo's own spec requires. A hook
                   that repeats a list is a copy nothing compares.
Check added:       tools/ci.sh declares CI_FAST_STAGES and CI_FULL_STAGES, and both hooks NAME a tier
                   (`tools/ci.sh fast`, `tools/ci.sh --require-clean full`) instead of repeating one.
                   CI_DEFAULT_STAGES IS the full tier, so a hand run and a push run the same thirteen
                   stages and only --require-clean differs. The `kitprobes` stage runs
                   tools/kit-probes/hook-tiers-agree.sh, which fails when a hook names a stage, when a
                   stage the gate defines is in no tier, when the fast tier is not a subset of the
                   full one, or when the lists printed in the header stop matching the variables.
                   `format` joins the fast tier: it is the check that would have caught the
                   unformatted commit FSMTable pushed on the same day.
Why it must stay:  Put the list back in the hook and the next stage added to the gate is run by a
                   hand run and skipped by every push — silently, with GATE PASSED printed about a
                   stage set the push did not run.

---

## 2026-09-20 — the fuzz stage certified a round trip it could not reach

What broke:        The `fuzz` stage was green and its oracle was live — the seed smoke proved that
                   inputs reached the assert — and it could not reach the surface it existed to
                   defend. An independent adversarial verification
                   (`/tmp/permuto-verification.md`) sabotaged `apply_reverse()` to skip every
                   mapping whose context path contains a digit. That sabotage **survived 4.2
                   million executions** (`identity_checks=59709`, all matching, exit 0).
                   Instrumenting the harness showed why: condition C3 requires the context's
                   leaf-path set to EQUAL the template's placeholder-path set, and both documents
                   arrive through one byte stream, so a mutation on one side breaks the equality
                   and the input is skipped — only VALUE mutations survived. 117 of 9,952 corpus
                   inputs ever reached the assertion, and in all 117 the context's key names were
                   byte-identical to the seeds'. The hand-written input `{"x":"${/a1}"}` +
                   `{"a1":1}` finds the same sabotage instantly, so the stage was certifying a
                   surface its input could not touch.
Check added:       a SECOND interpretation of every input (`fuzz/fuzz_permuto.cpp`): the same
                   bytes are read again as a template plus a value pool, the harness collects the
                   paths the template references and BUILDS the context that provides exactly
                   those paths — so the two documents agree by construction and key names, key
                   count and nesting shape mutate freely (RFC 6901 tokens are unescaped while
                   building, so keys holding `/` or `~` are reachable as well). Each reading has
                   its own counter pair and the seed smoke now requires BOTH to be non-zero.
                   Measured after: a 60 s campaign reaches the assertion 27,201 times from the
                   structured reading against 2,040 from the original one, at 11,728 exec/s (was
                   ~19,100 — two parses per input, the honest price). The verifier's four
                   sabotages all die inside the 10 s budget, S1 instantly and from the tracked
                   seeds alone, with a control run of the untouched tree clean. The verifier also
                   found that reverting the empty-token fix left every reproducer clean, so
                   `fuzz/regressions/empty-key-context-side` exists for it now.
Why it must stay:  The same shape as the gate that printed `GATE PASSED` with nine of its ten
                   stages never run: a check whose input space cannot reach the property is
                   indistinguishable from outside from one that can. Deleting the second
                   interpretation re-narrows the oracle SILENTLY, because the first one keeps the
                   counters healthy — which is why the smoke requires both.

## 2026-09-20 — the install tree was never consumable, and nothing checked it

What broke:        `find_package(Permuto)` failed for every downstream project, in two layers,
                   both found the first time the install tree was put in front of a real consumer:
                   (1) `PermutoConfig.cmake` was produced by `configure_file()`, which leaves the
                   `@PACKAGE_INIT@` placeholder EMPTY — and that placeholder is what defines
                   `check_required_components()` — so a consumer's configure died on
                   `Unknown CMake command "check_required_components"`. (2) With that fixed it
                   died in `PermutoTargets.cmake`: "The link interface of target
                   Permuto::permuto contains Threads::Threads but the target was not found",
                   because `PermutoConfig.cmake.in` declared `find_dependency(nlohmann_json)` and
                   not `find_dependency(Threads)` — every imported target in an exported
                   interface must be re-established on the consumer side, and this library links
                   Threads for its thread_local state. The library, the headers and the target
                   files all installed perfectly, which is why this looked complete: installable,
                   and unusable.
Check added:       `configure_package_config_file()` in place of `configure_file()`,
                   `find_dependency(Threads)`, and the `package` stage in `tools/ci.sh`, which
                   installs the built tree into a temp prefix and then writes, configures, builds
                   and RUNS a two-file consumer project (`find_package(Permuto)` plus a program
                   that calls `permuto::apply()` through the installed headers), requiring the
                   exact substitution it prints. Verified end to end: `find_package(Permuto) ->
                   apply() -> {"greeting":"consumer"}`, stage cost 3 s. On a machine without a
                   system nlohmann/json the stage SKIPs and says why, because the consumer
                   resolves that dependency through `find_dependency`.
Why it must stay:  A package is a promise to people who are not in this repo, and this is the one
                   class of failure a repo cannot see from the inside: every stage here builds
                   against the SOURCE tree, where nothing is imported and nothing can be missing.
                   It was found by accident — checking the exported interface after adding the
                   nlohmann fetch fallback — and the stage is what makes it found by the gate
                   instead. See also the entry above: same shape, an assumption with no check.

## 2026-09-20 — a fresh clone could not configure, because nlohmann was assumed to be installed

What broke:        `cmake -B build` on a fresh clone died inside
                   `find_package(nlohmann_json 3.2.0 REQUIRED)` on the Nobara laptop: no
                   `/usr/include/nlohmann`, and the package that provides it there is called
                   `json-devel`, which nobody guesses. Nothing was wrong with the code — the build
                   assumed a system dependency that only the machine it was developed on happened
                   to have (`nlohmann-json3-dev`), so "it builds" was a property of the developer's
                   box, not of the repo. (The `git pull` before the attempt changed nothing: this
                   work was still uncommitted, so that clone was unchanged.)
Check added:       `CMakeLists.txt` finds the package QUIETly and, when there is none, supplies
                   the headers itself: it fetches nlohmann's LATEST RELEASE from GitHub
                   (`releases/latest/download/json.tar.xz`, unpinned by Harri's call — a mature
                   library, and pinning would only mean re-deciding later), with
                   `-DPERMUTO_NLOHMANN_DIR` for a machine that must stay offline. Both fallbacks
                   register the headers as an IMPORTED interface target under the same name
                   `find_package()` creates, and that detail is load-bearing: adding the
                   dependency as a subproject breaks `install(EXPORT PermutoTargets)` at generate
                   time ("requires target nlohmann_json that is not in any export set" — measured
                   on this change's first attempt, which is why the fix is an imported target and
                   not `add_subdirectory`). Verified: system-package path (no fetch, tests pass,
                   install works), forced-fetch path (fetched, built, tested, installed), the
                   `PERMUTO_NLOHMANN_DIR` path offline, and a downstream `find_package(Permuto)`
                   consumer built against the fetched install tree.
Why it must stay:  An assumption with no check, in the same shape as every other entry here: it
                   was invisible on the machine where the code was written and fatal on a clean
                   one. Note honestly that NO gate stage exercises the fetch path — this machine
                   has the package, `find_package()` wins, and the gate stays offline. The
                   intended exercise is a from-scratch build on a machine that lacks it (the Pi
                   has no nlohmann): clone, configure, build, test, with no local install.

## 2026-09-20 — the docs described a project that did not exist, and nothing read them

What broke:        `CLAUDE.md`'s Project Overview still said the project was "in the
                   **design/planning phase** with comprehensive documentation but no
                   implementation yet", and its status line advertised a test count, months after
                   the library, CLI, examples, tests and gate all existed. The same figure sat in
                   README's badge and Testing section, and the file carried a seven-phase
                   "Implementation Plan" whose every phase was still "pending". All of it was
                   false and every stage was green, because no stage has ever read a document:
                   a stale count is a claim with no check behind it, while the suite is one
                   `ctest -N` away from the truth.
Check added:       the `docs` stage in `tools/ci.sh` — no file in `CI_DOCS_FILES` (README.md,
                   CLAUDE.md, CODING_STANDARDS.md, TECHNICAL_DETAILS.md, REQUIREMENTS.md) may
                   quote a test count, in the badge form (`tests-64%2F64`), the prose form
                   ("64 tests", "64 unit tests", "64 TESTS"), the label form ("Total Tests: 64")
                   or the parenthetical form ("tests (64)"). The first version of that pattern
                   was porous — an independent verification walked "Total Tests: 66" and
                   "58 unit tests" straight past it, and the pattern now covers all eight
                   shapes it tried. `INCIDENTS.md` is deliberately EXEMPT: its numbers are dated
                   measurements of past events, and keeping them in step with today's suite
                   would mean requiring them to be wrong. The count-quoting rule is Harri's
                   call, 2026-09-20: "don't quote counts" — describe what the tests cover
                   instead. This entry's own prose obeys it.
Why it must stay:  A stale doc is worse than a missing one, because it is read as current.
                   Note the limit honestly: this stage checks COUNTS, not whether prose is
                   true. The Project Overview was corrected by hand, and only review keeps it
                   true — the stage exists to stop the one shape of drift that is measurable.

## 2026-09-20 — a template key holding `/` or `~` silently vanished from the round trip

What broke:        The library's headline guarantee — `apply()`, then `create_reverse_template()`,
                   then `apply_reverse()` reproduces the context — failed quietly for any template
                   whose member key holds `/` or `~`. `analyze_object()` composed the result
                   pointer as `current_path + "/" + key`, with no RFC 6901 escaping, so the reverse
                   template for `{"a/b": "${/x}"}` was `{"/a/b": "/x"}`: `apply_reverse()` looked up
                   `/a/b` in `{"a/b": 1}`, which reads as member `a` then member `b`, found nothing,
                   and emitted `{}`. No exception, no warning, no visibly wrong output — the value
                   was simply gone. `REQUIREMENTS.md` FR-3.1 already required the escaped form
                   ("/user~1role" for keys with slashes) on the input side of the same rule, so the
                   library was violating its own documented path syntax when it produced one.
                   Found by the fuzzer's round-trip oracle ten seconds into its first campaign, on
                   an input mutated from this repo's own README example. Six independent
                   reproducers are kept in `fuzz/regressions/`.
Check added:       `escape_pointer_token()` in `src/reverse_processor.cpp` (one pass, `~` -> `~0`
                   then `/` -> `~1`, so a literal `~1` in a key cannot come back as a slash), used
                   by `analyze_object()`; three tests written RED first —
                   `ResultPathEscapesSlashInKey`, `ResultPathEscapesTildeInKey`,
                   `ResultPathEscapesNestedKeyHoldingBoth` (`tests/test_reverse_processor.cpp`).
                   Behind them, the `fuzz` stage: the reproducers are replayed by every run, and
                   the seed smoke fails the gate when no input reaches the assertion, so the oracle
                   cannot rot into decoration.
Why it must stay:  Every other stage was green on this tree, and every input any of them ever fed
                   this library was written by a human who already believed it would work. This is
                   the class of defect a suite can only catch once somebody thinks of the case: a
                   silently wrong answer in the one feature the project is named after, not a crash.

## 2026-09-20 — `/` addressed the whole document, so an empty-key member never came back

What broke:        Two sites, one cause. `JsonPointer::parse_path()` split a pointer's tail with
                   `std::getline()`, which emits no final empty token, so the pointer `/` produced
                   ZERO tokens: `is_root()` was true and `resolve("/")` answered with the whole
                   document instead of the member whose key is empty. The same bug ate every
                   trailing empty token. Measured before the fix, against `{"":1,"a":{"":2}}`:
                   `""` -> root (correct), `/` -> 0 tokens -> the whole document (wrong, RFC 6901
                   says one empty token), `/a/` -> 1 token -> `{"":2}` (wrong, that pointer names
                   the empty key inside `a`), `//` -> 1 token (wrong, two empty tokens).
                   `ReverseProcessor::path_to_tokens()` carried a second, hand-copied tokenizer with
                   the same bug, so on the write side `set_at_path(context, "/", value)` looped over
                   no tokens and stored nothing: another silent drop. Both were reached by the
                   harness, the second inside the same reproducer as the key-escaping defect above.
Check added:       `parse_path()` splits on `/` by hand, preserving empty tokens; and
                   `path_to_tokens()` no longer implements a tokenizer at all — it returns
                   `JsonPointer(path).tokens()`, so this codebase has ONE pointer tokenizer instead
                   of two that can drift apart. Five tests written RED first:
                   `SlashIsTheMemberWithTheEmptyKey`, `TrailingSlashNamesTheEmptyKeyMember`,
                   `DoubleSlashIsTwoEmptyTokens` (`tests/test_json_pointer.cpp`) and
                   `RoundTripEmptyTemplateKey`, `RoundTripEmptyContextKey`
                   (`tests/test_reverse_processor.cpp`).
Why it must stay:  RFC 6901 defines `""` as the whole document and `"/"` as one token, and the
                   difference is invisible until a document actually has an empty key. The
                   duplicate tokenizer is the structural half of the lesson: the two copies had
                   already drifted, and the drift was only observable through a round trip.

## 2026-09-20 — ten gate stages, and not one of them ran the library against hostile input

What broke:        Nothing visible — which is the point. The gate certified ten stages
                   (`tree format kitprobes build tests version asan tsan tidy pristine`) and
                   printed `GATE PASSED`, and every input any of them ever fed this library
                   was written by a human who already believed it would work: the 58 tests,
                   the three examples, `example_template.json`. `asan` and `tsan` are
                   sanitizers, not input sources — they can only find a bug in an input
                   somebody already thought to write down. The adaptation notes at the top of
                   `tools/ci.sh` said so in one line, `* no fuzz stage: the repo has no fuzz
                   target yet`, and that line had been true long enough to read as a decision
                   rather than a hole. It was a hole. The first ten seconds of the harness
                   that filled it produced a reproducer, from a mutation of this repo's own
                   README example: `create_reverse_template()` builds the result pointer for
                   an object member as `current_path + "/" + key` without JSON-Pointer
                   escaping (`src/reverse_processor.cpp`, `analyze_object`), so a template key
                   holding `/` or `~` produces a reverse template that addresses nothing —
                   template `{"a/b":"${/x}"}` with context `{"x":1}` reverses to
                   `{"/a/b":"/x"}`, which resolves nowhere in `{"a/b":1}`, and the round trip
                   silently returns `{}`. That is the documented round-trip guarantee
                   (README.md "Reverse Operations", REQUIREMENTS.md) failing quietly, on a
                   tree where every other stage is green.
Check added:       `fuzz/fuzz_permuto.cpp` (a libFuzzer harness whose oracle IS the round-trip
                   guarantee: `apply` -> `create_reverse_template` -> `apply_reverse` must
                   reproduce the context, asserted only under the conditions the guarantee is
                   claimed for, which the harness header spells out one by one) + the `fuzz`
                   stage in `tools/ci.sh`, in `CI_DEFAULT_STAGES` and in `.githooks/pre-push`,
                   between `tsan` and `tidy`. The stage runs the binary twice. The second run
                   is the timed campaign (`CI_FUZZ_SECONDS`, default 10 s, against the corpus
                   in `fuzz/corpus/` plus the tracked seeds in `fuzz/seeds/`). The FIRST run
                   is the check this entry is really about: `-runs=0` over `fuzz/seeds` with
                   `PERMUTO_FUZZ_REQUIRE_IDENTITY=1`, which makes the harness exit non-zero
                   when not one input reached the round-trip assert, and the stage then prints
                   the number that did.
Why it must stay:  A fuzz target that never reaches its property is indistinguishable, from
                   the outside, from one that always passes: same exit status, same green
                   stage, same `GATE PASSED`. That is the identical shape as the failure two
                   entries down (nine stages that never ran, reported as ten that passed) and
                   as the tidy baseline that could never match — a check whose output is
                   "fine" whether or not it did anything. Deleting the seed smoke, or letting
                   the seeds drift until the four conditions stop holding for all of them,
                   converts this stage back into decoration while leaving the summary line
                   unchanged. The budget is deliberately small (10 s on every push, the
                   rationale is in `.ci.env.example` and the adaptation notes) because an
                   always-red or always-slow stage gets `--no-verify`, which is the same hole
                   again with a different cause.


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

