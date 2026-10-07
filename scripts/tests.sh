#!/usr/bin/env bash
#
# tests — the suite, every failure reported.
#
# Called from gate.toml as `[stage.tests] cmd = "scripts/tests.sh"`. The test runner is
# CI_TEST_CMD (scripts/gate-env.sh): ctest over $CI_BUILD_DIR by default, and it is eval'd so a
# project can point it at its own suite instead.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

if [ ! -f "$CI_BUILD_DIR/CMakeCache.txt" ]; then
    fail "tests: no build configured in $CI_BUILD_DIR — run the build stage first"
fi

# shellcheck disable=SC2086
if ! eval "$CI_TEST_CMD" > "$CI_LOG_DIR/tests.log" 2>&1; then
    grep -E "FAILED|Failed|\*\*\*Failed|assert" "$CI_LOG_DIR/tests.log" | head -30
    show_log "$CI_LOG_DIR/tests.log"
    fail "tests: test failures (all of them are above; the full output is in the log)"
fi

grep -E "tests passed|100% tests passed" "$CI_LOG_DIR/tests.log" | tail -1
printf 'the suite passed (the tally is in %s)\n' "$CI_LOG_DIR/tests.log"
