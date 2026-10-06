# CLAUDE.md

Guidance for Claude Code (claude.ai/code) and any other agent working in this repository.
It describes the repo as it is. Where it states a rule, the rule has a check behind it.

## What this is

Permuto is a C++17 library for JSON template processing: values from a context document are
substituted into a JSON template through `${<json-pointer>}` placeholders, and the reverse
direction reconstructs the context from a processed result. The library, the CLI, the
examples, the suite and a local gate all exist and run — there is no plan phase left, and no
phase list in this file. `tools/ci.sh` is the single definition of "done".

Public API (`include/permuto/permuto.hpp`, `nlohmann::json` in and out):

```cpp
nlohmann::json apply(template_json, context, options);
nlohmann::json create_reverse_template(template_json, options);
nlohmann::json apply_reverse(reverse_template, result_json);
```

Core internals (`src/`, not installed): `TemplateProcessor` (traversal and substitution),
`PlaceholderParser` (`${...}` recognition, configurable markers), `JsonPointer` (RFC 6901
resolution), `ReverseProcessor` (reverse templates and reconstruction), `CycleDetector`
(self-referencing context values), and the exception hierarchy rooted at `PermutoException`.

**`JsonPointer` is the only pointer tokenizer in this codebase.** Anything that needs to split
or unescape a JSON Pointer goes through it. `ReverseProcessor` once carried a second, subtly
different copy of that logic, and the drift between the two silently dropped an empty-key
member from a reconstruction (`INCIDENTS.md`). Do not add a third.

## Layout

```
include/permuto/permuto.hpp   the single public header
src/                          implementation (static lib target `permuto`)
cli/main.cpp                  the `permuto` command-line tool
tests/                        GoogleTest suite, registered with ctest
fuzz/                         libFuzzer harness, seeds, regressions, make_seeds.py
examples/                     api_example, mixed_mode_example, multi_stage_example
tools/ci.sh                   the gate: every check this repo has, in one script
.githooks/                    pre-commit and pre-push, both call tools/ci.sh
cmake/                        version.hpp.in, PermutoConfig.cmake.in
```

## Build and run

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug     # the gate's own build type
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix /tmp/somewhere

./build/permuto example_template.json example_context.json    # CLI
./build/api_example                                           # examples
```

Options: `PERMUTO_BUILD_TESTS` (ON), `PERMUTO_BUILD_EXAMPLES` (ON), `PERMUTO_BUILD_FUZZING`
(OFF, clang only), `PERMUTO_NLOHMANN_DIR` (see Dependencies).

## The gate

`tools/ci.sh` is the only thing that may be called "the gate passed". It is offline,
non-destructive (it commits, stages, reverts and reformats nothing) and runs thirteen stages
in this order:

`tree docs format kitprobes build tests version asan tsan fuzz tidy pristine package`

- `tree` — every file is committed or ignored; `.gitignore` audit; `--require-clean` also
  fails on uncommitted edits to tracked files.
- `docs` — no document listed in `CI_DOCS_FILES` (`README.md`, this file,
  `CODING_STANDARDS.md`, `TECHNICAL_DETAILS.md`, `REQUIREMENTS.md`) may quote a count of the
  suite. Describe what is covered instead. `INCIDENTS.md` is exempt: its numbers are dated
  measurements of past events. Beware that the pattern is deliberately broad — a digit
  earlier on the same line as the word "test" can trip it even when nothing is being counted.
- `format` — clang-format dry run over the files this branch touches.
- `kitprobes` — `tools/kit-probes/*` hold this copy to the AI-DEV-STARTER fixes it claims to
  carry, by name and by behaviour.
- `build` — configure plus build, zero warnings.
- `tests` — the suite, every failure reported.
- `version` — `project(VERSION)`, the generated header, the CMake package-version file and
  `permuto --version` must all agree.
- `asan` / `tsan` — the suite again in separate build directories, under ASan+UBSan and TSan.
- `fuzz` — see below. Needs clang++; SKIPs without it unless `CI_STRICT_TOOLS=1`.
- `tidy` — clang-tidy with this repo's `.clang-tidy` (the bug-finding subset:
  `clang-diagnostic-*` and `clang-analyzer-*`, no style families), only findings new against
  `CI_TIDY_BASELINE`. It fails when a source is missing from `build/compile_commands.json`,
  so the database must cover everything.
- `pristine` — `git archive HEAD` into a temp dir, then configure, build, run the suite:
  proves the COMMITTED tree is complete. It therefore certifies nothing about uncommitted
  work; while a change is still in the working tree, run the other stages and say so.
- `package` — installs the built tree to a temp prefix and builds a two-file consumer against
  it with `find_package(Permuto)`: the install tree was installable and unusable once
  (`INCIDENTS.md`). Needs a system nlohmann/json, and SKIPs saying so when there is none.

Hooks (`git config core.hooksPath .githooks`, once per clone):
`pre-commit` runs `build tests` — a commit cannot afford minutes. `pre-push` runs every stage
plus `--require-clean`.

Configuration: `.ci.env` (gitignored, optional, documented knob by knob in `.ci.env.example`).
Every knob has a default in the script, so the repo works with no config at all. Stage logs
land in `.ci-logs/`. Build type is Debug for every stage that builds the library, except
`fuzz`, which builds RelWithDebInfo because an unoptimised harness buys far less exploration
per second. The kit's optimized-build (`release`) stage is deliberately NOT taken here.

**Every deviation from the AI-DEV-STARTER kit is listed in the adaptation block at the top of
`tools/ci.sh`, with its reason. Every rule change has an entry in `INCIDENTS.md`.** Read that
file before "simplifying" anything in the gate: what looks redundant there is usually a fix
for something that failed silently.

## The fuzz harness

`fuzz/fuzz_permuto.cpp`. Its oracle is this library's own documented round trip —
`apply` → `create_reverse_template` → `apply_reverse` must reproduce the context — asserted
only under the conditions the guarantee is claimed for, which are enumerated in the file's
header comment. A wrong answer here never crashes, so the assertion IS the oracle.

**Each input is read twice, and both readings assert the same property.**

1. The bytes are two documents: `<template> 0x00 <context>`.
2. The same bytes are a template plus a value pool: the harness collects the pointers the
   template references, unescapes their tokens and BUILDS a context providing exactly those
   paths, so the two documents agree by construction.

Reading 2 exists because reading 1 alone could not reach key-shape mutations, and that was
measured, not assumed: the equality-of-path-sets condition means byte mutation has to change
both sides of the separator together, so only value mutations survived and every context that
reached the assert had the seeds' key names. A sabotage that made `apply_reverse()` skip
mappings whose context path contains a digit survived millions of executions under reading 1
and dies instantly under reading 2.

Each reading has its own counter pair. The seed smoke (`-runs=0` over `fuzz/seeds` with
`PERMUTO_FUZZ_REQUIRE_IDENTITY=1`) fails unless BOTH are non-zero: a harness that cannot reach
its property looks exactly like one that always passes.

`fuzz/seeds/` is tracked and hand-derived from the documentation examples. `fuzz/corpus/` is
gitignored — libFuzzer grows it, and it is machine-local. `fuzz/regressions/` holds the
reproducers of defects this harness actually found; they are replayed on every run so a fixed
bug cannot come back unnoticed, and they carry input shapes the seeds do not. When the stage
reports a finding, the reproducer becomes a regression file and a unit test.

`CI_FUZZ_SECONDS` defaults to ten — a **regression budget** for a push prompt, not a campaign.
Rare shapes need longer: run `CI_FUZZ_SECONDS=1800 tools/ci.sh fuzz` (or the binary directly)
out of band, against the same corpus. Do not raise the gate's budget to chase one.

The harness is compiled in the default (non-fuzzing, gcc) build too, as an object library that
nothing links, so `build`, `format` and `tidy` all see it. Keep it compilable by both
compilers and free of libFuzzer-only headers.

## Known limits

- **Array element references do not round-trip.** A reference INTO a non-empty array
  (`${/items/0}`) cannot be reconstructed: a reverse template is a flat map from result
  pointer to context path and records no container kinds, so `apply_reverse()` rebuilds
  objects only and `{"items":[1,2]}` comes back as `{"items":{"0":1,"1":2}}`. A reference to
  the array as a whole (`${/items}`) does round-trip, and an empty array is a leaf. Fixing
  this means changing the reverse-template format; it is parked with Harri. The harness
  excludes exactly this case and nothing else.
- **The nlohmann fetch fallback is not exercised by any gate stage** (every machine that runs
  the gate has the system package), and it has a consequence worth knowing before relying on
  it: the exported package resolves nlohmann through `find_dependency`, so an install tree
  built against a fetched nlohmann is consumable only where a system nlohmann of the same ABI
  version is present. nlohmann versions its symbols in an inline namespace, so a mismatch is a
  link error in the consumer, not a warning.
- `apply_reverse()` accepts a reverse template from the caller and does not validate it; a
  hand-written one can still provoke exceptions that are not `PermutoException`.

## Dependencies

- **nlohmann/json is the JSON type in the public API**, deliberately: it is the de-facto
  standard, distro-packaged type, so a consumer programs against something already known and
  adopts nothing new to use Permuto. (JSOM, this suite's own JSON library, is deliberately not
  the boundary type here.)
- CMake uses the system copy when there is one. Otherwise it fetches nlohmann's **latest
  release** from GitHub with **no version pinned** (Harri's call) — the "latest release" URL,
  not a branch tip. A configured build directory keeps the copy it first fetched, so only a
  fresh configure picks up a newer release, and a from-scratch build on a machine without the
  package is what actually exercises this path.
- Offline machines: `-DPERMUTO_NLOHMANN_DIR=<dir holding nlohmann/json.hpp>`, or the distro
  package (`nlohmann-json3-dev`, `json-devel`).
- Both fallbacks register the headers as an IMPORTED interface target named
  `nlohmann_json::nlohmann_json` — the same name `find_package()` creates. That is load
  bearing: `install(EXPORT PermutoTargets)` requires every target in the link interface to be
  exportable, and a FetchContent subproject target is not.
- GoogleTest (`find_package(GTest REQUIRED)`) is needed only when `PERMUTO_BUILD_TESTS` is ON
  and is not part of the installed package. Threads IS in the library's link interface (it
  uses `thread_local` state), so `PermutoConfig.cmake.in` carries a `find_dependency` for it
  as well as for nlohmann — a dependency missing from that file is a consumer-side configure
  failure, and Threads was missing until the `package` stage found it (`INCIDENTS.md`).

## What an agent must do here

1. **TDD in both directions.** A failing test FIRST for every behavior change AND every bug
   fix — watched RED before the change, GREEN after. A bug fix with no test that was red is
   not finished.
2. **`CODING_STANDARDS.md` is mandatory**: modern C++17 in the spirit of the C++ Core
   Guidelines (Type/Bounds/Lifetime profiles), exceptions allowed for error handling. No raw
   owning pointers, no `new`/`delete`, no `reinterpret_cast` or C-style casts.
3. **One gate command**: `tools/ci.sh` must print `GATE PASSED`. Add `--require-clean` when
   the work is committed (that is what `pre-push` does).
4. **Never weaken a stage to make it pass** — no loosened flags, no shortened lists, no
   skipped check. If a build fails on a pre-existing warning, fix the warning with a small,
   targeted change. A gate that fails open is the thing this tooling exists to prevent.
5. Anything that changes a gate rule gets an `INCIDENTS.md` entry saying what broke and what
   catches it now.

## Where to read more

- `README.md` — the technical reference, written for LLM consumption: API, options, examples,
  round-trip limits, build options.
- `REQUIREMENTS.md` — the functional requirements the behaviour is held to.
- `TECHNICAL_DETAILS.md` — design and internals.
- `CODING_STANDARDS.md` — the C++ rules, and the tooling status behind them.
- `INCIDENTS.md` — every real failure and the check that now catches it. Dated, and exempt
  from the `docs` rule for that reason.
- Human-facing tutorials: the book at https://harrypehkonen.github.io/ComputoPermutoBook/
  (source: https://github.com/HarryPehkonen/ComputoPermutoBook).
