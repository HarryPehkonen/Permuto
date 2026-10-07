cmake_minimum_required(VERSION 3.15)
project(Permuto VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Version header (the artifact-identity invariant) ---
# project(Permuto VERSION ...) on line 2 is the ONE place the version is written. This
# generates the second copy the CLI compiles against; the gate's `version` stage fails
# when the two disagree, and when `permuto --version` reports something else.
configure_file(cmake/version.hpp.in "${CMAKE_BINARY_DIR}/generated/version.hpp" @ONLY)

# Dependencies.
#
# nlohmann/json: prefer the copy the machine already has — that keeps the gate and any offline
# machine network-free — and otherwise supply it, because "it is installed everywhere" is not
# true: a fresh clone on a machine without the system package could not even configure (the
# Nobara laptop, 2026-09-20 — INCIDENTS.md).
#
# NO VERSION IS PINNED, deliberately (Harri's call, 2026-09-20): nlohmann is mature enough that
# the newest release is the one to build against, and pinning would mean re-deciding it later.
# Note WHAT is unpinned: the URL below is GitHub's "latest release" alias, not a branch tip —
# nlohmann's default branch is `develop`, i.e. unreleased work. A stale build dir keeps the copy
# it fetched first, so only a fresh configure picks up a new release; a clean clone on a machine
# without the system package is what exercises this path (that is the point of the occasional
# from-scratch build on the Pi).
#
# Offline on a package-less machine: -DPERMUTO_NLOHMANN_DIR=<dir containing nlohmann/json.hpp>,
# or install the distro package (Debian/Ubuntu: nlohmann-json3-dev; Fedora/Nobara: json-devel).
#
# Both fallbacks hand the headers to the build as an IMPORTED interface target named
# `nlohmann_json::nlohmann_json` — the SAME name find_package() creates — instead of adding the
# dependency as a subproject. That is not cosmetic: install(EXPORT PermutoTargets) requires every
# target in `permuto`'s link interface to be exportable, an IMPORTED target is, and a subproject
# target is NOT ("includes target permuto which requires target nlohmann_json that is not in any
# export set" — measured 2026-09-20, INCIDENTS.md). It also keeps the install tree honest: the
# exported interface names a target the consumer resolves through PermutoConfig.cmake's
# find_dependency(nlohmann_json 3.2.0), so Permuto never ships or pins a copy of nlohmann.
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
    set(PERMUTO_NLOHMANN_DIR "" CACHE PATH
        "Dir holding nlohmann/json.hpp, used when no system package is found (offline builds)")
    if(PERMUTO_NLOHMANN_DIR)
        if(EXISTS "${PERMUTO_NLOHMANN_DIR}/include/nlohmann/json.hpp")
            set(PERMUTO_NLOHMANN_INCLUDE_DIR "${PERMUTO_NLOHMANN_DIR}/include")
        elseif(EXISTS "${PERMUTO_NLOHMANN_DIR}/nlohmann/json.hpp")
            set(PERMUTO_NLOHMANN_INCLUDE_DIR "${PERMUTO_NLOHMANN_DIR}")
        else()
            message(FATAL_ERROR
                "PERMUTO_NLOHMANN_DIR=${PERMUTO_NLOHMANN_DIR} holds no nlohmann/json.hpp — "
                "point it at an nlohmann/json checkout (its include/ dir) or at the dir that "
                "contains nlohmann/json.hpp.")
        endif()
        message(STATUS "nlohmann/json: using headers from PERMUTO_NLOHMANN_DIR")
    else()
        include(FetchContent)
        message(STATUS
            "nlohmann/json: no system package found — fetching the LATEST RELEASE from GitHub "
            "(unpinned on purpose; see CMakeLists.txt)")
        # SOURCE_SUBDIR keeps FetchContent from add_subdirectory()ing the dependency: we only
        # want its headers, and a subproject target cannot be exported (see above).
        FetchContent_Declare(nlohmann_json
            URL https://github.com/nlohmann/json/releases/latest/download/json.tar.xz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            SOURCE_SUBDIR cmake-is-not-used-here)
        FetchContent_MakeAvailable(nlohmann_json)
        set(PERMUTO_NLOHMANN_INCLUDE_DIR "${nlohmann_json_SOURCE_DIR}/include")
    endif()
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PERMUTO_NLOHMANN_INCLUDE_DIR}")
endif()
find_package(Threads REQUIRED)

# --- Fuzzing (fuzz/fuzz_permuto.cpp) -------------------------------------------------
# OFF by default. The harness is compiled either way (see the permuto_fuzz targets near
# the bottom of this file) so the build, format and tidy gates always see it; only the
# libFuzzer executable and the instrumentation are gated on this option.
#
# These flags are directory scope on purpose: instrumenting ONLY the harness leaves
# libFuzzer with no coverage feedback from the library it is supposed to be exploring
# (measured elsewhere in this suite: cov 742 against 1493). They have to be set here,
# before any target in this directory is created, because add_compile_options() only
# reaches targets declared after it.
option(PERMUTO_BUILD_FUZZING "Build the libFuzzer harness (clang only)" OFF)
if(PERMUTO_BUILD_FUZZING)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR
            "PERMUTO_BUILD_FUZZING=ON needs clang: libFuzzer ships with clang as "
            "-fsanitize=fuzzer and has no GCC equivalent. This compiler is "
            "'${CMAKE_CXX_COMPILER_ID}'. Reconfigure a separate build dir with "
            "-DCMAKE_CXX_COMPILER=clang++ (the fuzz stage, scripts/fuzz.sh, does exactly that).")
    endif()
    add_compile_options(-fsanitize=fuzzer-no-link,address,undefined
                        -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g -O1)
    add_link_options(-fsanitize=address,undefined)
endif()

# Library target
add_library(permuto STATIC
    src/template_processor.cpp
    src/placeholder_parser.cpp
    src/json_pointer.cpp
    src/reverse_processor.cpp
    src/cycle_detector.cpp
    src/exceptions.cpp
    src/api.cpp
)

target_include_directories(permuto
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(permuto
    PUBLIC
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Set compiler warnings
if(MSVC)
    target_compile_options(permuto PRIVATE /W4)
else()
    target_compile_options(permuto PRIVATE -Wall -Wextra -Wpedantic)
endif()

# CLI executable
add_executable(permuto-cli cli/main.cpp)
target_link_libraries(permuto-cli PRIVATE permuto)
# The generated version header (see configure_file at the top): the CLI prints the
# version, and it must print the one CMakeLists.txt declares.
target_include_directories(permuto-cli PRIVATE "${CMAKE_BINARY_DIR}/generated")
set_target_properties(permuto-cli PROPERTIES OUTPUT_NAME permuto)

# Testing
option(PERMUTO_BUILD_TESTS "Build tests" ON)
if(PERMUTO_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    
    add_executable(permuto_tests
        tests/test_json_pointer.cpp
        tests/test_template_processor.cpp
        tests/test_placeholder_parser.cpp
        tests/test_reverse_processor.cpp
        tests/test_cycle_detector.cpp
        tests/test_integration.cpp
    )
    
    target_link_libraries(permuto_tests
        PRIVATE
            permuto
            GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(permuto_tests)
endif()

# Examples
option(PERMUTO_BUILD_EXAMPLES "Build examples" ON)
if(PERMUTO_BUILD_EXAMPLES)
    add_executable(api_example examples/api_example.cpp)
    target_link_libraries(api_example PRIVATE permuto)
    
    add_executable(mixed_mode_example examples/mixed_mode_example.cpp)
    target_link_libraries(mixed_mode_example PRIVATE permuto)
    
    add_executable(multi_stage_example examples/multi_stage_example.cpp)
    target_link_libraries(multi_stage_example PRIVATE permuto)
endif()

# Fuzz harness
if(PERMUTO_BUILD_FUZZING)
    # The real thing: libFuzzer drives LLVMFuzzerTestOneInput, the library above is
    # already instrumented by the directory-scope flags.
    add_executable(permuto_fuzz fuzz/fuzz_permuto.cpp)
    target_link_libraries(permuto_fuzz PRIVATE permuto)
    target_compile_options(permuto_fuzz PRIVATE -fsanitize=fuzzer)
    target_link_options(permuto_fuzz PRIVATE -fsanitize=fuzzer)
else()
    # Fuzzing OFF (the default, and what every other stage of the gate builds):
    # compile the harness anyway as an object library. That is what puts
    # fuzz/fuzz_permuto.cpp into build/compile_commands.json — the `tidy` stage fails
    # the run when a source is missing from the compile database — and what makes the
    # `build` stage check its warnings. Nothing links it.
    add_library(permuto_fuzz_objects OBJECT fuzz/fuzz_permuto.cpp)
    target_link_libraries(permuto_fuzz_objects PRIVATE permuto)
endif()

# Installation
install(TARGETS permuto permuto-cli
    EXPORT PermutoTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT PermutoTargets
    FILE PermutoTargets.cmake
    NAMESPACE Permuto::
    DESTINATION lib/cmake/Permuto
)

# Package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    PermutoConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# configure_package_config_file(), NOT configure_file(): the .in file begins with
# @PACKAGE_INIT@, and only this function expands it — into the definition of
# check_required_components() and the prefix-relocation logic. With plain configure_file() the
# placeholder is empty, so every downstream find_package(Permuto) died on "Unknown CMake command
# check_required_components" (measured 2026-09-20 by the consumer test the `package` stage runs;
# INCIDENTS.md).
configure_package_config_file(
    cmake/PermutoConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfig.cmake"
    INSTALL_DESTINATION lib/cmake/Permuto
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfigVersion.cmake"
    DESTINATION lib/cmake/Permuto
)

# --- CODING_STANDARDS.md: warnings as errors for Permuto's own targets ---
foreach(permuto_target permuto permuto-cli permuto_tests permuto_fuzz permuto_fuzz_objects)
    if(TARGET ${permuto_target})
        target_compile_options(${permuto_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endforeach()
