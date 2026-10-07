#!/usr/bin/env bash
#
# Accept the clang-tidy findings this repo has inherited: run the real build and tidy stages and
# capture every finding into CI_TIDY_BASELINE.
#
# NOT a gate stage, and NOT a gate pass: while there is no baseline the tidy stage is EXPECTED
# to fail, so this is a mode a human runs by hand. Until 2026-10-06 it was the
# `--write-tidy-baseline` flag of the old tools/ci.sh; kit-ci's vocabulary has no modes, so the
# mode became this script (card t_6989cec2, INCIDENTS.md).
#
#     scripts/write-tidy-baseline.sh
#
# It runs the real stage scripts as children, so there is exactly one definition of what a
# finding is (tidy_key in scripts/gate-env.sh) and of where tidy's log comes from.
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
. "$here/gate-env.sh"

printf '==> write the tidy baseline (%s)\n' "$CI_TIDY_BASELINE"
printf 'running the real build + tidy stages (treat a tidy failure below as expected)\n\n'
bash "$here/build.sh" || fail "write-tidy-baseline: the build stage failed, so tidy cannot run"
bash "$here/tidy.sh" > "$CI_LOG_DIR/write-baseline.log" 2>&1

if [ ! -f "$CI_LOG_DIR/tidy.log" ]; then
    printf 'no %s was written, so there is nothing to capture — the run stopped before the tidy stage:\n' "$CI_LOG_DIR/tidy.log" >&2
    tail -n 20 "$CI_LOG_DIR/write-baseline.log" | sed 's/^/      /' >&2
    printf '    full log: %s\n' "$CI_LOG_DIR/write-baseline.log" >&2
    exit 1
fi

mkdir -p "$(dirname "$CI_TIDY_BASELINE")"
grep -E 'warning:|error:' "$CI_LOG_DIR/tidy.log" | tidy_key | sort -u > "$CI_TIDY_BASELINE"
accepted=$(grep -c . "$CI_TIDY_BASELINE" || true)
printf '%s finding(s) accepted into %s:\n' "${accepted:-0}" "$CI_TIDY_BASELINE"
head -20 "$CI_TIDY_BASELINE"

if [ "${accepted:-0}" -eq 0 ]; then
    printf '\nNothing to accept: the tidy stage is clean, so delete %s and keep the stage strict.\n' "$CI_TIDY_BASELINE"
    exit 0
fi
printf '\nThis is NOT a gate pass: from now on the tidy stage tolerates exactly these findings\n'
printf 'and fails on anything else. Commit the file — it is this repo'"'"'s accepted-findings list,\n'
printf 'and the tree stage fails on a file that is neither committed nor ignored:\n'
printf '    git add %s && git commit -m "ci: accept the inherited clang-tidy findings"\n' "$CI_TIDY_BASELINE"
