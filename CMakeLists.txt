cmake_minimum_required(VERSION 3.15)
project(Permuto VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Threads REQUIRED)

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
        tests/test_thread_safety.cpp
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

configure_file(cmake/PermutoConfig.cmake.in PermutoConfig.cmake @ONLY)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PermutoConfigVersion.cmake"
    DESTINATION lib/cmake/Permuto
)