#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Permuto::permuto" for configuration ""
set_property(TARGET Permuto::permuto APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(Permuto::permuto PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libpermuto.a"
  )

list(APPEND _cmake_import_check_targets Permuto::permuto )
list(APPEND _cmake_import_check_files_for_Permuto::permuto "${_IMPORT_PREFIX}/lib/libpermuto.a" )

# Import target "Permuto::permuto-cli" for configuration ""
set_property(TARGET Permuto::permuto-cli APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(Permuto::permuto-cli PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/bin/permuto"
  )

list(APPEND _cmake_import_check_targets Permuto::permuto-cli )
list(APPEND _cmake_import_check_files_for_Permuto::permuto-cli "${_IMPORT_PREFIX}/bin/permuto" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
