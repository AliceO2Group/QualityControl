# ---- Library ----

add_library(O2QcREC)

target_sources(O2QcREC PRIVATE src/ITSTPCMatchingTask.cxx  src/VertexingQcTask.cxx )

target_include_directories(
  O2QcREC
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcREC
  PUBLIC
  O2QualityControl
  O2::Steer
  O2::DataFormatsGlobalTracking
  O2::DataFormatsITS
  O2::GlobalTracking)

install(TARGETS O2QcREC
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcREC
                    HEADERS
  include/REC/ITSTPCMatchingTask.h
  include/REC/VertexingQcTask.h
                    LINKDEF include/REC/LinkDef.h
                    BASENAME O2QcREC)

# ---- Test(s) ----

set(TEST_SRCS test/testQcREC.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcREC Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

