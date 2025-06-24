# permuto/src/CMakeLists.txt

set(LIB_SOURCES
    permuto.cpp
    exceptions.cpp
)

# Create library target - type will be determined by BUILD_SHARED_LIBS
add_library(permuto ${LIB_SOURCES})

# Apply strict compiler flags for clean code
set_strict_compiler_flags(permuto)

# Set symbol visibility for shared libraries
if(BUILD_SHARED_LIBS)
    target_compile_options(permuto PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fvisibility=hidden -fPIC>
    )
else()
    target_compile_options(permuto PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fPIC>
    )
endif()

# Link dependencies
# nlohmann_json is header-only, but its CMake target sets include directories correctly
target_link_libraries(permuto PUBLIC nlohmann_json::nlohmann_json)

# Ensure includes from the project's include directory are available
# PUBLIC: Consumers of permuto also need these include dirs
# PRIVATE: Only permuto needs these include dirs
# INTERFACE: Only consumers of permuto need these include dirs
target_include_directories(permuto
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>  # For targets linking within build
        $<INSTALL_INTERFACE:include> # For installed targets (relative to CMAKE_INSTALL_PREFIX)
    PRIVATE
        . # Allow src files to include each other if needed
)

# Set C++ standard for the library
target_compile_features(permuto PUBLIC cxx_std_17)
