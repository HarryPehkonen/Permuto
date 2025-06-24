# permuto/cli/CMakeLists.txt

add_executable(permuto-cli main.cpp)

# Apply strict compiler flags for clean code
set_strict_compiler_flags(permuto-cli)

# Link against the permuto library
target_link_libraries(permuto-cli PRIVATE permuto)

# Set C++ standard (can inherit from parent, but explicit is fine)
target_compile_features(permuto-cli PUBLIC cxx_std_17)

# Installation for the command-line tool
include(GNUInstallDirs)
install(TARGETS permuto-cli DESTINATION ${CMAKE_INSTALL_BINDIR})
