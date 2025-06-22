# permuto/CMakeLists.txt

cmake_minimum_required(VERSION 3.15) # Increased for FetchContent best practices

project(permuto LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Compiler Flags for Clean Code ---
function(set_strict_compiler_flags TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        # Enable all warnings and treat them as errors
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        
        # Additional critical warning flags for GCC/Clang (not covered by -Wall -Wextra)
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -Werror=array-bounds
            -Werror=strict-overflow
            -Werror=strict-aliasing
            -Werror=format-security
            -Werror=format-nonliteral
            -Werror=cast-qual
            -Werror=cast-align
            -Werror=conversion
            -Werror=sign-conversion
            -Werror=float-conversion
            -Werror=double-promotion
            -Werror=shadow
            -Werror=redundant-decls
            -Werror=inline
            -Werror=disabled-optimization
            -Werror=stack-protector
            -Werror=unsafe-loop-optimizations
            -Werror=logical-op
            -Werror=duplicated-cond
            -Werror=duplicated-branches
            -Werror=null-dereference
            # -Werror=larger-than=4096
            -Werror=missing-field-initializers
            -Werror=missing-braces
            -Werror=parentheses
            -Werror=sequence-point
            -Werror=invalid-offsetof
            -Werror=pointer-arith
            -Werror=pointer-compare
            -Werror=suggest-attribute=cold
            -Werror=suggest-attribute=const
            -Werror=suggest-attribute=format
            -Werror=suggest-attribute=malloc
            -Werror=suggest-attribute=noreturn
            -Werror=suggest-attribute=pure
            -Werror=suggest-final-methods
            -Werror=suggest-final-types
            -Werror=suggest-override
            -Werror=switch-default
            -Werror=switch-enum
            -Werror=switch-unreachable
            -Werror=tautological-compare
            -Werror=trampolines
            -Werror=type-limits
            -Werror=unused-const-variable=2
            -Werror=vector-operation-performance
            -Werror=zero-as-null-pointer-constant
        >
        
        # Additional warning flags for MSVC
        $<$<CXX_COMPILER_ID:MSVC>:
            /w14062  # enumerator in switch is not handled
            /w14101  # unreferenced local variable
            /w14102  # unreferenced formal parameter
            /w14111  # local variable hides global declaration
            /w14112  # local variable hides member declaration
            /w14121  # unreferenced local function has been removed
            /w14126  # unreferenced formal parameter
            /w14127  # unreferenced local variable
            /w14128  # unreferenced local function
            /w14129  # unreferenced local typedef
            /w14130  # unreferenced local enum
            /w14131  # unreferenced local union
            /w14132  # unreferenced local struct
            /w14133  # unreferenced local class
            /w14134  # unreferenced local namespace
            /w14135  # unreferenced local template
            /w14136  # unreferenced local template specialization
            /w14137  # unreferenced local template instantiation
            /w14138  # unreferenced local template partial specialization
            /w14139  # unreferenced local template explicit instantiation
            /w14140  # unreferenced local template explicit specialization
            /w14141  # unreferenced local template friend
            /w14142  # unreferenced local template member
            /w14143  # unreferenced local template static member
            /w14144  # unreferenced local template non-static member
            /w14145  # unreferenced local template constructor
            /w14146  # unreferenced local template destructor
            /w14147  # unreferenced local template conversion operator
            /w14148  # unreferenced local template assignment operator
            /w14149  # unreferenced local template comparison operator
            /w14150  # unreferenced local template arithmetic operator
            /w14151  # unreferenced local template logical operator
            /w14152  # unreferenced local template bitwise operator
            /w14153  # unreferenced local template subscript operator
            /w14154  # unreferenced local template function call operator
            /w14155  # unreferenced local template address-of operator
            /w14156  # unreferenced local template dereference operator
            /w14157  # unreferenced local template arrow operator
            /w14158  # unreferenced local template increment operator
            /w14159  # unreferenced local template decrement operator
            /w14160  # unreferenced local template unary plus operator
            /w14161  # unreferenced local template unary minus operator
            /w14162  # unreferenced local template logical not operator
            /w14163  # unreferenced local template bitwise not operator
            /w14164  # unreferenced local template cast operator
            /w14165  # unreferenced local template sizeof operator
            /w14166  # unreferenced local template typeid operator
            /w14167  # unreferenced local template new operator
            /w14168  # unreferenced local template delete operator
            /w14169  # unreferenced local template throw operator
            /w14170  # unreferenced local template comma operator
            /w14171  # unreferenced local template conditional operator
            /w14172  # unreferenced local template scope resolution operator
            /w14173  # unreferenced local template member access operator
            /w14174  # unreferenced local template pointer-to-member operator
            /w14175  # unreferenced local template function-like macro
            /w14176  # unreferenced local template object-like macro
            /w14177  # unreferenced local template preprocessor directive
            /w14178  # unreferenced local template pragma directive
            /w14179  # unreferenced local template line directive
            /w14180  # unreferenced local template error directive
            /w14181  # unreferenced local template warning directive
            /w14182  # unreferenced local template include directive
            /w14183  # unreferenced local template define directive
            /w14184  # unreferenced local template undef directive
            /w14185  # unreferenced local template if directive
            /w14186  # unreferenced local template ifdef directive
            /w14187  # unreferenced local template ifndef directive
            /w14188  # unreferenced local template elif directive
            /w14189  # unreferenced local template else directive
            /w14190  # unreferenced local template endif directive
            /w14191  # unreferenced local template include_next directive
            /w14192  # unreferenced local template import directive
            /w14193  # unreferenced local template using directive
            /w14194  # unreferenced local template namespace directive
            /w14195  # unreferenced local template template directive
            /w14196  # unreferenced local template export directive
            /w14197  # unreferenced local template module directive
            /w14198  # unreferenced local template module partition directive
            /w14199  # unreferenced local template module import directive
            /w14200  # unreferenced local template module export directive
        >
    )
    
    # Enable sanitizers in debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${TARGET_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address,undefined,leak>
            $<$<CXX_COMPILER_ID:MSVC>:/fsanitize=address>
        )
        target_link_options(${TARGET_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address,undefined,leak>
            $<$<CXX_COMPILER_ID:MSVC>:/fsanitize=address>
        )
    endif()
    
    # Additional flags for extra safety
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -fstack-protector-strong
            -fstack-check
            -D_FORTIFY_SOURCE=2
            -fPIE
        >
        $<$<CXX_COMPILER_ID:MSVC>:
            /GS
            /sdl
            /guard:cf
        >
    )
    
    # Link flags for security
    target_link_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-pie>
        $<$<CXX_COMPILER_ID:MSVC>:/DYNAMICBASE /NXCOMPAT>
    )
endfunction()

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
