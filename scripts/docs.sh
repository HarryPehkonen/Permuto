#!/usr/bin/env bash
#
# docs — no document in CI_DOCS_FILES quotes a test count.
#
# One rule, and it exists because its absence was invisible: README's badge read
# "tests-65/65" and CLAUDE.md advertised "65 tests" against a suite of 58, and CLAUDE.md still
# described the project as being in a design phase after it was implemented. No stage had ever
# read a document, so every one of those statements was green. A number in prose is a claim
# with no check behind it; the suite is the only tally. The full rationale is in INCIDENTS.md.
#
# Called from gate.toml as `[stage.docs] cmd = "scripts/docs.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

files=0
offenders=0
for file in $CI_DOCS_FILES; do
    [ -f "$file" ] || continue
    files=$((files + 1))
    # Every shape a tally can take: the badge (tests-58%2F58), prose ("58 tests", "58 unit
    # tests"), the label form ("Total Tests: 66", "tests: 58") and the parenthetical
    # ("tests (58)"). -H so the finding names the file: an offender you cannot locate is a
    # finding you will not fix. The first version of this pattern was porous — the verifier
    # walked "Total Tests: 66" and "58 unit tests" straight past it (C6 in
    # /tmp/permuto-verification.md).
    if grep -nHEi 'tests-[0-9]|[0-9]+[[:space:]]+([[:alpha:]]+[[:space:]]+)*tests?\b|tests?[[:space:]]*:[[:space:]]*[0-9]+|tests?[[:space:]]*\([[:space:]]*[0-9]+' "$file"; then
        offenders=$((offenders + 1))
    fi
done

if [ "$files" -eq 0 ]; then
    fail "docs: none of CI_DOCS_FILES exists ($CI_DOCS_FILES) — this stage would check nothing and still pass"
fi
if [ "$offenders" -ne 0 ]; then
    fail "docs: $offenders doc file(s) quote a test count — the suite is the tally; say what the tests cover instead"
fi
printf '%s doc file(s) describe the project without quoting a test count\n' "$files"
