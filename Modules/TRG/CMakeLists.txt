# ---- Library ----

add_library(O2QcTRG)

target_sources(O2QcTRG PRIVATE src/RawDataQcTask.cxx )

target_include_directories(
  O2QcTRG
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcTRG PUBLIC O2QualityControl)

install(TARGETS O2QcTRG
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcTRG
                    HEADERS
include/TRG/RawDataQcTask.h
                    LINKDEF include/TRG/LinkDef.h
                    BASENAME O2QcTRG)

# ---- Test(s) ----

set(TEST_SRCS test/testQcTRG.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcTRG Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

