# ---- Library ----

add_library(O2QcExample
  src/BenchmarkTask.cxx
  src/EveryObject.cxx
  src/ExampleTask.cxx
  src/FakeCheck.cxx
  src/ExampleCondition.cxx
  src/CustomTH2F.cxx
  )

target_include_directories(
  O2QcExample
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcExample PUBLIC O2QualityControl)

add_root_dictionary(O2QcExample
                    HEADERS include/Example/BenchmarkTask.h
                            include/Example/EveryObject.h
                            include/Example/ExampleTask.h
                            include/Example/FakeCheck.h
                            include/Example/ExampleCondition.h
                            include/Example/CustomTH2F.h
                    LINKDEF include/Example/LinkDef.h)

install(TARGETS O2QcExample
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Example
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Tests ----

add_executable(o2-qc-test-example
               test/testFactory.cxx
               test/testQcExample.cxx)
target_link_libraries(o2-qc-test-example PRIVATE O2QcExample O2::Catch2)
target_sources(o2-qc-test-example PRIVATE ${CMAKE_BINARY_DIR}/getTestDataDirectory.cxx)
target_include_directories(o2-qc-test-example PRIVATE ${CMAKE_SOURCE_DIR})
set_property(TARGET o2-qc-test-example
             PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
add_test(NAME o2-qc-test-example COMMAND o2-qc-test-example)
set_tests_properties(o2-qc-test-example PROPERTIES TIMEOUT 20)
