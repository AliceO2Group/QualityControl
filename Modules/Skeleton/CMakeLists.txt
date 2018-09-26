set(MODULE_NAME "QcSkeleton")

####################################
# Dependencies
####################################

find_package(ROOT 6.06.02 COMPONENTS RHTTP RMySQL Gui REQUIRED)
find_package(Boost 1.58 COMPONENTS unit_test_framework)
find_package(Configuration REQUIRED)

####################################
# Files
####################################

set(SRCS
  src/SkeletonTask.cxx
  src/SkeletonCheck.cxx
  )

set(HEADERS
  include/Skeleton/SkeletonTask.h
  include/Skeleton/SkeletonCheck.h
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
get_target_property(config_inc_dir Configuration::Configuration INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${config_inc_dir}")
list(REMOVE_DUPLICATES include_dirs)
include_directories(${include_dirs})

set(dict "${MODULE_NAME}Dict")
set(dict_src ${CMAKE_CURRENT_BINARY_DIR}/${dict}.cxx)
set_source_files_properties(${dict_src} PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
set_source_files_properties(${dict_src} PROPERTIES GENERATED TRUE)

ROOT_GENERATE_DICTIONARY("${dict}" ${HEADERS}
  LINKDEF include/Skeleton/LinkDef.h)

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
  )

####################################
# Tests
####################################

set(TEST_SRCS
  test/testQcSkeleton.cxx
  )

foreach (test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
