#!/usr/bin/env bash
#
# build — cmake configure plus build, and a count of the warnings the compiler reported.
#
# -Werror covers the targets it is wired onto, so a target that never got the flag would build
# with warnings and pass. Counting `warning:` in the log is what covers that hole.
#
# Called from gate.toml as `[stage.build] cmd = "scripts/build.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

# shellcheck disable=SC2086
if ! cmake -S . -B "$CI_BUILD_DIR" -DCMAKE_BUILD_TYPE="$CI_BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/configure.log" 2>&1; then
    show_log "$CI_LOG_DIR/configure.log"
    fail "build: cmake configure failed"
fi

if ! cmake --build "$CI_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/build.log" 2>&1; then
    show_log "$CI_LOG_DIR/build.log"
    fail "build: build failed (-Werror is on: a warning is a build failure)"
fi

warns=$(grep -c 'warning:' "$CI_LOG_DIR/build.log" || true)
if [ "${warns:-0}" -gt 0 ]; then
    grep 'warning:' "$CI_LOG_DIR/build.log" | head -5
    fail "build: $warns compiler warning(s) in a target -Werror does not cover"
fi
printf 'built with no warnings\n'
