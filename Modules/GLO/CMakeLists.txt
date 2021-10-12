# ---- Library ----

add_library(O2QcGLO)

target_sources(O2QcGLO PRIVATE src/VertexingQcTask.cxx )

target_include_directories(
  O2QcGLO
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcGLO
  PUBLIC
  O2QualityControl
  O2::Steer)

install(TARGETS O2QcGLO
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcGLO
                    HEADERS
  include/GLO/VertexingQcTask.h
                    LINKDEF include/GLO/LinkDef.h
                    BASENAME O2QcGLO)

# ---- Test(s) ----

set(TEST_SRCS test/testQcGLO.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcGLO Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

