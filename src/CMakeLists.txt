# clavis/src/CMakeLists.txt

set(LIB_SOURCES
    clavis.cpp
    exceptions.cpp
)

add_library(clavis-lib STATIC ${LIB_SOURCES}) # Use STATIC as requested

# Link dependencies
# nlohmann_json is header-only, but its CMake target sets include directories correctly
target_link_libraries(clavis-lib PUBLIC nlohmann_json::nlohmann_json)

# Ensure includes from the project's include directory are available
# PUBLIC: Consumers of clavis-lib also need these include dirs
# PRIVATE: Only clavis-lib needs these include dirs
# INTERFACE: Only consumers of clavis-lib need these include dirs
target_include_directories(clavis-lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>  # For targets linking within build
        $<INSTALL_INTERFACE:include> # For installed targets (relative to CMAKE_INSTALL_PREFIX)
    PRIVATE
        . # Allow src files to include each other if needed
)

# Set C++ standard for the library
target_compile_features(clavis-lib PUBLIC cxx_std_17)
