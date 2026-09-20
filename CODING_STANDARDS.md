# Permuto — C++ Coding Standards (MANDATORY for all agents and humans)

This file is the binding standard for all C++ work in this repository. It
implements the *C++ Core Guidelines* (Type / Bounds / Lifetime profiles) in a
form that is machine-checkable and agent-followable. Exceptions are ALLOWED
for error handling — this is not MISRA/JSF.

Reference: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines

## Hard rules (no exceptions)

1. **C++17 minimum.** No C-style code in new work.
2. **No raw owning pointers.** Use `std::unique_ptr` / `std::shared_ptr` and
   RAII. A raw pointer (or reference) is a *non-owning view* only.
3. **No `new` / `delete`**, no `malloc` / `free` in project code.
4. **Views over ranges:** `std::span` for contiguous buffers, `std::string_view`
   for read-only string parameters, instead of pointer+length.
5. **No `reinterpret_cast` or C-style casts.** `const_cast` only with a comment.
6. **No undefined behavior:** no reliance on signed overflow, no out-of-bounds
   access (use `.at()` or checked access), nothing read uninitialized —
   initialize every variable at declaration.
7. **`[[nodiscard]]`** on accessors and pure functions. If a call site must
   discard the value, cast to `(void)` with a comment.
8. **Exceptions are for error handling only**, never control flow. RAII
   guarantees cleanup on throw. Throw the project's documented exception type,
   never built-ins.
9. **No global mutable state.** Threaded code must be race-free: mutex/atomic,
   prefer immutable data.
10. **Never keep iterators or references across container mutation.** Re-fetch
    after any mutating call (the Lifetime profile's core rule).
11. **Zero warnings.** Project targets compile with
    `-Wall -Wextra -Wpedantic -Werror` where wired (see Tooling status).
12. **TDD.** Write the failing test first, watch it fail, implement, watch it
    pass. Bug fixes too: failing test that isolates the bug → fix → green.
13. **Sanitizer gate:** all tests must pass under ASan+UBSan before a change is
    done (see Tooling status for this repo's exact command).

## Style

- clang-format per the repo's `.clang-format` (run the repo's format target if
  present). Match existing naming conventions.
- Keep the naming/architecture conventions documented in this repo's
  CLAUDE.md / README.

## Definition of done (agent checklist)

- [ ] `bash tools/ci.sh --require-clean` prints `GATE PASSED` (that one command runs
      every check below, in the order the push gate runs them)
- [ ] Zero-warning build (see Tooling status — `-Werror` where wired)
- [ ] All tests pass
- [ ] Tests pass under ASan+UBSan
- [ ] clang-tidy — no NEW findings vs baseline (where tidy is configured; NOT configured
      in this repo — see Tooling status for the measured decision)
- [ ] No raw owning pointers / `new` / `reinterpret_cast` introduced
- [ ] Test written first (RED) for every behavior change or bug fix

## Tooling status (this repo, as of 2026-09-20)

Everything below is wired into `tools/ci.sh`, which is what `.githooks/pre-commit` (fast
tier: `build tests`) and `.githooks/pre-push` (full tier) run. Run it by hand at any time:

    bash tools/ci.sh                  # the whole gate, in order
    bash tools/ci.sh --list           # the stages and the effective default list
    bash tools/ci.sh --require-clean  # ...and fail on an uncommitted tracked file

- **Formatting:** `.clang-format` (the suite file: LLVM base, 4-space, 100 columns) and
  the `format` stage — every file a branch touches must conform. Fix a file with
  `clang-format -i <file>`.
- **Warnings:** `-Wall -Wextra -Wpedantic` on the `permuto` target, `-Werror` on all three
  own targets (`permuto`, `permuto-cli`, `permuto_tests`), and the `build` stage also
  counts `warning:` lines in its log, so a target that never got `-Werror` cannot slip
  through. Zero warnings is the verified baseline.
- **Tests:** `ctest --test-dir build` (PERMUTO_BUILD_TESTS, default ON) — 58 tests.
- **Sanitizers:** the `asan` (ASan+UBSan) and `tsan` (ThreadSanitizer) stages build in
  `build-asan/` and `build-tsan/` and run the same suite. By hand:
  `cmake -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined
  -fno-omit-frame-pointer" && cmake --build build-asan && ctest --test-dir build-asan`.
- **Tree:** the `tree` stage — every file is either committed or ignored, the gate's own
  footprint (`build/`, `build-*/`, `.ci-logs/`, `.ci.env`) is ignored, and with
  `--require-clean` no tracked file has uncommitted edits. Build output is never
  committed here; `INCIDENTS.md` says why that has a check behind it.
- **Version identity:** the `version` stage compares `project(VERSION)` in CMakeLists.txt
  against the generated `build/generated/version.hpp`, the generated
  `build/*ConfigVersion.cmake` that a downstream `find_package()` reads, and
  `permuto --version`. All three copies are one edit apart in CMakeLists.txt.
- **clang-tidy: deliberately NOT adopted in this repo.** There is no `.clang-tidy`, and the
  checklist item above is scoped to repos where tidy IS configured. Measured 2026-09-20
  against the sibling repo's house `.clang-tidy`: 451 findings over the 17 translation
  units and 387 s of wall clock on 4 cores — about six times this repo's whole gate. The
  two largest groups are house-style choices rather than defects
  (`modernize-use-trailing-return-type` 220, `modernize-use-nodiscard` 136), and the set's
  most safety-relevant check, `bugprone-unchecked-optional-access` (9 findings), is a false
  positive in all 9 cases — GoogleTest `ASSERT_TRUE(opt.has_value())` guards it cannot see
  through. Zero findings would only be reachable with an exclusion list naming every check
  that fires (a rule set that certifies nothing) or a baseline file, which the kit forbids
  for a config that was never adopted. The `tidy` stage is implemented: `tools/ci.sh tidy`
  reports the truth on demand, and wiring it the day a `.clang-tidy` lands is a one-line
  change to the stage list. Reasons: `INCIDENTS.md`.
- **Never weaken a stage to make it pass; a gate that fails open is the thing this tooling
  exists to prevent.** Every deviation from the AI-DEV-STARTER kit is listed in the
  adaptation notes at the top of `tools/ci.sh` with its reason, and every rule change gets
  an entry in `INCIDENTS.md` next to the check that enforces it.

## Upstream reference

https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines — the Profiles
chapter (Type / Bounds / Lifetime) is the conceptual core.
