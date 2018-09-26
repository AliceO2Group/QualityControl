set(MODULE_NAME "QcCommon")

####################################
# Dependencies
####################################

find_package(ROOT 6.06.02 COMPONENTS RHTTP RMySQL Gui REQUIRED)
find_package(Boost 1.58 COMPONENTS unit_test_framework)

####################################
# Files
####################################

set(SRCS
  src/NonEmpty.cxx
  src/MeanIsAbove.cxx
)

set(HEADERS
  include/Common/NonEmpty.h
  include/Common/MeanIsAbove.h
)

# Produce the final Version.h using template Version.h.in and substituting variables.
# We don't want to polute our source tree with it, thus putting it in binary tree.
configure_file("include/Common/Version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/Common/Version.h"
  @ONLY
)

####################################
# ROOT dictionary
####################################

# ROOT dictionary
# the following root macros expect include dirs to be set as directory property
get_directory_property(include_dirs INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/include") # this module
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/include") # the framework
get_target_property(qc_inc_dir QualityControl INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${qc_inc_dir}")
list(REMOVE_DUPLICATES include_dirs)
include_directories(${include_dirs})

set(dict "${MODULE_NAME}Dict")
set(dict_src ${CMAKE_CURRENT_BINARY_DIR}/${dict}.cxx)
set_source_files_properties(${dict_src} PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
set_source_files_properties(${dict_src} PROPERTIES GENERATED TRUE)

ROOT_GENERATE_DICTIONARY("${dict}" ${HEADERS}
  LINKDEF include/Common/LinkDef.h)

# TODO review how and what to install for dictionary
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}Dict_rdict.pcm
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${MODULE_NAME}Dict.rootmap
  DESTINATION lib)

####################################
# Library
####################################

# Create library
add_library(${MODULE_NAME} SHARED ${SRCS} ${MODULE_NAME}Dict.cxx)

target_include_directories(${MODULE_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link targets
target_link_libraries(${MODULE_NAME}
  PUBLIC
    QualityControl
  PRIVATE
    ROOT::Graf
)

####################################
# Tests
####################################

set(TEST_SRCS
    test/testMeanIsAbove.cxx
    test/testNonEmpty.cxx
)

foreach (test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
