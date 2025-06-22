# permuto/src/CMakeLists.txt

set(LIB_SOURCES
    permuto.cpp
    exceptions.cpp
)

add_library(permuto-lib STATIC ${LIB_SOURCES}) # Use STATIC as requested

# Apply strict compiler flags for clean code
set_strict_compiler_flags(permuto-lib)

# Link dependencies
# nlohmann_json is header-only, but its CMake target sets include directories correctly
target_link_libraries(permuto-lib PUBLIC nlohmann_json::nlohmann_json)

# Ensure includes from the project's include directory are available
# PUBLIC: Consumers of permuto-lib also need these include dirs
# PRIVATE: Only permuto-lib needs these include dirs
# INTERFACE: Only consumers of permuto-lib need these include dirs
target_include_directories(permuto-lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>  # For targets linking within build
        $<INSTALL_INTERFACE:include> # For installed targets (relative to CMAKE_INSTALL_PREFIX)
    PRIVATE
        . # Allow src files to include each other if needed
)

# Set C++ standard for the library
target_compile_features(permuto-lib PUBLIC cxx_std_17)
