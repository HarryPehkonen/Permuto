#!/usr/bin/env bash
#
# fuzz — libFuzzer against this library's own documented round-trip oracle, for CI_FUZZ_SECONDS.
#
# clang++ is guaranteed to exist: gate.toml declares `when = "tool:clang++"` on this stage
# (libFuzzer ships as clang's -fsanitize=fuzzer and has no GCC equivalent).
#
# Three things here are measured fixes, not decoration (INCIDENTS.md, and the adaptation block
# the old tools/ci.sh carried — this is the same code and the same invocations):
#   * the build is RelWithDebInfo, NOT Debug: an unoptimised harness buys far less exploration
#     per second, and CI_FUZZ_SECONDS is small on purpose;
#   * the corpus directory is created first, because libFuzzer REFUSES TO START when it does
#     not exist and a fresh clone has none (fuzz/corpus is gitignored by design);
#   * the SEED SMOKE runs first with PERMUTO_FUZZ_REQUIRE_IDENTITY=1, and the stage fails when
#     the seed run reached no identity check — that check is the live-oracle guard, and it
#     lives in the harness (both readings must be non-zero there). A harness that cannot reach
#     its property looks exactly like one that always passes.
#
# Ten seconds is a REGRESSION budget for a push prompt, not a campaign. Long campaigns are
# manual and out of band, against the same corpus: CI_FUZZ_SECONDS=1800 scripts/fuzz.sh
#
# Called from gate.toml as `[stage.fuzz] cmd = "scripts/fuzz.sh"` (timeout = 900).
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

started=$(date +%s)
mkdir -p "$CI_FUZZ_CORPUS"

# shellcheck disable=SC2086
if ! cmake -S . -B "$CI_FUZZ_BUILD_DIR" \
    -DPERMUTO_BUILD_FUZZING=ON \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DPERMUTO_BUILD_TESTS=OFF \
    -DPERMUTO_BUILD_EXAMPLES=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    ${CI_CMAKE_FLAGS:-} > "$CI_LOG_DIR/fuzz-configure.log" 2>&1; then
    show_log "$CI_LOG_DIR/fuzz-configure.log"
    fail "fuzz: cmake configure failed (the harness build is clang-only)"
fi
if ! cmake --build "$CI_FUZZ_BUILD_DIR" -j "$CI_JOBS" > "$CI_LOG_DIR/fuzz-build.log" 2>&1; then
    show_log "$CI_LOG_DIR/fuzz-build.log"
    fail "fuzz: the fuzz harness did not build"
fi

binary="$CI_FUZZ_BUILD_DIR/permuto_fuzz"
if [ ! -x "$binary" ]; then
    show_log "$CI_LOG_DIR/fuzz-build.log"
    fail "fuzz: $binary was not produced — nothing was fuzzed"
fi

# ---- the live-oracle guard ---------------------------------------------------
# -runs=0 executes every seed once and nothing else; PERMUTO_FUZZ_REQUIRE_IDENTITY=1 makes the
# binary exit non-zero when not one of them reached the round-trip assert.
if ! PERMUTO_FUZZ_REQUIRE_IDENTITY=1 "$binary" fuzz/seeds -runs=0 \
    -artifact_prefix="$CI_FUZZ_CORPUS/" > "$CI_LOG_DIR/fuzz-seeds.log" 2>&1; then
    grep -E "DEAD ORACLE|permuto fuzz:|ERROR|SUMMARY" "$CI_LOG_DIR/fuzz-seeds.log" | head -10
    show_log "$CI_LOG_DIR/fuzz-seeds.log"
    fail "fuzz: the seed smoke failed: fuzz/seeds could not exercise the round-trip oracle (or it found something)"
fi
checks=$(sed -n 's/.*identity_checks=\([0-9][0-9]*\).*/\1/p' "$CI_LOG_DIR/fuzz-seeds.log" | tail -1)
if [ -z "$checks" ] || [ "$checks" -eq 0 ] 2>/dev/null; then
    fail "fuzz: the seed smoke reported no identity_checks — the oracle is dead and a pass would certify nothing"
fi
printf 'seed smoke: %s input(s) reached the round-trip assert\n' "$checks"
grep -E '^#[0-9]+[[:space:]]+(INITED|DONE)' "$CI_LOG_DIR/fuzz-seeds.log"

# ---- the timed campaign ------------------------------------------------------
# $CI_FUZZ_CORPUS first (that is the corpus libFuzzer WRITES to and grows), then the tracked
# seed corpus, then fuzz/regressions — the reproducers of defects this harness actually found,
# replayed on every run so a fixed bug cannot come back unnoticed (the standing rule in
# INCIDENTS.md: the artifact becomes a test). -artifact_prefix or the reproducer lands in the
# CWD where nothing that reports failures will look for it.
if ! "$binary" "$CI_FUZZ_CORPUS" fuzz/seeds fuzz/regressions \
    -max_total_time="$CI_FUZZ_SECONDS" \
    -artifact_prefix="$CI_FUZZ_CORPUS/" \
    -print_final_stats=1 > "$CI_LOG_DIR/fuzz.log" 2>&1; then
    grep -E "ROUND-TRIP MISMATCH|permuto fuzz:|ERROR:|SUMMARY:|Test unit written to" "$CI_LOG_DIR/fuzz.log" | head -20
    artifact=$(sed -n 's/.*Test unit written to \(.*\)$/\1/p' "$CI_LOG_DIR/fuzz.log" | tail -1)
    if [ -n "$artifact" ]; then
        printf 'reproducer: %s\nreplay it:  %s %s\n' "$artifact" "$binary" "$artifact"
    fi
    show_log "$CI_LOG_DIR/fuzz.log"
    fail "fuzz: libFuzzer reported a finding — turn the reproducer above into a regression test"
fi
grep -E '^stat::|^permuto fuzz:' "$CI_LOG_DIR/fuzz.log"
printf '%ss campaign clean (corpus %s + fuzz/seeds + fuzz/regressions), stage took %ss\n' \
    "$CI_FUZZ_SECONDS" "$CI_FUZZ_CORPUS" "$(( $(date +%s) - started ))"
