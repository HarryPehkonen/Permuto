#!/usr/bin/env bash
#
# package — install the built tree into a temp prefix and build a two-file consumer against it
# with find_package(Permuto): the install tree and its CMake package files are a promise to
# people who are not in this repo, and until 2026-09-20 nothing checked it. PermutoConfig.cmake.in
# starts with @PACKAGE_INIT@ and the file was written with configure_file(), which leaves that
# placeholder EMPTY — so the library and headers installed perfectly, and every downstream
# find_package(Permuto) died on "Unknown CMake command check_required_components" (INCIDENTS.md).
#
# It needs a SYSTEM nlohmann/json, because the consumer resolves Permuto's dependency through the
# installed package's find_dependency. With none it prints why and exits 0 — a SKIP.
#
# That SKIP is the ONE place this conversion cannot keep the engine's own distinction, and it is
# recorded rather than hidden: kit-ci's only skip is `when = "tool:<name>"` in gate.toml, and
# "does this CMake cache hold a system nlohmann/json" is a question only the shell can ask. The
# stage therefore passes with a SKIP line in its output, where the old gate printed
# "SKIP package" in its summary (2026-10-06, INCIDENTS.md).
#
# Called from gate.toml as `[stage.package] cmd = "scripts/package.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

if [ ! -f "$CI_BUILD_DIR/CMakeCache.txt" ]; then
    fail "package: no configured $CI_BUILD_DIR — run the build stage first (this stage installs what it built)"
fi
if [ -z "$(sed -n 's/^nlohmann_json_DIR:PATH=//p' "$CI_BUILD_DIR/CMakeCache.txt" 2>/dev/null)" ]; then
    printf 'SKIP: no system nlohmann/json in %s — a consumer of this install tree needs one (find_dependency)\n' "$CI_BUILD_DIR"
    exit 0
fi

prefix=$(mktemp -d "${TMPDIR:-/tmp}/ci-package-prefix-XXXXXX")
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ci-package-consumer-XXXXXX")
cleanup() {
    [ "$CI_KEEP_TMP" = "1" ] || rm -rf "$prefix" "$tmp"
}
trap cleanup EXIT

if ! cmake --install "$CI_BUILD_DIR" --prefix "$prefix" > "$CI_LOG_DIR/package-install.log" 2>&1; then
    show_log "$CI_LOG_DIR/package-install.log"
    fail "package: cmake --install failed on the built tree"
fi
printf 'installed: %s\n' "$(cd "$prefix" && find . -name '*.cmake' -path '*Permuto*' | sort | tr '\n' ' ')"

cat > "$tmp/CMakeLists.txt" <<'CONSUMER'
cmake_minimum_required(VERSION 3.15)
project(permuto_consumer CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Permuto REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE Permuto::permuto)
CONSUMER
cat > "$tmp/main.cpp" <<'CONSUMER'
#include <permuto/permuto.hpp>
#include <iostream>
int main() {
    const auto templ = nlohmann::json::parse(R"({"greeting": "${/who}"})");
    const auto ctx = nlohmann::json::parse(R"({"who": "consumer"})");
    std::cout << permuto::apply(templ, ctx).dump() << '\n';
    return 0;
}
CONSUMER

if ! cmake -S "$tmp" -B "$tmp/build" -DCMAKE_PREFIX_PATH="$prefix" \
    > "$CI_LOG_DIR/package-configure.log" 2>&1; then
    show_log "$CI_LOG_DIR/package-configure.log"
    fail "package: a consumer project cannot find_package(Permuto) from the installed tree"
fi
if ! cmake --build "$tmp/build" -j "$CI_JOBS" > "$CI_LOG_DIR/package-build.log" 2>&1; then
    show_log "$CI_LOG_DIR/package-build.log"
    fail "package: a consumer cannot compile and link against the installed tree"
fi
if ! out=$("$tmp/build/consumer" 2> "$CI_LOG_DIR/package-run.log"); then
    show_log "$CI_LOG_DIR/package-run.log"
    fail "package: the consumer built but failed at run time"
fi
if [ "$out" != '{"greeting":"consumer"}' ]; then
    fail "package: the consumer ran and printed '$out', not the substitution the library promises"
fi
printf 'consumer: find_package(Permuto) -> apply() -> %s\n' "$out"
