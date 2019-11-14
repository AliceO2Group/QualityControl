# ---- Library ----

add_library(QcSkeleton)

target_sources(QcSkeleton PRIVATE src/SkeletonTask.cxx src/SkeletonCheck.cxx src/SkeletonPostProcessing.cxx)

target_include_directories(
  QcSkeleton
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcSkeleton PUBLIC QualityControl)

install(TARGETS QcSkeleton
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- ROOT dictionary ----

add_root_dictionary(QcSkeleton
                    HEADERS include/Skeleton/SkeletonTask.h
                            include/Skeleton/SkeletonCheck.h
                            include/Skeleton/SkeletonPostProcessing.h
                    LINKDEF "include/Skeleton/LinkDef.h"
                    BASENAME QcSkeleton)

# ---- Tests ----

set(TEST_SRCS test/testQcSkeleton.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcSkeleton Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()
