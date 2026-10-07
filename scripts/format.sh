#!/usr/bin/env bash
#
# format — clang-format --dry-run over the files THIS BRANCH touches, plus the STAGED copy of
# those files.
#
# Not the whole tree: the repo has pre-existing non-compliant files and a whole-tree check
# buries the signal in noise nobody edited. On a clean checkout (nothing ahead of origin/main)
# the last commit is what is checked instead.
#
# The staged-copy check is kit fix `format-checks-staged` (kit commit 96719c4), carried in this
# repo since commit f996aac, and this is the same code the old gate ran: the working-tree check
# above reads the DISK, a commit records the INDEX, so a file staged unformatted and then
# formatted on disk passes the first check while the commit still records the unformatted text.
#
# clang-format is guaranteed to exist: gate.toml declares `when = "tool:clang-format"`, so
# kit-ci skips this stage on a machine without it (the old gate's CI_STRICT_TOOLS knob, which
# made a missing tool a FAILURE, is not carried — see scripts/gate-env.sh).
#
# Called from gate.toml as `[stage.format] cmd = "scripts/format.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

base="$(git merge-base HEAD origin/main 2>/dev/null || git rev-parse HEAD)"
touched="$(
    { git diff --name-only --diff-filter=ACMR "$base" HEAD
      git diff --name-only --diff-filter=ACMR HEAD
      git diff --cached --name-only --diff-filter=ACMR   # a staged-only change is invisible to the line above
      git ls-files --others --exclude-standard
    } | sort -u
)"
if [ -z "$touched" ]; then
    touched="$(git show --name-only --pretty=format: HEAD | sed '/^$/d')"
    printf '(level with origin/main: checking the last commit instead)\n'
fi

sources=()
while IFS= read -r f; do
    [ -n "$f" ] && [ -f "$f" ] && sources+=("$f")
done < <(printf '%s\n' "$touched" | grep -E '\.(cpp|cc|cxx|hpp|hh|h)$')

if [ "${#sources[@]}" -eq 0 ]; then
    printf 'nothing to check\n'
    exit 0
fi

if clang-format --dry-run -Werror "${sources[@]}" > "$CI_LOG_DIR/format.log" 2>&1; then
    staged=()
    while IFS= read -r staged_f; do
        [ -n "$staged_f" ] && [ -f "$staged_f" ] && [[ "$staged_f" =~ \.(cpp|cc|cxx|hpp|hh|h)$ ]] && staged+=("$staged_f")
    done < <(git diff --cached --name-only --diff-filter=ACMR)

    if [ "${#staged[@]}" -gt 0 ]; then
        index_drift=()
        for f in "${staged[@]}"; do
            git show ":$f" 2>/dev/null |
                clang-format --dry-run -Werror --assume-filename="$f" - > /dev/null 2>> "$CI_LOG_DIR/format.log" ||
                index_drift+=("$f")
        done
        if [ "${#index_drift[@]}" -gt 0 ]; then
            printf 'the STAGED copy is not formatted (that is what the commit would record):\n'
            printf '%s\n' "${index_drift[@]}"
            fail "format: the staged copy of the file(s) above fails clang-format — this stage checks the working tree, and a commit records the index (fix: clang-format -i <files> && git add <files>)"
        fi
    fi
    printf '%s file(s) conform to .clang-format\n' "${#sources[@]}"
    exit 0
fi

grep -oE '^[^:]+\.(cpp|cc|cxx|hpp|hh|h)' "$CI_LOG_DIR/format.log" | sort -u
show_log "$CI_LOG_DIR/format.log"
fail "format: clang-format drift in the file(s) listed above (fix: clang-format -i <those files>)"
