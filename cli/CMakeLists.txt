# permuto/cli/CMakeLists.txt

add_executable(permuto main.cpp)

# Link our library and any other necessary libraries
target_link_libraries(permuto PRIVATE permuto-lib)

# Set C++ standard (can inherit from parent, but explicit is fine)
target_compile_features(permuto PUBLIC cxx_std_17)

# Installation for the command-line tool
include(GNUInstallDirs)
install(TARGETS permuto DESTINATION ${CMAKE_INSTALL_BINDIR})
