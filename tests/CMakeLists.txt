# permuto/tests/CMakeLists.txt

if(NOT PERMUTO_ENABLE_TESTING)
    message(STATUS "Tests are disabled.")
    return()
endif()

if(NOT TARGET GTest::gtest)
    message(FATAL_ERROR "GoogleTest target not found. Check root CMakeLists.txt FetchContent setup.")
endif()

# Create the test executable
add_executable(permuto_tests permuto_test.cpp)

# Link the library being tested and Google Test
target_link_libraries(permuto_tests PRIVATE permuto-lib GTest::gtest_main) # gtest_main provides main()

# Add test to CTest (requires CMake 3.10+)
include(GoogleTest)
gtest_discover_tests(permuto_tests)
