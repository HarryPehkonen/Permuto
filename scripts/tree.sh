#!/usr/bin/env bash
#
# tree — every file is committed or ignored; the .gitignore audit; and, when the CALLER says
# so (CI_REQUIRE_CLEAN=1, which is what .githooks/pre-push sets), uncommitted edits to tracked
# files. That last rule is the old gate's `--require-clean` flag, moved from a flag to the
# environment because kit-ci's vocabulary has no per-run flags (2026-10-06, INCIDENTS.md).
#
# Called from gate.toml as `[stage.tree] cmd = "scripts/tree.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

# The gate creates these; a .gitignore that does not cover them makes the next run fail the
# moment it writes a log. Create them first: git check-ignore cannot match a directory pattern
# against a path that does not exist yet.
mkdir -p "$CI_BUILD_DIR" "$CI_ASAN_BUILD_DIR" "$CI_TSAN_BUILD_DIR" "$CI_LOG_DIR"

status=0

untracked=$(git ls-files --others --exclude-standard)
if [ -n "$untracked" ]; then
    printf 'not committed and not ignored:\n%s\n' "$untracked"
    if [ "${CI_ALLOW_UNTRACKED:-0}" = "1" ]; then
        printf '(not failing: CI_ALLOW_UNTRACKED=1)\n'
    else
        printf 'tree: %s file(s) are neither committed nor ignored — git add them, or add a .gitignore rule\n' \
            "$(printf '%s\n' "$untracked" | wc -l)" >&2
        status=1
    fi
else
    printf 'no untracked, unignored files\n'
fi

dirty=$(git status --porcelain --untracked-files=no)
if [ -n "$dirty" ]; then
    printf 'uncommitted changes to tracked files:\n%s\n' "$dirty"
    if [ "${CI_REQUIRE_CLEAN:-0}" = "1" ]; then
        printf 'tree: uncommitted changes to tracked files (that is the point of the push gate)\n' >&2
        status=1
    else
        printf '(not failing: CI_REQUIRE_CLEAN is not 1 — a hand run may work on a dirty tree)\n'
    fi
else
    printf 'no uncommitted changes to tracked files\n'
fi

tracked_ignored=$(git ls-files -i -c --exclude-standard)
if [ -n "$tracked_ignored" ]; then
    printf 'tracked files matched by .gitignore (stale rules):\n%s\n' "$tracked_ignored"
    printf 'tree: tracked files matched by .gitignore (stale rules)\n' >&2
    status=1
fi

# The gate's own footprint must be ignored, or the NEXT run reports its own logs as untracked.
missing=0
for path in "$CI_BUILD_DIR" "$CI_ASAN_BUILD_DIR" "$CI_TSAN_BUILD_DIR" "$CI_LOG_DIR" .ci.env; do
    if ! git check-ignore -q "$path" 2>/dev/null; then
        printf 'NOT ignored: %s\n' "$path"
        missing=$((missing + 1))
    fi
done
if [ "$missing" -gt 0 ]; then
    printf 'tree: %s path(s) that the gate itself creates are not in .gitignore — the next run would fail on its own log files\n' \
        "$missing" >&2
    status=1
else
    printf 'gate footprint (build dirs, %s, .ci.env) is ignored\n' "$CI_LOG_DIR"
fi

exit "$status"
