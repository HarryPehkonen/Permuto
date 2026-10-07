#!/usr/bin/env bash
#
# pristine — git archive HEAD into a temp dir, then configure, build and run the suite there:
# proves the COMMITTED tree is complete (it catches files that are uncommitted or ignored).
# It therefore certifies nothing about uncommitted work; while a change is still in the working
# tree, run the other stages and say so.
#
# Called from gate.toml as `[stage.pristine] cmd = "scripts/pristine.sh"` (timeout = 900).
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

tmp=$(mktemp -d "${TMPDIR:-/tmp}/ci-pristine-XXXXXX")
cleanup() {
    [ "$CI_KEEP_TMP" = "1" ] || rm -rf "$tmp"
}
trap cleanup EXIT

git archive HEAD | tar -x -C "$tmp" || fail "pristine: git archive HEAD failed"
printf 'HEAD checked out: %s files\n' "$(find "$tmp" -type f | wc -l)"

# shellcheck disable=SC2086
if ! cmake -S "$tmp" -B "$tmp/build" -DCMAKE_BUILD_TYPE="$CI_BUILD_TYPE" \
    ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/pristine-configure.log" 2>&1; then
    show_log "$CI_LOG_DIR/pristine-configure.log"
    fail "pristine: a fresh checkout of HEAD does not even configure (a needed file is not committed)"
fi
if ! cmake --build "$tmp/build" -j "$CI_JOBS" > "$CI_LOG_DIR/pristine-build.log" 2>&1; then
    show_log "$CI_LOG_DIR/pristine-build.log"
    fail "pristine: a fresh checkout of HEAD does not build"
fi

saved="$CI_TEST_CMD"
# shellcheck disable=SC2086
CI_TEST_CMD="$(printf '%s' "$saved" | sed "s|\$CI_BUILD_DIR|$tmp/build|g")"
if ! eval "$CI_TEST_CMD" > "$CI_LOG_DIR/pristine-tests.log" 2>&1; then
    CI_TEST_CMD="$saved"
    show_log "$CI_LOG_DIR/pristine-tests.log"
    fail "pristine: tests fail on a fresh checkout of HEAD"
fi
CI_TEST_CMD="$saved"
tail -n 2 "$CI_LOG_DIR/pristine-tests.log"
if [ "$CI_KEEP_TMP" = "1" ]; then
    printf 'kept: %s\n' "$tmp"
fi
