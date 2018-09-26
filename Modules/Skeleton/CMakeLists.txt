set(MODULE_NAME "QcSkeleton")

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
# ROOT dictionary
####################################

GENERATE_ROOT_DICT(MODULE_NAME ${MODULE_NAME} LINKDEF "include/Skeleton/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

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
