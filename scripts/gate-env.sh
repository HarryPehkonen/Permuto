#!/usr/bin/env bash
#
# The configuration every stage of this gate reads — the .ci.env knobs the old
# tools/ci.sh carried, each with the default it had there. SOURCED, never executed
# (scripts/gate.sh does not need it; the stage scripts all use it).
#
# Permuto's gate is gate.toml (the POLICY: which stages exist, which tier runs which of
# them, how a failure is recognised) run by kit-ci (the ENGINE, installed once per
# machine). Everything a stage needs BEYOND that policy — a build dir, a job count, the
# fuzz budget — lives here, so the policy file stays a list of stages.
#
# History: until 2026-10-06 this was the defaults block of a 1087-line tools/ci.sh
# (card t_6989cec2). The knobs are the same ones with the same defaults, so a machine
# with no .ci.env behaves exactly as it did before the conversion; .ci.env is sourced
# last and still wins over every default here.
set -uo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$REPO_ROOT" || exit 1

# No colour from the tools: several stages grep their output (warning:, error:) and ANSI
# escapes defeat the greps.
export NO_COLOR=1

# git exports GIT_INDEX_FILE to a hook when the commit is made with a PATHSPEC
# (`git commit -- <path>`): it names git's TEMPORARY index for that one commit, not this
# repository's index, and every process the hook starts inherits it. Any `git` command
# run inside ANOTHER repository then reads THIS repo's index entries against that
# repository's object store and dies on the first blob it does not have. Unset once,
# here, so it reaches every stage — this is the kit fix f9c3300, carried in this repo
# since commit 8541d27 as one line in the old gate's prologue. scripts/gate.sh unsets it
# too, so a stage cmd that never sources this file is covered as well.
unset GIT_INDEX_FILE

CI_JOBS=${CI_JOBS:-$(nproc 2>/dev/null || echo 4)}
CI_BUILD_TYPE=${CI_BUILD_TYPE:-Debug}
CI_BUILD_DIR=${CI_BUILD_DIR:-build}
CI_ASAN_BUILD_DIR=${CI_ASAN_BUILD_DIR:-build-asan}
CI_TSAN_BUILD_DIR=${CI_TSAN_BUILD_DIR:-build-tsan}
CI_FUZZ_BUILD_DIR=${CI_FUZZ_BUILD_DIR:-build-fuzz}
# 10 s, because this runs on every push and a push must not cost minutes: a REGRESSION
# budget over an accumulated corpus, not a campaign. Longer campaigns stay manual
# (CI_FUZZ_SECONDS=1800 scripts/fuzz.sh) against the same corpus.
CI_FUZZ_SECONDS=${CI_FUZZ_SECONDS:-10}
CI_FUZZ_CORPUS=${CI_FUZZ_CORPUS:-fuzz/corpus}
CI_LOG_DIR=${CI_LOG_DIR:-.ci-logs}
CI_DOCS_FILES=${CI_DOCS_FILES:-"README.md CLAUDE.md CODING_STANDARDS.md TECHNICAL_DETAILS.md REQUIREMENTS.md"}
CI_SOURCE_GLOBS=${CI_SOURCE_GLOBS:-"'*.cpp' '*.cc' '*.cxx' '*.hpp' '*.hh' '*.h'"}
CI_VERSION_HEADER=${CI_VERSION_HEADER:-"$CI_BUILD_DIR/generated/version.hpp"}
CI_VERSION_BINARIES=${CI_VERSION_BINARIES:-"$CI_BUILD_DIR/permuto"}
CI_TEST_CMD=${CI_TEST_CMD:-"ctest --test-dir \$CI_BUILD_DIR --output-on-failure -j \$CI_JOBS"}
CI_TIDY_BASELINE=${CI_TIDY_BASELINE:-.ci/tidy-baseline.txt}
CI_KEEP_TMP=${CI_KEEP_TMP:-0}
# 1 = the tree stage fails on uncommitted edits to tracked files. The pre-push hook sets
# it. This is the old gate's `--require-clean` flag moved to the call site: kit-ci's
# vocabulary has no per-run flags, and the rule itself still lives in scripts/tree.sh
# (2026-10-06, INCIDENTS.md).
CI_REQUIRE_CLEAN=${CI_REQUIRE_CLEAN:-0}
# 1 = do not fail the tree stage on untracked, unignored files (the old --allow-untracked).
CI_ALLOW_UNTRACKED=${CI_ALLOW_UNTRACKED:-0}
# CI_STRICT_TOOLS (a missing clang-format/clang-tidy/clang++ FAILS instead of skipping) is
# NOT carried and has no default here: kit-ci skips a stage by its own rule,
# `when = "tool:<name>"` in gate.toml, and no key turns that skip into a failure. See
# INCIDENTS.md and .ci.env.example.

if [ -f .ci.env ]; then
    # shellcheck disable=SC1091
    . ./.ci.env
fi

mkdir -p "$CI_LOG_DIR"

# ---------------------------------------------------------------- stage helpers
# A stage's verdict is its exit status now, so the old ci_pass/ci_fail/summary trio is gone:
# kit-ci prints "pass (1.2s)" or "FAIL (<reason>)" per stage and prints the first 40 lines of a
# failed stage's combined output (SPEC.md §4). What a stage still owes the reader is WHY it
# failed, in its own words, and the last lines of the log it wrote.
fail() {
    printf '%s\n' "$*" >&2
    exit 1
}

show_log() {  # show_log <file> — the tail, for out-of-order output like a build log
    local file="${1:-}"
    if [ -n "$file" ] && [ -f "$file" ]; then
        printf -- '--- %s (last 20 lines) ---\n' "$file" >&2
        tail -n 20 "$file" | sed 's/^/    /' >&2
    fi
}

# The comparable form of a clang-tidy finding — applied to BOTH sides of the baseline
# comparison, so the file a repo captures and the log the tidy stage just wrote are the same
# shape:
#
#   <repo>/src/foo.cpp:42:7: warning: ...   ->   src/foo.cpp: warning: ...
#
#   * the repo root is stripped: clang-tidy reports the path it was handed by the compile
#     database, which CMake writes as an absolute path, so a baseline captured in one clone
#     names no finding in a checkout at another path (the nightly clean checkout, a
#     colleague's machine) and every inherited finding reads as new;
#   * :line:column is stripped, so the same finding after an unrelated edit above it is still
#     the same finding. This is line-blind on purpose, and the flip side is worth knowing: a
#     SECOND identical finding in a file that already has one collapses into the first. Fix
#     the baselined finding instead of growing the baseline.
#
# scripts/write-tidy-baseline.sh captures through this same function, so the documented way to
# accept findings cannot drift from the way they are compared. Kit fix `tidy-baseline`
# (carried in this repo since commit cbb7248), whose probe went with the kit probes.
tidy_key() {
    awk -v root="$REPO_ROOT/" '
        { i = index($0, root); if (i) $0 = substr($0, i + length(root)); print }' \
        | sed 's/:[0-9]*:[0-9]*:/:/'
}
