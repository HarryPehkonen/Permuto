#!/usr/bin/env bash
#
# The gate — run this before you push (and before you deploy).
#
# The POLICY is gate.toml (the stages, their tiers, their failure rules). The ENGINE is
# kit-ci, one binary installed once per machine:
#
#     cmake -S ~/hermes-workspace/KitCI -B build && cmake --build build
#     cmake --install build --prefix ~/.local        # -> ~/.local/bin/kit-ci
#
# This file is what all three callers run, so "the gate passed" means one thing no
# matter who says it:
#
#   1. a human, by hand            scripts/gate.sh [--tier fast]
#   2. git, on commit and on push  .githooks/pre-commit (--tier fast) and
#                                  .githooks/pre-push (--tier full, CI_REQUIRE_CLEAN=1)
#   3. a clean checkout elsewhere  a nightly job: fresh `git clone` into a temp dir,
#                                  then this same script
#
# Changing the gate means editing gate.toml, not this file. Until 2026-10-06 the gate WAS
# one file: a 1087-line tools/ci.sh, which ran thirteen stages and printed its own
# summary (card t_6989cec2). The stages are now gate.toml plus the scripts in this
# directory, and the knobs that script read (.ci.env) live in scripts/gate-env.sh.
#
# One thing that did NOT move into gate.toml: `--require-clean`, which used to be a flag
# this gate's pre-push hook passed. kit-ci's vocabulary has no per-run flags, so the hook
# now sets CI_REQUIRE_CLEAN=1 in the environment and scripts/tree.sh reads it — same rule,
# same caller, and it still defaults to off for a hand run on a dirty tree.
#
# Bypass deliberately, never accidentally:  git push --no-verify
set -uo pipefail

cd "$(dirname "$0")/.." || exit 1

# git's TEMPORARY index (exported by `git commit -- <path>`) must reach no stage: the long
# note is in scripts/gate-env.sh, where it also lives. Here as well because kit-ci itself
# is started from this script and never sources that file.
unset GIT_INDEX_FILE

# A gate that cannot find its engine must fail loudly rather than read as green.
if ! command -v kit-ci >/dev/null 2>&1; then
    printf 'gate: kit-ci is not installed. Build KitCI, then: cmake --install build --prefix ~/.local\n' >&2
    exit 1
fi

# No arguments: the tier the caller names (GATE_TIER, the variable the old gate honoured,
# still works) and otherwise the full tier — what a human running the gate by hand wants.
[ "$#" -gt 0 ] && exec kit-ci "$@"
exec kit-ci --tier "${GATE_TIER:-full}"
