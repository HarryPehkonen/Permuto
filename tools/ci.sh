#!/usr/bin/env bash
#
# Local CI — every gate this repo has, in one script, with no service anywhere.
#
# from AI-DEV-STARTER (plunk-in kit) — if you edit this file, say why in INCIDENTS.md.
#
# No GitHub, no network, no framework: this is what the git hooks in .githooks/ run,
# and you can run it by hand at any time (it is non-destructive — nothing is committed,
# staged, reverted or reformatted for you).
#
#   tools/ci.sh                      # all default stages, in order
#   tools/ci.sh build tests          # just these stages, in the order given
#   tools/ci.sh --list               # what the stages are
#   tools/ci.sh --write-tidy-baseline   # accept the tidy findings you inherited
#   tools/ci.sh --help
#
#   git config core.hooksPath .githooks     # one-time, per clone, enables the hooks
#
# Two tiers, because a C++ full run is minutes and a commit cannot afford minutes:
#
#   fast  (pre-commit)  format build tests
#   full  (pre-push)    --require-clean tree docs format kitprobes build tests version asan tsan fuzz tidy pristine package
#
# PERMUTO ADAPTATIONS (every deviation from the kit is listed here, with the reason):
#   * `tidy` runs against this repo's own `.clang-tidy`, which enables the BUG-FINDING subset
#     (clang-diagnostic-* + clang-analyzer-*) and no style families. Wire-date measurement:
#     0 findings over 17 translation units in 104-201 s on 4 cores (cache-dependent: 201 s
#     cold after a fresh build, 104 s warm) — and the one finding it did report on the day it
#     was wired was real (a dead store in examples/api_example.cpp that made the example claim
#     success after a failed round trip). The two heavier rule sets and why they were rejected,
#     with numbers, are in INCIDENTS.md and .ci.env.example.
#   * stage_version also compares the generated CMake package-version file
#     (write_basic_package_version_file -> $CI_BUILD_DIR/*ConfigVersion.cmake): a THIRD
#     copy of the version number that no kit stage ever looked at. See INCIDENTS.md.
#   * `fuzz` — a libFuzzer harness (fuzz/fuzz_permuto.cpp) whose oracle is this library's own
#     documented round-trip guarantee: apply() -> create_reverse_template() -> apply_reverse()
#     must reproduce the context, asserted only under the conditions the guarantee is claimed
#     for (they are spelled out in the harness header). It needs clang++ — libFuzzer ships as
#     clang's -fsanitize=fuzzer and has no GCC equivalent — so a machine without clang SKIPs
#     the stage unless CI_STRICT_TOOLS=1, exactly like clang-format and clang-tidy.
#     Budget: CI_FUZZ_SECONDS, default 10 s. A commit and a push must not cost minutes — this
#     gate is already ~3 minutes with `tidy` in it, and a fuzzer's value comes from hours, not
#     from the 60 s nobody will wait for at a push prompt. Ten seconds is a REGRESSION check
#     (the corpus in fuzz/corpus/ keeps what earlier runs found, and the seeds are re-read
#     every time), not a campaign; long campaigns belong on a nightly run with a bigger
#     -max_total_time against the same corpus.
#     The stage runs the binary TWICE. First a seed smoke: -runs=0 over fuzz/seeds with
#     PERMUTO_FUZZ_REQUIRE_IDENTITY=1, which fails when no input reached the round-trip
#     assert. That is the live-oracle guard — a harness that never reaches its property looks
#     exactly like a harness that always passes, and the ten stages before this one all had
#     that shape with respect to hostile input. Then the timed campaign.
#   * `docs` — one rule: no file in CI_DOCS_FILES quotes a test count. This repo advertised
#     "65 tests" in a badge, a README section and CLAUDE.md against a suite of 58, and
#     CLAUDE.md's Project Overview still described a design phase that had ended — all of it
#     green, because no stage has ever read a document. Counts drift silently; describing
#     coverage does not. INCIDENTS.md is exempt on purpose (dated measurements, above).
#   * `package` — the kit has no such stage. The install tree and PermutoConfig.cmake are a
#     promise to consumers and NOTHING checked them: the config file was generated with
#     configure_file(), so @PACKAGE_INIT@ expanded to nothing and every downstream
#     find_package(Permuto) died on "Unknown CMake command check_required_components"
#     (2026-09-20, INCIDENTS.md). It needs a SYSTEM nlohmann/json, because the consumer resolves
#     Permuto's dependency through find_dependency; with none it SKIPs and says why.
#   (Until 2026-09-20 this block also had to explain ci-build/ and build-ci*/ build dirs
#    and the missing tree and format stages. All of that existed only because the OLD
#    build/ tree was committed; untracking it removed the cause, so the adaptations went
#    with it. If build/ is ever committed again, `tools/ci.sh tree` fails — the rule has a
#    check behind it now.)
#
# Configuration lives in .ci.env (gitignored, optional); every knob has a default here,
# so the repo works with no config at all. See .ci.env.example.
#
# Exit status: 0 only if every stage that ran passed.
#
# One deliberate reading of "report every failure": a failing stage prints EVERYTHING
# that failed inside it (every unformatted file, every failing test, every finding) and
# then STOPS the run, because these stages are a chain — once the build fails, the
# tests, sanitizers and pristine stages would only repeat the same compiler error with
# more noise. Stages that do not depend on each other (tree, format, version) are
# cheap and run first for that reason. Where a run stops, the summary says so.

set -uo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$REPO_ROOT" || exit 1

# No colour from the tools: this script greps their output (warning:, error:) and ANSI
# escapes defeat the greps. The escapes this script prints itself are for the human.
export NO_COLOR=1

# git exports GIT_INDEX_FILE to a hook when the commit is made with a PATHSPEC
# (`git commit -- <path>`): it names git's TEMPORARY index for that one commit, not this
# repository's index, and every process a hook starts inherits it. Any `git` command the
# gate runs inside ANOTHER repository then reads THIS repo's index entries against that
# repository's object store and dies on the first blob it does not have,
#     fatal: unable to read 691e2bdafaf312970644391de042d38c2c5972d8
# which is how a pathspec commit failed its OWN gate at `build` on Computo, naming a
# dependency update, on a tree that builds fine (cards t_9541aa62 -> t_0a9a0018); it leaves
# that checkout hollow as well (no `.git/index`, empty worktree).
#
# This repo's exposure is stated honestly rather than implied: its own gate has no
# dependency clone — CMakeLists.txt uses find_package(GTest) and no FetchContent — so no
# stage of it runs git outside this checkout TODAY. The line is here because the class is
# general, and this copy already contains an instance of it: tools/kit-probes/ runs git in
# throwaway repositories in a temp dir by design (that is how the kit's probes work), and
# anything a stage shells out to — a future FetchContent, a tool that clones — inherits the
# variable the same way. One line in the prologue is cheaper than re-diagnosing it.
#
# Unset it once, here, rather than `env -u` per command: the variable reaches everything the
# gate starts, so a per-invocation fix covers the instance and leaves the class. Measured
# before choosing the place (a real pathspec commit in a throwaway clone, with the gate's own
# stages run both ways): every input the `tree`/`format` stages read is identical and their
# output is byte-identical. Ported from the kit's templates/cpp/ci.sh at f9c3300;
# tools/kit-probes/git-index-file.sh holds this copy to it.
unset GIT_INDEX_FILE

# ---------------------------------------------------------------- defaults + config
CI_JOBS=${CI_JOBS:-$(nproc 2>/dev/null || echo 4)}
CI_BUILD_DIR=${CI_BUILD_DIR:-build}
CI_ASAN_BUILD_DIR=${CI_ASAN_BUILD_DIR:-build-asan}
CI_TSAN_BUILD_DIR=${CI_TSAN_BUILD_DIR:-build-tsan}
# --- fuzz stage (see the adaptation notes at the top) ---
# 10 s, because this runs on every push and a push must not cost minutes. It is a
# regression check over an accumulated corpus, not a campaign.
CI_FUZZ_SECONDS=${CI_FUZZ_SECONDS:-10}
# Its own build dir: the harness build is clang-only and instruments the LIBRARY too,
# so it can never share build/ with the normal gcc build. build-*/ is already ignored.
CI_FUZZ_BUILD_DIR=${CI_FUZZ_BUILD_DIR:-build-fuzz}
# The corpus libFuzzer grows and re-reads. Gitignored; fuzz/seeds/ is tracked instead.
CI_FUZZ_CORPUS=${CI_FUZZ_CORPUS:-fuzz/corpus}
# --- docs stage ---
# The documents that describe this repo's CURRENT state, and therefore must not quote a test
# count: the count is one `ctest -N` away and drifts the moment a test is added, while the
# prose never notices. INCIDENTS.md is deliberately absent — its numbers are dated
# measurements of past events, and keeping them in step with today's suite would mean
# requiring them to be wrong. `.ci.env` can override this list.
CI_DOCS_FILES=${CI_DOCS_FILES:-"README.md CLAUDE.md CODING_STANDARDS.md TECHNICAL_DETAILS.md REQUIREMENTS.md"}
CI_LOG_DIR=${CI_LOG_DIR:-.ci-logs}
CI_STRICT_TOOLS=${CI_STRICT_TOOLS:-0}           # 1 = a missing tool fails instead of SKIPping
CI_KEEP_TMP=${CI_KEEP_TMP:-0}                   # 1 = keep the pristine temp dir for inspection
# The two hook tiers, ONE definition each. Both hooks NAME a tier instead of repeating a list, so a
# stage added below cannot be run by a hand run and skipped by a push (or the reverse) — the failure
# this repo's own incident log records elsewhere in the fleet, where a push ran eleven stages of
# twelve and still printed GATE PASSED. The lines below are what probes/hook-tiers-agree.sh compares
# the two variables against, and against the tier the hooks name.
#
#
# `format` is in the fast tier deliberately: it is the one check that says "the file you are about to
# commit is not the file clang-format would write", it costs well under a second on a warm tree, and
# its absence from a fast tier is how an unformatted commit reached FSMTable's main branch on
# 2026-10-06. `full` IS the default list, so a hand run and a push run the same stages and only
# --require-clean differs.
CI_FAST_STAGES=${CI_FAST_STAGES:-"format build tests"}
CI_FULL_STAGES=${CI_FULL_STAGES:-"tree docs format kitprobes build tests version asan tsan fuzz tidy pristine package"}
CI_DEFAULT_STAGES=${CI_DEFAULT_STAGES:-$CI_FULL_STAGES}
CI_TIDY_BASELINE=${CI_TIDY_BASELINE:-.ci/tidy-baseline.txt}
CI_BUILD_TYPE=${CI_BUILD_TYPE:-Debug}
# The source set the format and tidy stages own. Extend for your layout.
CI_SOURCE_GLOBS=${CI_SOURCE_GLOBS:-"'*.cpp' '*.cc' '*.cxx' '*.hpp' '*.hh' '*.h'"}
# Where the SECOND copy of the version number lives. Two forms are both fine:
#   the CMake-generated header  (configure_file(version.hpp.in ...) -> ${CMAKE_BINARY_DIR}/generated/version.hpp)
#   a tracked header            (include/<project>/version.hpp) — point this knob at it
CI_VERSION_HEADER=${CI_VERSION_HEADER:-"$CI_BUILD_DIR/generated/version.hpp"}
# The test runner. ctest is the portable default; override with a single test binary if
# your project does not register tests with add_test().
CI_TEST_CMD=${CI_TEST_CMD:-"ctest --test-dir \$CI_BUILD_DIR --output-on-failure -j \$CI_JOBS"}
# ---- Permuto's own defaults (rationale in the adaptation notes at the top) ----
# These OVERRIDE the kit defaults above by plain assignment, on purpose: the kit's
# ${VAR:-default} form would keep its own value because it is set first. .ci.env is
# sourced after this block, so a config file still wins over everything here.
#
# The build dirs are the kit's own (build/, build-asan/, build-tsan/) and CI_VERSION_HEADER
# stays at the kit default ($CI_BUILD_DIR/generated/version.hpp), because
# configure_file(cmake/version.hpp.in ...) writes the second copy of the number there.
# The binary that has to answer --version with the same number:
CI_VERSION_BINARIES="$CI_BUILD_DIR/permuto"
# What this repo can certify: every stage the kit ships, `tidy` included — it runs against
# this repo's own .clang-tidy (the bug-finding subset; see the adaptation notes at the top).
# Both hooks run this list (the pre-push hook spells it out), and the fast tier stays
# "build tests".
CI_DEFAULT_STAGES="tree docs format kitprobes build tests version asan tsan fuzz tidy pristine package"

if [ -f .ci.env ]; then
    # shellcheck disable=SC1091
    . ./.ci.env
fi

REQUIRE_CLEAN=0
ALLOW_UNTRACKED=0
WRITE_TIDY_BASELINE=0
STAGES_REQUESTED=()

# ---------------------------------------------------------------- plumbing
RESULT_LINES=()
FAILED_STAGE=""
RAN_STAGES=()
RUN_TMP_DIRS=()

usage() {
    # The kit's own header comment, stopping before the PERMUTO ADAPTATIONS block below
    # it. Derived from that marker line rather than from a hard-coded line number: this
    # header already grew once (the two-tier line, 2026-09-20) and a stale range silently
    # prints the wrong slice of the file into --help. The kit derives its end from the
    # first blank line, which cannot work here — the repo-local adaptation notes sit inside
    # the same comment block.
    awk 'NR > 1 { if (/^# PERMUTO ADAPTATIONS/) exit; sub(/^# ?/, ""); print }' "$0"
    cat <<'EOF'

Stages:
  tree        every file committed or ignored; .gitignore audit; the gate's own
              footprint (build dirs, logs, .ci.env) is ignored; --require-clean also
              fails on uncommitted changes to tracked files
  docs        no document in CI_DOCS_FILES quotes a test count: the suite is the only
              tally, and a number in prose drifts silently. INCIDENTS.md is exempt (its
              numbers are dated measurements of past events)
  format      clang-format drift — dry run against the repo .clang-format
  kitprobes   the kit fixes this copy claims to carry, held to their contracts: every
              script in tools/kit-probes/ checks one kit fix in THIS gate script by name
              and by behaviour (offline, no kit checkout, no build, <1 s). A missing
              directory SKIPs: it means this copy carries no probe yet, not that it is
              behind. The rule: KIT-REVISION-CONVENTION.md
  build       cmake configure + build, zero warnings (the stage counts them even where
              -Werror is not wired onto a target)
  tests       the test suite (ctest by default), every failure reported
  version     one version number: project(VERSION) in CMakeLists.txt == the header the
              build generates/uses, and the number every binary prints for --version
  asan        separate build dir, ASan+UBSan, same suite
  tsan        separate build dir, ThreadSanitizer, same suite
  fuzz        libFuzzer (clang only), CI_FUZZ_SECONDS seconds against fuzz/corpus +
              fuzz/seeds. The oracle is this library's documented round trip:
              apply -> create_reverse_template -> apply_reverse must reproduce the
              context. A seed smoke runs first with PERMUTO_FUZZ_REQUIRE_IDENTITY=1
              and fails when NO input reached that assert, so a corpus that cannot
              exercise the property fails instead of passing silently
  tidy        clang-tidy, only NEW findings vs CI_TIDY_BASELINE (a baseline file is
              optional; with none, tidy must be clean). Findings compare line-blind and
              clone-blind — capture the baseline with --write-tidy-baseline, never by
              hand (a raw copy of the log matches nothing: see .ci.env.example)
  pristine    git archive HEAD -> temp dir -> configure, build, test: proves the
              COMMITTED tree is complete (catches files that are uncommitted or ignored)
  package     installs the built tree to a temp prefix and builds a two-file consumer
              against it with find_package(Permuto): proves the install tree and its CMake
              package files are consumable, not merely installable. Needs a system
              nlohmann/json (the consumer resolves it via find_dependency); SKIPs if absent

tidy runs here against this repo's own .clang-tidy, which enables the bug-finding subset
(clang-diagnostic-* + clang-analyzer-*) and no style families, and it IS in the default
stage list. The measurement behind that choice — and the two heavier rule sets that were
rejected, with their numbers — is in INCIDENTS.md; the PERMUTO ADAPTATIONS notes at the top
of this file say the same thing.

Options:
  --require-clean     make the tree stage fail when tracked files have uncommitted edits
  --allow-untracked   do not fail when untracked, unignored files exist (deliberate escape)
  --strict-tools      a missing tool (clang-format/clang-tidy) fails instead of skipping
  --write-tidy-baseline
                      accept every finding tidy reports now into CI_TIDY_BASELINE (runs the
                      build and tidy stages first); prints, and IS NOT, a gate pass
  --list              list the stages and exit
  --help              this text
EOF
}

ci_begin() { printf '\n\033[1m==> %s\033[0m\n' "$1"; }
ci_pass() { RESULT_LINES+=("pass  $1"); }
ci_skip() {
    RESULT_LINES+=("SKIP  $1 ($2)")
    printf '    SKIP: %s\n' "$2"
}

ci_fail() {
    local name="$1" reason="$2" logfile="${3:-}"
    RESULT_LINES+=("FAIL  $name")
    printf '\n\033[1;31mFAILED: %s — %s\033[0m\n' "$name" "$reason"
    if [ -n "$logfile" ] && [ -f "$logfile" ]; then
        printf '    last output from %s:\n' "$logfile"
        tail -n 25 "$logfile" | sed 's/^/      /'
        printf '    full log: %s\n' "$logfile"
    fi
    FAILED_STAGE="$name"
    summary
    printf '\nGATE FAILED\n' >&2
    exit 1
}

summary() {
    printf '\n--- ci summary ---\n'
    local line
    for line in "${RESULT_LINES[@]}"; do printf '  %s\n' "$line"; done
    if [ -n "$FAILED_STAGE" ]; then
        printf '  stopped at: %s\n' "$FAILED_STAGE"
        # A stage that never ran because an earlier one failed must not look like a
        # stage that passed. Naming it is the whole difference between "everything is
        # fine" and "the tests never ran" — this is why the run stops instead of
        # pretending: once the build fails, the test stage has nothing trustworthy to
        # say, so it is reported as blocked, never as green.
        local s r ran
        for s in "${STAGES_REQUESTED[@]:-}"; do
            [ -n "$s" ] || continue
            ran=0
            for r in "${RAN_STAGES[@]:-}"; do
                [ "$r" = "$s" ] && ran=1
            done
            if [ "$ran" = "0" ] && [ "$s" != "$FAILED_STAGE" ]; then
                printf '  BLOCK %s (did not run: the run stopped at %s)\n' "$s" "$FAILED_STAGE"
            fi
        done
    fi
}

require_tool() {  # require_tool <tool> <stage>; returns 1 when the caller should skip
    local tool="$1" stage="$2"
    if command -v "$tool" >/dev/null 2>&1; then return 0; fi
    if [ "$CI_STRICT_TOOLS" = "1" ]; then
        ci_fail "$stage" "$tool not installed (CI_STRICT_TOOLS=1) — nothing was checked"
    fi
    ci_skip "$stage" "$tool not installed — this gate was NOT exercised on this machine"
    return 1
}

# The file set the format gate owns: same list the CMake `format` target should use.
# --others --exclude-standard includes new files that are not committed yet: while
# working, a new source file is not in `git ls-files`, and a gate that cannot see it
# lets it through unformatted until after it is committed (that incident is in
# INCIDENTS.md).
ci_sources() {
    # shellcheck disable=SC2086
    eval "git ls-files --cached --others --exclude-standard -- $CI_SOURCE_GLOBS" | sort -u
}

# clang-tidy needs a compile database entry per translation unit, so headers are not
# passed here: diagnostics inside headers still surface through the sources that include
# them (that is what .clang-tidy's HeaderFilterRegex is for).
ci_tidy_sources() {
    # shellcheck disable=SC2086
    eval "git ls-files --cached --others --exclude-standard -- $CI_SOURCE_GLOBS" \
        | grep -E '\.(cpp|cc|cxx)$' | sort -u
}

# The comparable form of a clang-tidy finding — applied to BOTH sides of the baseline
# comparison, so the file a repo captures and the log this gate just wrote are the same
# shape:
#
#   <repo>/src/foo.cpp:42:7: warning: ...   ->   src/foo.cpp: warning: ...
#
#   * the repo root is stripped: clang-tidy reports the path it was handed by the compile
#     database, which CMake writes as an absolute path, so a baseline captured in one clone
#     names no finding in a checkout at another path (the nightly clean checkout, a
#     colleague's machine) and every inherited finding reads as new;
#   * :line:column is stripped, so the same finding after an unrelated edit above it is
#     still the same finding. This is line-blind on purpose, and the flip side is worth
#     knowing: a SECOND identical finding in a file that already has one collapses into the
#     first. Fix the baselined finding instead of growing the baseline.
#
# `tools/ci.sh --write-tidy-baseline` captures the baseline through this same function, so
# the documented way to accept findings cannot drift from the way they are compared.
tidy_key() {
    awk -v root="$REPO_ROOT/" '
        { i = index($0, root); if (i) $0 = substr($0, i + length(root)); print }' \
        | sed 's/:[0-9]*:[0-9]*:/:/'
}

run_tests() {  # run_tests <build-dir> <logfile>
    # shellcheck disable=SC2086
    eval "$CI_TEST_CMD" > "$2" 2>&1
}

mkdir -p "$CI_LOG_DIR"

# ---------------------------------------------------------------- stages
stage_tree() {
    ci_begin "tree (every file committed or ignored)"
    # The gate creates these; a .gitignore that does not cover them makes the next run
    # fail the moment it writes a log. Create them first: git check-ignore cannot match
    # a directory pattern against a path that does not exist yet.
    mkdir -p "$CI_BUILD_DIR" "$CI_ASAN_BUILD_DIR" "$CI_TSAN_BUILD_DIR" "$CI_LOG_DIR"

    local untracked
    untracked=$(git ls-files --others --exclude-standard)
    if [ -n "$untracked" ]; then
        printf '    not committed and not ignored:\n'
        printf '%s\n' "$untracked" | sed 's/^/      /'
        if [ "$ALLOW_UNTRACKED" = "1" ]; then
            printf '    (not failing: --allow-untracked was passed)\n'
        else
            ci_fail tree "$(printf '%s\n' "$untracked" | wc -l) file(s) are neither committed nor ignored — git add them, or add a .gitignore rule"
        fi
    else
        printf '    no untracked, unignored files\n'
    fi

    local dirty
    dirty=$(git status --porcelain --untracked-files=no)
    if [ -n "$dirty" ]; then
        printf '    uncommitted changes to tracked files:\n'
        printf '%s\n' "$dirty" | sed 's/^/      /'
        if [ "$REQUIRE_CLEAN" = "1" ]; then
            ci_fail tree "uncommitted changes to tracked files (that is the point of the push gate)"
        fi
        printf '    (not failing: --require-clean was not passed)\n'
    else
        printf '    no uncommitted changes to tracked files\n'
    fi

    local tracked_ignored
    tracked_ignored=$(git ls-files -i -c --exclude-standard)
    if [ -n "$tracked_ignored" ]; then
        printf '%s\n' "$tracked_ignored" > "$CI_LOG_DIR/tree.log"
        ci_fail tree "tracked files matched by .gitignore (stale rules) — see $CI_LOG_DIR/tree.log"
    fi

    local path missing=0
    for path in "$CI_BUILD_DIR" "$CI_ASAN_BUILD_DIR" "$CI_TSAN_BUILD_DIR" "$CI_LOG_DIR" ".ci.env"; do
        if ! git check-ignore -q "$path" 2>/dev/null; then
            printf '    NOT ignored: %s\n' "$path"
            missing=$((missing + 1))
        fi
    done
    if [ "$missing" -gt 0 ]; then
        ci_fail tree "$missing path(s) that the gate itself creates are not in .gitignore — the next run would fail on its own log files"
    fi
    printf '    gate footprint (build dirs, %s, .ci.env) is ignored\n' "$CI_LOG_DIR"
    ci_pass tree
}

stage_docs() {
    ci_begin "docs (no doc quotes a test count)"
    # One rule, and it exists because its absence was invisible: README's badge read
    # "tests-65/65" and CLAUDE.md advertised "65 tests" against a suite of 58, and CLAUDE.md
    # still described the project as being in a design phase after it was implemented. No
    # stage had ever read a document, so every one of those statements was green. A number in
    # prose is a claim with no check behind it; the suite is the only tally.
    # The full rationale is in INCIDENTS.md.
    local -a files=()
    local file
    for file in $CI_DOCS_FILES; do
        if [ -f "$file" ]; then
            files+=("$file")
        fi
    done
    if [ "${#files[@]}" -eq 0 ]; then
        ci_fail docs "none of CI_DOCS_FILES exists ($CI_DOCS_FILES) — this stage would check nothing and still pass"
    fi

    local offenders=0
    : > "$CI_LOG_DIR/docs.log"
    for file in "${files[@]}"; do
        # Every shape a tally can take: the badge (tests-58%2F58), prose ("58 tests",
        # "58 unit tests", "58 TESTS"), the label form ("Total Tests: 66", "tests: 58") and
        # the parenthetical ("tests (58)"). -H so the log names the file: grep prints no
        # filename when it is handed a single one, and an offender you cannot locate is a
        # finding you will not fix. The first version of this pattern was porous — the
        # verifier walked "Total Tests: 66" and "58 unit tests" straight past it (C6 in
        # /tmp/permuto-verification.md).
        if grep -nHEi 'tests-[0-9]|[0-9]+[[:space:]]+([[:alpha:]]+[[:space:]]+)*tests?\b|tests?[[:space:]]*:[[:space:]]*[0-9]+|tests?[[:space:]]*\([[:space:]]*[0-9]+' "$file" \
            >> "$CI_LOG_DIR/docs.log"; then
            offenders=$((offenders + 1))
        fi
    done
    if [ "$offenders" -ne 0 ]; then
        sed 's/^/      /' "$CI_LOG_DIR/docs.log"
        ci_fail docs "$offenders doc file(s) quote a test count — the suite is the tally; say what the tests cover instead" "$CI_LOG_DIR/docs.log"
    fi
    printf '    %s doc file(s) describe the project without quoting a test count\n' "${#files[@]}"
    ci_pass docs
}

stage_format() {
    ci_begin "format (clang-format --dry-run) — the files this branch touches"
    require_tool clang-format format || return 0
    # Only the files this branch touches: the repo has pre-existing non-compliant files
    # and a whole-tree check buries the signal in noise nobody edited. On a clean
    # checkout (nothing ahead of origin/main) fall back to the last commit.
    local base touched
    base="$(git merge-base HEAD origin/main 2>/dev/null || git rev-parse HEAD)"
    touched="$(
        { git diff --name-only --diff-filter=ACMR "$base" HEAD
          git diff --name-only --diff-filter=ACMR HEAD
          git diff --cached --name-only --diff-filter=ACMR   # a staged-only change is invisible to the line above
          git ls-files --others --exclude-standard
        } | sort -u
    )"
    if [ -z "$touched" ]; then
        touched="$(git show --name-only --pretty=format: HEAD | sed '/^$/d')"
        printf '    (level with origin/main: checking the last commit instead)\n'
    fi
    local -a sources=()
    while IFS= read -r f; do
        [ -n "$f" ] && [ -f "$f" ] && sources+=("$f")
    done < <(printf '%s\n' "$touched" | grep -E '\.(cpp|cc|cxx|hpp|hh|h)$')
    if [ "${#sources[@]}" -eq 0 ]; then
        printf '    nothing to check\n'
        ci_pass format
        return 0
    fi
    if clang-format --dry-run -Werror "${sources[@]}" > "$CI_LOG_DIR/format.log" 2>&1; then
        # The check above read the WORKING TREE, and a commit records the INDEX. Stage an unformatted
        # file, then format it on disk -- which is what anyone does after this stage rejects a commit --
        # and the check above passes while the commit still records the unformatted text. HEAD then
        # differs from the working tree, and the next `--require-clean` push fails in `tree` with
        # "uncommitted changes to tracked files": a message that never mentions formatting. Kit fix
        # `format-checks-staged` (kit commit 96719c4, docs/KIT-FIXES.md); probe in tools/kit-probes/.
        local -a staged=() index_drift=()
        local f staged_f
        while IFS= read -r staged_f; do
            [ -n "$staged_f" ] && [ -f "$staged_f" ] && [[ "$staged_f" =~ \.(cpp|cc|cxx|hpp|hh|h)$ ]] && staged+=("$staged_f")
        done < <(git diff --cached --name-only --diff-filter=ACMR)
        if [ "${#staged[@]}" -gt 0 ]; then
            for f in "${staged[@]}"; do
                git show ":$f" 2>/dev/null |
                    clang-format --dry-run -Werror --assume-filename="$f" - > /dev/null 2>> "$CI_LOG_DIR/format.log" ||
                    index_drift+=("$f")
            done
            if [ "${#index_drift[@]}" -gt 0 ]; then
                printf '    the STAGED copy is not formatted (that is what the commit would record):\n'
                printf '%s\n' "${index_drift[@]}" | sed 's/^/      /'
                ci_fail format "the staged copy of the file(s) above fails clang-format -- this stage checks the working tree, and a commit records the index (fix: clang-format -i <files> && git add <files>)" "$CI_LOG_DIR/format.log"
                return 1
            fi
        fi
        printf '    %s file(s) conform to .clang-format\n' "${#sources[@]}"
        ci_pass format
        return 0
    fi
    grep -oE '^[^:]+\.(cpp|cc|cxx|hpp|hh|h)' "$CI_LOG_DIR/format.log" | sort -u | sed 's/^/      /'
    ci_fail format "clang-format drift in the files listed above (fix: clang-format -i <those files>)" "$CI_LOG_DIR/format.log"
}

stage_build() {
    ci_begin "build (-Werror, zero warnings)"
    # shellcheck disable=SC2086
    cmake -S . -B "$CI_BUILD_DIR" -DCMAKE_BUILD_TYPE="$CI_BUILD_TYPE" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/configure.log" 2>&1 \
        || ci_fail build "cmake configure failed" "$CI_LOG_DIR/configure.log"
    cmake --build "$CI_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/build.log" 2>&1 \
        || ci_fail build "build failed (-Werror is on: a warning is a build failure)" "$CI_LOG_DIR/build.log"
    # -Werror only covers the targets it is wired onto, so a target that never got the
    # flag would build with warnings and pass. Count them here too.
    local warns
    warns=$(grep -c 'warning:' "$CI_LOG_DIR/build.log" || true)
    if [ "${warns:-0}" -gt 0 ]; then
        grep 'warning:' "$CI_LOG_DIR/build.log" | head -5 | sed 's/^/      /'
        ci_fail build "$warns compiler warning(s) in a target -Werror does not cover" "$CI_LOG_DIR/build.log"
    fi
    printf '    built with no warnings\n'
    ci_pass build
}

stage_tests() {
    ci_begin "tests"
    if [ ! -f "$CI_BUILD_DIR/CMakeCache.txt" ]; then
        ci_fail tests "no build configured in $CI_BUILD_DIR — run the build stage first"
    fi
    if ! run_tests "$CI_BUILD_DIR" "$CI_LOG_DIR/tests.log"; then
        grep -E "FAILED|Failed|\*\*\*Failed|assert" "$CI_LOG_DIR/tests.log" | head -30 | sed 's/^/      /'
        ci_fail tests "test failures (all of them are above; full output in the log)" "$CI_LOG_DIR/tests.log"
    fi
    grep -E "tests passed|100% tests passed" "$CI_LOG_DIR/tests.log" | tail -1 | sed 's/^/      /'
    ci_pass tests
}

stage_version() {
    ci_begin "version (two copies of one number must agree)"
    local cmake_version
    cmake_version=$(sed -n 's/^project *( *[A-Za-z0-9_.-]* *VERSION *\([0-9][0-9.]*\).*/\1/p' CMakeLists.txt | head -1)
    if [ -z "$cmake_version" ]; then
        ci_fail version "could not read a VERSION from CMakeLists.txt — project(<name> VERSION <x.y.z>) is what this stage parses"
    fi

    local header="$CI_VERSION_HEADER"
    if [ ! -f "$header" ]; then
        # Help, not guesswork: say which files could hold the second copy.
        local candidates
        candidates=$(find "$CI_BUILD_DIR" -type f \( -name '*version*.hpp' -o -name '*version*.h' \) 2>/dev/null | sort | head -5)
        if [ -n "$candidates" ]; then
            printf '    %s not found. Version-named headers in %s:\n' "$header" "$CI_BUILD_DIR"
            printf '%s\n' "$candidates" | sed 's/^/      /'
        fi
        ci_fail version "the header holding the second copy of the number is not at $CI_VERSION_HEADER — point CI_VERSION_HEADER at it (a tracked header works too)"
    fi
    local header_version
    # Any of:  #define APP_VERSION "1.2.3"   |   static constexpr char VERSION[] = "1.2.3";
    header_version=$(grep -oE '"[0-9]+\.[0-9]+\.[0-9]+[^"]*"' "$header" | head -1 | tr -d '"')
    if [ -z "$header_version" ]; then
        ci_fail version "$header holds no quoted x.y.z version string"
    fi
    if [ "$cmake_version" != "$header_version" ]; then
        printf '    CMakeLists.txt says %s, %s says %s\n' "$cmake_version" "$header" "$header_version"
        ci_fail version "the two copies of the version number disagree (bump both from the same edit)"
    fi
    printf '    %s == %s == %s\n' "CMakeLists.txt" "$header" "$cmake_version"

    # --- every OTHER copy this repo has: the generated CMake package-version file -----
    # write_basic_package_version_file() writes set(PACKAGE_VERSION "x.y.z") into the
    # build dir. It is generated from project(VERSION), so it is right by construction —
    # until someone edits the wrong line there or hands the macro a version of its own,
    # and then a downstream find_package() is answered by a number this repo never
    # checked. (This repo's call reads `VERSION ${PACKAGE_VERSION}`, which is unset:
    # CMake's module falls back to PROJECT_VERSION, which is why the file is correct
    # today. That fallback is exactly the thing not to take on trust.)
    # maxdepth 1 is where CMakePackageConfigHelpers writes when it is given a bare
    # filename (this repo's shape). Searching the whole build tree would also pick up a
    # FetchContent'd dependency's *ConfigVersion.cmake — a version that is not ours to
    # police, and a check that would then fail on somebody else's number.
    if grep -qE '(^|[^#[:alnum:]_])write_basic_package_version_file' CMakeLists.txt; then
        local -a pkg_version_files=()
        mapfile -t pkg_version_files < <(find "$CI_BUILD_DIR" -maxdepth 1 -name '*ConfigVersion.cmake' -type f | sort)
        if [ "${#pkg_version_files[@]}" -eq 0 ]; then
            ci_fail version "CMakeLists.txt calls write_basic_package_version_file() and $CI_BUILD_DIR (top level) holds no *ConfigVersion.cmake — that copy of the number was NOT checked (run the build stage first, or teach this stage where the file is written)"
        fi
        local pf pkg_version bad_copies=0
        for pf in "${pkg_version_files[@]}"; do
            pkg_version=$(sed -n 's/^set(PACKAGE_VERSION "\([0-9][0-9.]*\)".*/\1/p' "$pf" | head -1)
            if [ -z "$pkg_version" ]; then
                ci_fail version "$pf holds no set(PACKAGE_VERSION \"x.y.z\") — the copy could not be read at all"
            fi
            if [ "$pkg_version" != "$cmake_version" ]; then
                printf '    %s says %s\n' "${pf#"$REPO_ROOT"/}" "$pkg_version"
                bad_copies=$((bad_copies + 1))
            fi
        done
        if [ "$bad_copies" -gt 0 ]; then
            ci_fail version "$bad_copies generated package-version file(s) disagree with project(VERSION) $cmake_version — that file is what a downstream find_package() reads"
        fi
        printf '    %s package-version file(s) also say %s\n' "${#pkg_version_files[@]}" "$cmake_version"
    else
        printf '    (CMakeLists.txt does not call write_basic_package_version_file: no generated package version to compare)\n'
    fi

    # And, optionally, the binaries have to report it: a version nobody can ask for is
    # not a version. Off by default because every project names its executables
    # differently — set CI_VERSION_BINARIES="$CI_BUILD_DIR/myapp*" in .ci.env when the
    # naming is stable (see .ci.env.example).
    if [ -n "${CI_VERSION_BINARIES:-}" ]; then
        local binary reported checked=0 wrong=0
        # eval, exactly like CI_TEST_CMD: the documented value is "$CI_BUILD_DIR/myapp",
        # and a plain word-split would carry that through as an unexpanded literal, match
        # no file, and then print "0 binary/binaries report <version>" -- a PASS. That is
        # the fails-open shape this script exists to prevent, so the miss is also a FAIL
        # below.
        local binary_list
        eval "binary_list=\"$CI_VERSION_BINARIES\""
        for binary in $binary_list; do
            [ -f "$binary" ] && [ -x "$binary" ] || continue
            reported=$("$binary" --version 2>&1 | head -1)
            case "$reported" in
                *"$cmake_version"*) checked=$((checked + 1)) ;;
                *) wrong=$((wrong + 1)); printf '      %s --version printed: %s\n' "$(basename "$binary")" "$reported" ;;
            esac
        done
        if [ "$wrong" -gt 0 ]; then
            ci_fail version "$wrong binary/binaries do not report a --version containing $cmake_version"
        fi
        if [ "$checked" -eq 0 ]; then
            ci_fail version "CI_VERSION_BINARIES=\"$CI_VERSION_BINARIES\" matched no executable — the binaries were NOT checked"
        fi
        printf '    %s binary/binaries report %s\n' "$checked" "$cmake_version"
    else
        printf '    (CI_VERSION_BINARIES unset: the binaries were not asked for --version)\n'
    fi
    ci_pass version
}

stage_asan() {
    ci_begin "asan (ASan+UBSan, separate build dir)"
    # shellcheck disable=SC2086
    cmake -S . -B "$CI_ASAN_BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
        ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/asan-configure.log" 2>&1 \
        || ci_fail asan "cmake configure failed" "$CI_LOG_DIR/asan-configure.log"
    cmake --build "$CI_ASAN_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/asan-build.log" 2>&1 \
        || ci_fail asan "sanitizer build failed" "$CI_LOG_DIR/asan-build.log"
    # CI_ASAN_TEST_CMD: optional, for tests that cannot run under sanitizers at all
    # (RSS-based leak heuristics, wall-clock benchmarks). Defaults to CI_TEST_CMD, so
    # without it the sanitizer run is the same command in a different build dir.
    local saved="$CI_TEST_CMD"
    # shellcheck disable=SC2086
    CI_TEST_CMD="$(printf '%s' "${CI_ASAN_TEST_CMD:-$CI_TEST_CMD}" | sed "s|\$CI_BUILD_DIR|$CI_ASAN_BUILD_DIR|g")"
    if ! UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ASAN_OPTIONS=detect_leaks=1 \
        run_tests "$CI_ASAN_BUILD_DIR" "$CI_LOG_DIR/asan-tests.log"; then
        grep -E "ERROR: AddressSanitizer|runtime error|FAILED" "$CI_LOG_DIR/asan-tests.log" | head -20 | sed 's/^/      /'
        ci_fail asan "ASan/UBSan reported something" "$CI_LOG_DIR/asan-tests.log"
    fi
    CI_TEST_CMD="$saved"
    printf '    clean under ASan+UBSan\n'
    ci_pass asan
}

stage_tsan() {
    ci_begin "tsan (ThreadSanitizer, separate build dir)"
    # A thread sanitizer on a single-threaded suite still catches static/shared state
    # touched from more than one thread — the bug this gate exists for.
    # shellcheck disable=SC2086
    cmake -S . -B "$CI_TSAN_BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g" \
        ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/tsan-configure.log" 2>&1 \
        || ci_fail tsan "cmake configure failed" "$CI_LOG_DIR/tsan-configure.log"
    cmake --build "$CI_TSAN_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/tsan-build.log" 2>&1 \
        || ci_fail tsan "ThreadSanitizer build failed" "$CI_LOG_DIR/tsan-build.log"
    local saved="$CI_TEST_CMD"
    CI_TEST_CMD="$(printf '%s' "$saved" | sed "s|\$CI_BUILD_DIR|$CI_TSAN_BUILD_DIR|g")"
    if ! TSAN_OPTIONS=halt_on_error=1 run_tests "$CI_TSAN_BUILD_DIR" "$CI_LOG_DIR/tsan.log"; then
        grep -E "WARNING: ThreadSanitizer|data race|FAILED" "$CI_LOG_DIR/tsan.log" | head -20 | sed 's/^/      /'
        ci_fail tsan "ThreadSanitizer reported a data race (or a wrong answer)" "$CI_LOG_DIR/tsan.log"
    fi
    CI_TEST_CMD="$saved"
    printf '    no data races reported\n'
    ci_pass tsan
}

stage_fuzz() {
    ci_begin "fuzz (libFuzzer, round-trip oracle, ${CI_FUZZ_SECONDS}s)"
    # libFuzzer is clang's (-fsanitize=fuzzer); there is no GCC equivalent, so a machine
    # without clang++ SKIPs — same rule as clang-format and clang-tidy.
    require_tool clang++ fuzz || return 0
    local started
    started=$(date +%s)

    # libFuzzer REFUSES TO START when a corpus directory does not exist, and a fresh
    # clone has none (fuzz/corpus/ is gitignored on purpose: it is generated).
    mkdir -p "$CI_FUZZ_CORPUS"

    # Never leave CMAKE_BUILD_TYPE empty here: an empty build type is -O0, which is 5-20x
    # less fuzzing for the same ten seconds.
    # shellcheck disable=SC2086
    cmake -S . -B "$CI_FUZZ_BUILD_DIR" \
        -DPERMUTO_BUILD_FUZZING=ON \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DPERMUTO_BUILD_TESTS=OFF \
        -DPERMUTO_BUILD_EXAMPLES=OFF \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/fuzz-configure.log" 2>&1 \
        || ci_fail fuzz "cmake configure failed (the harness build is clang-only)" "$CI_LOG_DIR/fuzz-configure.log"
    cmake --build "$CI_FUZZ_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/fuzz-build.log" 2>&1 \
        || ci_fail fuzz "the fuzz harness did not build" "$CI_LOG_DIR/fuzz-build.log"

    local binary="$CI_FUZZ_BUILD_DIR/permuto_fuzz"
    if [ ! -x "$binary" ]; then
        ci_fail fuzz "$binary was not produced — nothing was fuzzed" "$CI_LOG_DIR/fuzz-build.log"
    fi

    # ---- the live-oracle guard ---------------------------------------------------
    # A harness that never reaches its property is indistinguishable from one that
    # always passes. -runs=0 executes every seed once and nothing else;
    # PERMUTO_FUZZ_REQUIRE_IDENTITY=1 makes the binary exit non-zero when not one of
    # them reached the round-trip assert.
    if ! PERMUTO_FUZZ_REQUIRE_IDENTITY=1 "$binary" fuzz/seeds -runs=0 \
        -artifact_prefix="$CI_FUZZ_CORPUS/" > "$CI_LOG_DIR/fuzz-seeds.log" 2>&1; then
        grep -E "DEAD ORACLE|permuto fuzz:|ERROR|SUMMARY" "$CI_LOG_DIR/fuzz-seeds.log" \
            | head -10 | sed 's/^/      /'
        ci_fail fuzz "the seed smoke failed: fuzz/seeds could not exercise the round-trip oracle (or it found something)" "$CI_LOG_DIR/fuzz-seeds.log"
    fi
    local checks
    checks=$(sed -n 's/.*identity_checks=\([0-9][0-9]*\).*/\1/p' "$CI_LOG_DIR/fuzz-seeds.log" | tail -1)
    if [ -z "$checks" ] || [ "$checks" -eq 0 ] 2>/dev/null; then
        ci_fail fuzz "the seed smoke reported no identity_checks — the oracle is dead and a pass would certify nothing" "$CI_LOG_DIR/fuzz-seeds.log"
    fi
    printf '    seed smoke: %s input(s) reached the round-trip assert\n' "$checks"
    grep -E '^#[0-9]+[[:space:]]+(INITED|DONE)' "$CI_LOG_DIR/fuzz-seeds.log" | sed 's/^/      /'

    # ---- the timed campaign ------------------------------------------------------
    # $CI_FUZZ_CORPUS first (that is the corpus libFuzzer WRITES to and grows), then the
    # tracked seed corpus, then fuzz/regressions — the reproducers of defects this harness
    # actually found, which are replayed on every run so a fixed bug cannot come back
    # unnoticed. That is the standing rule in INCIDENTS.md: the artifact becomes a test.
    # -artifact_prefix or the reproducer lands in the CWD where nothing that reports
    # failures will look for it.
    if ! "$binary" "$CI_FUZZ_CORPUS" fuzz/seeds fuzz/regressions \
        -max_total_time="$CI_FUZZ_SECONDS" \
        -artifact_prefix="$CI_FUZZ_CORPUS/" \
        -print_final_stats=1 > "$CI_LOG_DIR/fuzz.log" 2>&1; then
        grep -E "ROUND-TRIP MISMATCH|permuto fuzz:|ERROR:|SUMMARY:|Test unit written to" \
            "$CI_LOG_DIR/fuzz.log" | head -20 | sed 's/^/      /'
        local artifact
        artifact=$(sed -n 's/.*Test unit written to \(.*\)$/\1/p' "$CI_LOG_DIR/fuzz.log" | tail -1)
        if [ -n "$artifact" ]; then
            printf '    reproducer: %s\n' "$artifact"
            printf '    replay it:  %s %s\n' "$binary" "$artifact"
        fi
        ci_fail fuzz "libFuzzer reported a finding — turn the reproducer above into a regression test" "$CI_LOG_DIR/fuzz.log"
    fi
    grep -E '^stat::|^permuto fuzz:' "$CI_LOG_DIR/fuzz.log" | sed 's/^/      /'
    printf '    %ss campaign clean (corpus %s + fuzz/seeds + fuzz/regressions), stage took %ss\n' \
        "$CI_FUZZ_SECONDS" "$CI_FUZZ_CORPUS" "$(( $(date +%s) - started ))"
    ci_pass fuzz
}

stage_tidy() {
    ci_begin "tidy (clang-tidy, only NEW findings)"
    require_tool clang-tidy tidy || return 0
    if [ ! -f "$CI_BUILD_DIR/compile_commands.json" ]; then
        ci_fail tidy "no $CI_BUILD_DIR/compile_commands.json — configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON (the build stage does) before tidy can say anything"
    fi
    local -a sources=()
    mapfile -t sources < <(ci_tidy_sources)
    if [ "${#sources[@]}" -eq 0 ]; then
        ci_fail tidy "no sources matched $CI_SOURCE_GLOBS — tidy would analyse nothing and still pass"
    fi

    # A compile database that exists is not a compile database that covers this repo:
    # CMake can write one that holds a dependency's translation units and none of ours.
    # clang-tidy then analyses nothing, finds no header, and still prints a result.
    local uncovered=0 src
    for src in "${sources[@]}"; do
        if ! grep -qF "\"$REPO_ROOT/$src\"" "$CI_BUILD_DIR/compile_commands.json"; then
            printf '    not in the compile database: %s\n' "$src"
            uncovered=$((uncovered + 1))
        fi
    done
    if [ "$uncovered" -gt 0 ]; then
        ci_fail tidy "$uncovered of ${#sources[@]} sources are missing from $CI_BUILD_DIR/compile_commands.json — the result would be a lie"
    fi
    printf '    compile database covers all %s sources\n' "${#sources[@]}"

    printf '%s\n' "${sources[@]}" \
        | xargs -P "$CI_JOBS" -n 1 clang-tidy -p "$CI_BUILD_DIR" > "$CI_LOG_DIR/tidy.log" 2>&1
    local findings
    findings=$(grep -cE 'warning:|error:' "$CI_LOG_DIR/tidy.log" || true)
    if [ "${findings:-0}" -gt 0 ] && [ -f "$CI_TIDY_BASELINE" ]; then
        local new_findings
        # Both sides go through tidy_key, and both sides are de-duplicated: one key per
        # distinct finding. A baseline captured by `--write-tidy-baseline` is already in
        # that form; anything else in the file is normalised here rather than trusted.
        new_findings=$(comm -13 \
            <(tidy_key < "$CI_TIDY_BASELINE" | sort -u) \
            <(grep -E "warning:|error:" "$CI_LOG_DIR/tidy.log" | tidy_key | sort -u) | wc -l)
        if [ "$new_findings" -gt 0 ]; then
            ci_fail tidy "$new_findings new finding(s) vs $CI_TIDY_BASELINE" "$CI_LOG_DIR/tidy.log"
        fi
        printf '    no new findings vs %s (%s total)\n' "$CI_TIDY_BASELINE" "$findings"
    elif [ "${findings:-0}" -gt 0 ]; then
        grep -E 'warning:|error:' "$CI_LOG_DIR/tidy.log" | sed 's/^/      /' | head -20
        ci_fail tidy "$findings finding(s) and no baseline file — accept them in one step with 'tools/ci.sh --write-tidy-baseline', or fix them; see .ci.env.example" "$CI_LOG_DIR/tidy.log"
    else
        printf '    %s files clean\n' "${#sources[@]}"
    fi
    ci_pass tidy
}

# Capture the accepted-findings baseline. Not a stage: the list is compared against every
# finding in the log, not against the outcome of the run, and the run is EXPECTED to fail
# while there is no baseline yet. It runs the real stages as a child so there is exactly one
# definition of what a finding is (tidy_key) and of where tidy's log comes from.
write_tidy_baseline() {
    printf '\n\033[1m==> write the tidy baseline\033[0m (%s)\n' "$CI_TIDY_BASELINE"
    printf '    running the real build + tidy stages (build first: tidy refuses to run without\n'
    printf '    %s/compile_commands.json; treat the tidy failure below as expected)\n\n' "$CI_BUILD_DIR"
    bash "$0" build tidy > "$CI_LOG_DIR/write-baseline.log" 2>&1
    if [ ! -f "$CI_LOG_DIR/tidy.log" ]; then
        printf 'no %s was written, so there is nothing to capture — the run stopped before the tidy stage:\n' "$CI_LOG_DIR/tidy.log" >&2
        tail -n 20 "$CI_LOG_DIR/write-baseline.log" | sed 's/^/      /' >&2
        printf '    full log: %s\n' "$CI_LOG_DIR/write-baseline.log" >&2
        exit 1
    fi
    mkdir -p "$(dirname "$CI_TIDY_BASELINE")"
    grep -E 'warning:|error:' "$CI_LOG_DIR/tidy.log" | tidy_key | sort -u > "$CI_TIDY_BASELINE"
    local accepted
    accepted=$(grep -c . "$CI_TIDY_BASELINE" || true)
    printf '    %s finding(s) accepted into %s:\n' "${accepted:-0}" "$CI_TIDY_BASELINE"
    head -20 "$CI_TIDY_BASELINE" | sed 's/^/      /'
    if [ "${accepted:-0}" -eq 0 ]; then
        printf '\nNothing to accept: the tidy stage is clean, so delete %s and keep the stage strict.\n' "$CI_TIDY_BASELINE"
        exit 0
    fi
    printf '\nThis is NOT a gate pass: from now on the tidy stage tolerates exactly these findings\n'
    printf 'and fails on anything else. Commit the file — it is this repo'"'"'s accepted-findings list,
'
    printf 'and the tree stage fails on a file that is neither committed nor ignored:\n'
    printf '    git add %s && git commit -m "ci: accept the inherited clang-tidy findings"\n' "$CI_TIDY_BASELINE"
}

stage_pristine() {
    ci_begin "pristine (does the COMMITTED tree build on its own?)"
    local tmp
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/ci-pristine-XXXXXX")
    RUN_TMP_DIRS+=("$tmp")
    git archive HEAD | tar -x -C "$tmp" || ci_fail pristine "git archive HEAD failed"
    printf '    HEAD checked out: %s files\n' "$(find "$tmp" -type f | wc -l)"

    # shellcheck disable=SC2086
    cmake -S "$tmp" -B "$tmp/build" -DCMAKE_BUILD_TYPE="$CI_BUILD_TYPE" \
        ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/pristine-configure.log" 2>&1 \
        || ci_fail pristine "a fresh checkout of HEAD does not even configure (a needed file is not committed)" "$CI_LOG_DIR/pristine-configure.log"
    cmake --build "$tmp/build" -j "$CI_JOBS" > "$CI_LOG_DIR/pristine-build.log" 2>&1 \
        || ci_fail pristine "a fresh checkout of HEAD does not build" "$CI_LOG_DIR/pristine-build.log"
    local saved="$CI_TEST_CMD"
    CI_TEST_CMD="$(printf '%s' "$saved" | sed "s|\$CI_BUILD_DIR|$tmp/build|g")"
    if ! run_tests "$tmp/build" "$CI_LOG_DIR/pristine-tests.log"; then
        ci_fail pristine "tests fail on a fresh checkout of HEAD" "$CI_LOG_DIR/pristine-tests.log"
    fi
    CI_TEST_CMD="$saved"
    tail -n 2 "$CI_LOG_DIR/pristine-tests.log" | sed 's/^/      /'
    if [ "$CI_KEEP_TMP" = "1" ]; then
        printf '    kept: %s\n' "$tmp"
    else
        rm -rf "$tmp"
    fi
    ci_pass pristine
}

stage_package() {
    ci_begin "package (can a consumer find_package(Permuto) and link it?)"
    # The install tree and its CMake package files are a promise to people who are not in this
    # repo, and until 2026-09-20 nothing checked it: PermutoConfig.cmake.in starts with
    # @PACKAGE_INIT@ but the file was written with configure_file(), which leaves that placeholder
    # EMPTY — so every downstream find_package(Permuto) died on "Unknown CMake command
    # check_required_components". The library and headers installed perfectly; the package was
    # installable and unusable (INCIDENTS.md). A two-file consumer project is the check.
    if [ ! -f "$CI_BUILD_DIR/CMakeCache.txt" ]; then
        ci_fail package "no configured $CI_BUILD_DIR — run the build stage first (this stage installs what it built)"
    fi
    # The consumer resolves nlohmann/json through PermutoConfig.cmake's find_dependency, so this
    # needs a system copy; with none it SKIPs and says so rather than pretending.
    if [ -z "$(sed -n 's/^nlohmann_json_DIR:PATH=//p' "$CI_BUILD_DIR/CMakeCache.txt" 2>/dev/null)" ]; then
        ci_skip package "no system nlohmann/json in $CI_BUILD_DIR — a consumer of this install tree needs one (find_dependency)"
        return 0
    fi

    local prefix tmp out
    prefix=$(mktemp -d "${TMPDIR:-/tmp}/ci-package-prefix-XXXXXX")
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/ci-package-consumer-XXXXXX")
    RUN_TMP_DIRS+=("$prefix" "$tmp")

    cmake --install "$CI_BUILD_DIR" --prefix "$prefix" > "$CI_LOG_DIR/package-install.log" 2>&1 \
        || ci_fail package "cmake --install failed on the built tree" "$CI_LOG_DIR/package-install.log"
    printf '    installed: %s\n' "$(cd "$prefix" && find . -name '*.cmake' -path '*Permuto*' | sort | tr '\n' ' ')"

    cat > "$tmp/CMakeLists.txt" <<'CONSUMER'
cmake_minimum_required(VERSION 3.15)
project(permuto_consumer CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Permuto REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE Permuto::permuto)
CONSUMER
    cat > "$tmp/main.cpp" <<'CONSUMER'
#include <permuto/permuto.hpp>
#include <iostream>
int main() {
    const auto templ = nlohmann::json::parse(R"({"greeting": "${/who}"})");
    const auto ctx = nlohmann::json::parse(R"({"who": "consumer"})");
    std::cout << permuto::apply(templ, ctx).dump() << '\n';
    return 0;
}
CONSUMER
    cmake -S "$tmp" -B "$tmp/build" -DCMAKE_PREFIX_PATH="$prefix" \
        > "$CI_LOG_DIR/package-configure.log" 2>&1 \
        || ci_fail package "a consumer project cannot find_package(Permuto) from the installed tree" "$CI_LOG_DIR/package-configure.log"
    cmake --build "$tmp/build" -j "$CI_JOBS" > "$CI_LOG_DIR/package-build.log" 2>&1 \
        || ci_fail package "a consumer cannot compile and link against the installed tree" "$CI_LOG_DIR/package-build.log"
    if ! out=$("$tmp/build/consumer" 2> "$CI_LOG_DIR/package-run.log"); then
        ci_fail package "the consumer built but failed at run time" "$CI_LOG_DIR/package-run.log"
    fi
    if [ "$out" != '{"greeting":"consumer"}' ]; then
        ci_fail package "the consumer ran and printed '$out', not the substitution the library promises"
    fi
    printf '    consumer: find_package(Permuto) -> apply() -> %s\n' "$out"
    [ "$CI_KEEP_TMP" = "1" ] || rm -rf "$prefix" "$tmp"
    ci_pass package
}

cleanup() {
    local dir
    if [ "$CI_KEEP_TMP" != "1" ]; then
        for dir in "${RUN_TMP_DIRS[@]:-}"; do
            [ -n "$dir" ] && [ -d "$dir" ] && rm -rf "$dir"
        done
    fi
}
trap cleanup EXIT

# ---------------------------------------------------------------- kit probes
# A fix that must propagate ships a probe (docs/KIT-REVISION-CONVENTION.md). Each script
# in tools/kit-probes/ holds this gate to ONE kit fix's contract — name and behaviour, not
# bytes — and exits non-zero when the fix is absent. The directory IS the list of fixes
# this copy claims to carry, so absence fails HERE, in under a second, on the machine that
# would otherwise push the lag: no kit checkout, no network, no build.
#
# Why not a hash or a diff against the kit: the copies of this file are forks (a repo's
# adapted stages, its own defaults, 60-586 differing lines), and diff SIZE measures
# divergence, not lateness — a one-fix-behind copy is missing 27 kit lines while a
# verified current record is missing 125. See the convention for the measurement.
#
# It is name- and contract-level: a semantic regression INSIDE a function that is still
# present is not caught. That needs a real build and a real run, which is what the port
# did by hand; the probe is the cheap net, not the whole net.
stage_kitprobes() {
    ci_begin "kit probes (the fixes this copy claims to carry)"
    local self probe name failed=0
    self="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
    if [ ! -d "$REPO_ROOT/tools/kit-probes" ]; then
        ci_skip kitprobes "no tools/kit-probes/ — this copy carries no kit probe yet"
        return 0
    fi
    for probe in "$REPO_ROOT"/tools/kit-probes/*.sh; do
        [ -f "$probe" ] || continue
        name="$(basename "$probe")"
        if bash "$probe" "$self" "$REPO_ROOT"; then
            printf '    ok   %s\n' "$name"
        else
            printf '    FAIL %s — this gate is missing that kit fix\n' "$name"
            failed=1
        fi
    done
    if [ "$failed" = "1" ]; then
        ci_fail kitprobes "a probe failed: this copy is behind a kit fix — port it from the kit (tools/kit-probes/ names which)"
    fi
    printf '    every probe in tools/kit-probes/ verified against this gate\n'
    ci_pass kitprobes
}

# ---------------------------------------------------------------- dispatch
while [ $# -gt 0 ]; do
    case "$1" in
        --help|-h) usage; exit 0 ;;
        --list)
            printf 'default stages: %s\n' "$CI_DEFAULT_STAGES"
            # Derived from the defined functions, so this cannot drift from the stages
            # the script actually implements.
            printf 'stages:'
            for fn in $(declare -F | awk '{print $3}' | grep '^stage_' | sed 's/^stage_//' | sort); do
                printf ' %s' "$fn"
            done
            printf '\n'
            exit 0 ;;
        --require-clean) REQUIRE_CLEAN=1 ;;
        --allow-untracked) ALLOW_UNTRACKED=1 ;;
        --strict-tools) CI_STRICT_TOOLS=1 ;;
        --write-tidy-baseline) WRITE_TIDY_BASELINE=1 ;;
        -*) printf 'unknown option: %s (try --help)\n' "$1" >&2; exit 2 ;;
        fast) STAGES_REQUESTED+=($CI_FAST_STAGES) ;;
        full) STAGES_REQUESTED+=($CI_FULL_STAGES) ;;
        *) STAGES_REQUESTED+=("$1") ;;
    esac
    shift
done

# Accepting inherited findings is its own mode, not a stage: it produces no verdict about
# the tree, only a file (and it exits before the summary so it can never print
# "GATE PASSED" about a run whose tidy stage failed on purpose).
if [ "$WRITE_TIDY_BASELINE" = "1" ]; then
    write_tidy_baseline
    exit 0
fi

if [ ${#STAGES_REQUESTED[@]} -eq 0 ]; then
    # shellcheck disable=SC2206
    STAGES_REQUESTED=($CI_DEFAULT_STAGES)
fi

printf '\033[1mlocal CI\033[0m — %s stage(s): %s\n' "${#STAGES_REQUESTED[@]}" "${STAGES_REQUESTED[*]}"
START=$(date +%s)
for stage in "${STAGES_REQUESTED[@]}"; do
    if ! declare -F "stage_$stage" > /dev/null; then
        printf 'unknown stage: %s (try --list)\n' "$stage" >&2
        exit 2
    fi
    # A stage that returns non-zero without reporting a verdict is not a pass, and a stage that
    # dies from a shell error cannot report anything at all — so neither is left to the summary.
    if ! "stage_$stage"; then
        FAILED_STAGE="$stage"
        summary
        printf 'FAILED: %s exited non-zero without reporting a verdict\n' "$stage" >&2
        printf '\nGATE FAILED\n' >&2
        exit 1
    fi
    RAN_STAGES+=("$stage")
done
ELAPSED=$(( $(date +%s) - START ))

# The verdict comes from what RAN, not from what was requested. A shell error can unwind out of
# the loop above without either guard seeing it — measured 2026-09-20 on Computo's fork: `set -u`
# plus `local -a sources` (declared, never filled) made "${#sources[@]}" an unbound-variable
# error, which aborted stage_format and the dispatch loop together, and the run then printed
# "all 10 stage(s) passed ... GATE PASSED" after executing one stage of ten (INCIDENTS.md). This
# comparison is the backstop for that whole class: if any requested stage did not run, the run
# fails.
if [ "${#RAN_STAGES[@]}" -ne "${#STAGES_REQUESTED[@]}" ]; then
    summary
    printf 'FAILED: %s of %s stage(s) did not run — the run ended early\n' \
        "$(( ${#STAGES_REQUESTED[@]} - ${#RAN_STAGES[@]} ))" "${#STAGES_REQUESTED[@]}" >&2
    printf '  ran: %s\n' "${RAN_STAGES[*]:-none}" >&2
    printf '\nGATE FAILED\n' >&2
    exit 1
fi

summary
printf '\nall %s stage(s) passed in %ss\nGATE PASSED\n' "${#STAGES_REQUESTED[@]}" "$ELAPSED"
