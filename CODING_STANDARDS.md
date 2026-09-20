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
- [ ] clang-tidy — the `tidy` stage is green with no findings at all (this repo's
      `.clang-tidy` enables the bug-finding subset and there is no baseline file; see
      Tooling status)
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
- **clang-tidy: wired, against the bug-finding subset.** `.clang-tidy` enables
  `clang-diagnostic-*` (the compiler diagnostics clang-tidy sees while parsing) and
  `clang-analyzer-*` (the Clang Static Analyzer), and no style families; the `tidy` stage is
  in `CI_DEFAULT_STAGES` and in `.githooks/pre-push`. Zero findings are required — there is
  no `.ci/tidy-baseline.txt`, because a baseline is for findings you inherited, not for a rule
  set nobody agreed to. Measured 2026-09-20 on this tree (17 translation units, 4 cores):
  **0 findings** in **104-201 s** with that set (cache-dependent), against **451 findings /
  387 s** for the stock style families and **53 findings / 204 s** for the suite's value-only
  set (`-*,bugprone-*,performance-*`). The style families are what `.clang-format` plus review
  already own; adopting the value-only set here is an OPEN decision with those numbers
  attached. The analyzer earned the wiring: the single finding it reported was real — a dead
  store in `examples/api_example.cpp` that made the example print a failed round trip and then
  report success with exit status 0. Cost: `tidy` is roughly three times the rest of the gate
  (~104-201 s against ~37-66 s; the whole 9-stage gate ran in 141 s with warm caches), paid on
  push only — the fast tier is still `build tests` — and it is the cheapest tidy in the suite.
  Reasons, and the set the earlier "tidy is absent" decision was measured against:
  `INCIDENTS.md`, `.ci.env.example`, and the adaptation notes at the top of `tools/ci.sh`.
- **Never weaken a stage to make it pass; a gate that fails open is the thing this tooling
  exists to prevent.** Every deviation from the AI-DEV-STARTER kit is listed in the
  adaptation notes at the top of `tools/ci.sh` with its reason, and every rule change gets
  an entry in `INCIDENTS.md` next to the check that enforces it.

## Upstream reference

https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines — the Profiles
chapter (Type / Bounds / Lifetime) is the conceptual core.
