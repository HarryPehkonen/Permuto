#!/usr/bin/env bash
#
# version — ONE version number. project(VERSION) in CMakeLists.txt, the header the build
# generates (or a tracked one), the generated CMake package-version file, and — when
# CI_VERSION_BINARIES is set — what every binary prints for --version, must all agree.
#
# The kit's version stage compares the first two copies. The last two are this repo's own
# additions with their own incidents (INCIDENTS.md, .ci.env.example): the package-version file
# was a THIRD copy nobody looked at, and a version nobody can ask a binary for is not a version.
#
# Called from gate.toml as `[stage.version] cmd = "scripts/version.sh"`.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

cmake_version=$(sed -n 's/^project *( *[A-Za-z0-9_.-]* *VERSION *\([0-9][0-9.]*\).*/\1/p' CMakeLists.txt | head -1)
if [ -z "$cmake_version" ]; then
    fail "version: could not read a VERSION from CMakeLists.txt — project(<name> VERSION <x.y.z>) is what this stage parses"
fi

header="$CI_VERSION_HEADER"
if [ ! -f "$header" ]; then
    # Help, not guesswork: say which files could hold the second copy.
    candidates=$(find "$CI_BUILD_DIR" -type f \( -name '*version*.hpp' -o -name '*version*.h' \) 2>/dev/null | sort | head -5)
    if [ -n "$candidates" ]; then
        printf '%s not found. Version-named headers in %s:\n%s\n' "$header" "$CI_BUILD_DIR" "$candidates"
    fi
    fail "version: the header holding the second copy of the number is not at $CI_VERSION_HEADER — point CI_VERSION_HEADER at it (a tracked header works too)"
fi

# Any of:  #define APP_VERSION "1.2.3"   |   static constexpr char VERSION[] = "1.2.3";
header_version=$(grep -oE '"[0-9]+\.[0-9]+\.[0-9]+[^"]*"' "$header" | head -1 | tr -d '"')
if [ -z "$header_version" ]; then
    fail "version: $header holds no quoted x.y.z version string"
fi
if [ "$cmake_version" != "$header_version" ]; then
    printf 'CMakeLists.txt says %s, %s says %s\n' "$cmake_version" "$header" "$header_version"
    fail "version: the two copies of the version number disagree (bump both from the same edit)"
fi
printf '%s == %s == %s\n' "CMakeLists.txt" "$header" "$cmake_version"

# --- every OTHER copy this repo has: the generated CMake package-version file --------------
# write_basic_package_version_file() writes set(PACKAGE_VERSION "x.y.z") into the build dir. It
# is generated from project(VERSION), so it is right by construction — until someone edits the
# wrong line there or hands the macro a version of its own, and then a downstream find_package()
# is answered by a number this repo never checked. (This repo's call reads `VERSION
# ${PACKAGE_VERSION}`, which is unset: CMake's module falls back to PROJECT_VERSION, which is
# why the file is correct today. That fallback is exactly the thing not to take on trust.)
# maxdepth 1 is where CMakePackageConfigHelpers writes when it is given a bare filename (this
# repo's shape). Searching the whole build tree would also pick up a FetchContent'd dependency's
# *ConfigVersion.cmake — a version that is not ours to police.
if grep -qE '(^|[^#[:alnum:]_])write_basic_package_version_file' CMakeLists.txt; then
    pkg_version_files=()
    mapfile -t pkg_version_files < <(find "$CI_BUILD_DIR" -maxdepth 1 -name '*ConfigVersion.cmake' -type f | sort)
    if [ "${#pkg_version_files[@]}" -eq 0 ]; then
        fail "version: CMakeLists.txt calls write_basic_package_version_file() and $CI_BUILD_DIR (top level) holds no *ConfigVersion.cmake — that copy of the number was NOT checked (run the build stage first, or teach this stage where the file is written)"
    fi
    bad_copies=0
    for pf in "${pkg_version_files[@]}"; do
        pkg_version=$(sed -n 's/^set(PACKAGE_VERSION "\([0-9][0-9.]*\)".*/\1/p' "$pf" | head -1)
        if [ -z "$pkg_version" ]; then
            fail "version: $pf holds no set(PACKAGE_VERSION \"x.y.z\") — the copy could not be read at all"
        fi
        if [ "$pkg_version" != "$cmake_version" ]; then
            printf '%s says %s\n' "${pf#"$REPO_ROOT"/}" "$pkg_version"
            bad_copies=$((bad_copies + 1))
        fi
    done
    if [ "$bad_copies" -gt 0 ]; then
        fail "version: $bad_copies generated package-version file(s) disagree with project(VERSION) $cmake_version — that file is what a downstream find_package() reads"
    fi
    printf '%s package-version file(s) also say %s\n' "${#pkg_version_files[@]}" "$cmake_version"
else
    printf '(CMakeLists.txt does not call write_basic_package_version_file: no generated package version to compare)\n'
fi

# And, optionally, the binaries have to report it: a version nobody can ask for is not a
# version. Off by default because projects name their executables differently — this repo sets
# CI_VERSION_BINARIES (scripts/gate-env.sh) because its naming is stable.
if [ -n "${CI_VERSION_BINARIES:-}" ]; then
    checked=0
    wrong=0
    # eval, exactly like CI_TEST_CMD: the documented value is "$CI_BUILD_DIR/permuto", and a
    # plain word-split would carry that through as an unexpanded literal, match no file, and
    # then print "0 binary/binaries report <version>" -- a PASS. That is the fails-open shape
    # this stage exists to prevent, so the miss is also a FAIL below.
    binary_list=""
    eval "binary_list=\"$CI_VERSION_BINARIES\""
    for binary in $binary_list; do
        [ -f "$binary" ] && [ -x "$binary" ] || continue
        reported=$("$binary" --version 2>&1 | head -1)
        case "$reported" in
            *"$cmake_version"*) checked=$((checked + 1)) ;;
            *) wrong=$((wrong + 1)); printf '  %s --version printed: %s\n' "$(basename "$binary")" "$reported" ;;
        esac
    done
    if [ "$wrong" -gt 0 ]; then
        fail "version: $wrong binary/binaries do not report a --version containing $cmake_version"
    fi
    if [ "$checked" -eq 0 ]; then
        fail "version: CI_VERSION_BINARIES=\"$CI_VERSION_BINARIES\" matched no executable — the binaries were NOT checked"
    fi
    printf '%s binary/binaries report %s\n' "$checked" "$cmake_version"
else
    printf '(CI_VERSION_BINARIES unset: the binaries were not asked for --version)\n'
fi
