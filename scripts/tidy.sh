#!/usr/bin/env bash
#
# tidy — clang-tidy against this repo's OWN .clang-tidy (the bug-finding subset:
# clang-diagnostic-* + clang-analyzer-*, no style families) and only the findings NEW against
# CI_TIDY_BASELINE. clang-tidy is guaranteed to exist: gate.toml declares
# `when = "tool:clang-tidy"` on this stage.
#
# Two rules here are measurements, not caution (INCIDENTS.md):
#   * a compile database that EXISTS is not one that COVERS this repo. CMake can write one
#     holding a dependency's translation units and none of ours; clang-tidy then analyses
#     nothing, finds no header, and still prints a result. Every source must be in it.
#   * findings compare line-blind and clone-blind through tidy_key() (scripts/gate-env.sh), on
#     BOTH sides of the comparison. That normaliser is kit fix `tidy-baseline`, carried here
#     since commit cbb7248; the kit's probe for it went with the probe stage
#     (2026-10-06, INCIDENTS.md), and scripts/write-tidy-baseline.sh is the documented way to
#     accept findings.
#
# Called from gate.toml as `[stage.tidy] cmd = "scripts/tidy.sh"` (timeout = 900).
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

if [ ! -f "$CI_BUILD_DIR/compile_commands.json" ]; then
    fail "tidy: no $CI_BUILD_DIR/compile_commands.json — configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON (the build stage does) before tidy can say anything"
fi

# clang-tidy needs a compile database entry per translation unit, so headers are not passed:
# diagnostics inside headers still surface through the sources that include them (that is what
# .clang-tidy's HeaderFilterRegex is for).
sources=()
while IFS= read -r f; do
    [ -n "$f" ] && sources+=("$f")
done < <(eval "git ls-files --cached --others --exclude-standard -- $CI_SOURCE_GLOBS" | grep -E '\.(cpp|cc|cxx)$' | sort -u)

if [ "${#sources[@]}" -eq 0 ]; then
    fail "tidy: no sources matched $CI_SOURCE_GLOBS — tidy would analyse nothing and still pass"
fi

uncovered=0
for src in "${sources[@]}"; do
    if ! grep -qF "\"$REPO_ROOT/$src\"" "$CI_BUILD_DIR/compile_commands.json"; then
        printf 'not in the compile database: %s\n' "$src"
        uncovered=$((uncovered + 1))
    fi
done
if [ "$uncovered" -gt 0 ]; then
    fail "tidy: $uncovered of ${#sources[@]} sources are missing from $CI_BUILD_DIR/compile_commands.json — the result would be a lie"
fi
printf 'compile database covers all %s sources\n' "${#sources[@]}"

printf '%s\n' "${sources[@]}" |
    xargs -P "$CI_JOBS" -n 1 clang-tidy -p "$CI_BUILD_DIR" > "$CI_LOG_DIR/tidy.log" 2>&1
findings=$(grep -cE 'warning:|error:' "$CI_LOG_DIR/tidy.log" || true)

if [ "${findings:-0}" -gt 0 ] && [ -f "$CI_TIDY_BASELINE" ]; then
    # Both sides go through tidy_key and both sides are de-duplicated: one key per distinct
    # finding. A baseline captured by scripts/write-tidy-baseline.sh is already in that form;
    # anything else in the file is normalised here rather than trusted.
    new_findings=$(comm -13 \
        <(tidy_key < "$CI_TIDY_BASELINE" | sort -u) \
        <(grep -E "warning:|error:" "$CI_LOG_DIR/tidy.log" | tidy_key | sort -u) | wc -l)
    if [ "$new_findings" -gt 0 ]; then
        show_log "$CI_LOG_DIR/tidy.log"
        fail "tidy: $new_findings new finding(s) vs $CI_TIDY_BASELINE"
    fi
    printf 'no new findings vs %s (%s total)\n' "$CI_TIDY_BASELINE" "$findings"
elif [ "${findings:-0}" -gt 0 ]; then
    grep -E 'warning:|error:' "$CI_LOG_DIR/tidy.log" | head -20
    fail "tidy: $findings finding(s) and no baseline file — accept them in one step with 'scripts/write-tidy-baseline.sh', or fix them; see .ci.env.example"
else
    printf '%s files clean\n' "${#sources[@]}"
fi
