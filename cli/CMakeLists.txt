# clavis/cli/CMakeLists.txt

add_executable(clavis main.cpp)

# Link our library and any other necessary libraries
target_link_libraries(clavis PRIVATE clavis-lib)

# Set C++ standard (can inherit from parent, but explicit is fine)
target_compile_features(clavis PUBLIC cxx_std_17)

# Installation for the command-line tool
include(GNUInstallDirs)
install(TARGETS clavis DESTINATION ${CMAKE_INSTALL_BINDIR})
