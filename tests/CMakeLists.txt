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

# Apply strict compiler flags for clean code
set_strict_compiler_flags(permuto_tests)

# Link the library being tested and Google Test
target_link_libraries(permuto_tests PRIVATE permuto GTest::gtest_main) # gtest_main provides main()

# Set C++ standard (can inherit from parent, but explicit is fine)
target_compile_features(permuto_tests PRIVATE cxx_std_17)

# Add tests
add_test(NAME permuto_tests COMMAND permuto_tests)

# Enable CTest integration
set_tests_properties(permuto_tests PROPERTIES
    TIMEOUT 300 # 5 minutes timeout
    ENVIRONMENT "GTEST_COLOR=1" # Enable colored output
)
