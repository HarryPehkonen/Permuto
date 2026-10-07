#!/usr/bin/env bash
#
# asan — the same suite under AddressSanitizer + UndefinedBehaviorSanitizer, in its OWN build
# dir: the flags must never touch the ordinary build/.
#
# Called from gate.toml as `[stage.asan] cmd = "scripts/asan.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

# shellcheck disable=SC2086
if ! cmake -S . -B "$CI_ASAN_BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
    ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/asan-configure.log" 2>&1; then
    show_log "$CI_LOG_DIR/asan-configure.log"
    fail "asan: cmake configure failed"
fi

if ! cmake --build "$CI_ASAN_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/asan-build.log" 2>&1; then
    show_log "$CI_LOG_DIR/asan-build.log"
    fail "asan: sanitizer build failed"
fi

# CI_ASAN_TEST_CMD: optional, for tests that cannot run under sanitizers at all (RSS-based leak
# heuristics, wall-clock benchmarks). It defaults to CI_TEST_CMD, so without it the sanitizer
# run is the same command in a different build dir.
saved="$CI_TEST_CMD"
# shellcheck disable=SC2086
CI_TEST_CMD="$(printf '%s' "${CI_ASAN_TEST_CMD:-$CI_TEST_CMD}" | sed "s|\$CI_BUILD_DIR|$CI_ASAN_BUILD_DIR|g")"
if ! UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ASAN_OPTIONS=detect_leaks=1 \
    eval "$CI_TEST_CMD" > "$CI_LOG_DIR/asan-tests.log" 2>&1; then
    CI_TEST_CMD="$saved"
    grep -E "ERROR: AddressSanitizer|runtime error|FAILED" "$CI_LOG_DIR/asan-tests.log" | head -20
    show_log "$CI_LOG_DIR/asan-tests.log"
    fail "asan: ASan/UBSan reported something"
fi
CI_TEST_CMD="$saved"
printf 'clean under ASan+UBSan\n'
