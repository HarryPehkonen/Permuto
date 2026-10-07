#!/usr/bin/env bash
#
# tsan — the same suite under ThreadSanitizer, in its own build dir. A thread sanitizer on a
# single-threaded suite still catches static/shared state touched from more than one thread,
# which is the bug this stage exists for.
#
# Called from gate.toml as `[stage.tsan] cmd = "scripts/tsan.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

# shellcheck disable=SC2086
if ! cmake -S . -B "$CI_TSAN_BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g" \
    ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/tsan-configure.log" 2>&1; then
    show_log "$CI_LOG_DIR/tsan-configure.log"
    fail "tsan: cmake configure failed"
fi

if ! cmake --build "$CI_TSAN_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/tsan-build.log" 2>&1; then
    show_log "$CI_LOG_DIR/tsan-build.log"
    fail "tsan: ThreadSanitizer build failed"
fi

saved="$CI_TEST_CMD"
# shellcheck disable=SC2086
CI_TEST_CMD="$(printf '%s' "$saved" | sed "s|\$CI_BUILD_DIR|$CI_TSAN_BUILD_DIR|g")"
if ! TSAN_OPTIONS=halt_on_error=1 eval "$CI_TEST_CMD" > "$CI_LOG_DIR/tsan.log" 2>&1; then
    CI_TEST_CMD="$saved"
    grep -E "WARNING: ThreadSanitizer|data race|FAILED" "$CI_LOG_DIR/tsan.log" | head -20
    show_log "$CI_LOG_DIR/tsan.log"
    fail "tsan: ThreadSanitizer reported a data race (or a wrong answer)"
fi
CI_TEST_CMD="$saved"
printf 'no data races reported\n'
