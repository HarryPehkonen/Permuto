# permuto/CMakeLists.txt

cmake_minimum_required(VERSION 3.15) # Increased for FetchContent best practices

project(permuto LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependencies ---
include(FetchContent)

# Declare dependencies
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3 # Using a specific stable tag is recommended
)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0 # Using a specific stable tag
    GIT_SHALLOW TRUE # Speed up download
)

# Helper function to prefer find_package over FetchContent
function(find_or_fetch_package PKG_NAME FIND_ARGS)
    if(NOT TARGET ${FIND_ARGS}) # Use target name to check if already found/fetched
        find_package(${PKG_NAME} QUIET)
        if(NOT ${PKG_NAME}_FOUND)
            message(STATUS "Fetching ${PKG_NAME} via FetchContent")
            FetchContent_MakeAvailable(${PKG_NAME})
            if(${PKG_NAME} STREQUAL "googletest")
                # Prevent GTest from installing itself globally
                set(INSTALL_GTEST OFF CACHE BOOL "Disable installing GTest" FORCE)
            endif()
        else()
            message(STATUS "Found pre-installed ${PKG_NAME}")
        endif()
    endif()
endfunction()

# Get dependencies (use target names for checking)
# nlohmann_json provides 'nlohmann_json::nlohmann_json' target
# googletest provides 'GTest::gtest', 'GTest::gtest_main', 'GTest::gmock', 'GTest::gmock_main' targets
find_or_fetch_package(nlohmann_json nlohmann_json::nlohmann_json)
find_or_fetch_package(googletest GTest::gtest)


# --- Subdirectories ---
add_subdirectory(src)
add_subdirectory(cli)

# --- Testing ---
option(PERMUTO_ENABLE_TESTING "Enable building tests" ON)
if(PERMUTO_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Installation ---
# Install target artifacts and headers

include(GNUInstallDirs)

# Install the library target (defined in src/CMakeLists.txt)
install(TARGETS permuto-lib
    EXPORT PermutoTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # For shared libs on Windows
)

# Install the public headers
install(DIRECTORY include/permuto/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/permuto
    FILES_MATCHING PATTERN "*.hpp"
)

# Generate and install the CMake package configuration files
# This allows find_package(permuto) to work for other projects
include(CMakePackageConfigHelpers)

# Generate PermutoConfigVersion.cmake
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Define the install location for the CMake config files
set(PERMUTO_CMAKE_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/permuto/cmake")

# Generate PermutoConfig.cmake
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PermutoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfig.cmake"
    INSTALL_DESTINATION "${PERMUTO_CMAKE_CONFIG_INSTALL_DIR}"
)

# Install the config files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfigVersion.cmake"
    DESTINATION "${PERMUTO_CMAKE_CONFIG_INSTALL_DIR}"
)

# Install the export file (lists installed targets)
install(
    EXPORT PermutoTargets
    FILE PermutoTargets.cmake
    NAMESPACE Permuto::
    DESTINATION "${PERMUTO_CMAKE_CONFIG_INSTALL_DIR}"
)

message(STATUS "Permuto Config files install path: ${PERMUTO_CMAKE_CONFIG_INSTALL_DIR}")
