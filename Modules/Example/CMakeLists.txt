# ---- Library ----

add_library(O2QcExample
  src/BenchmarkTask.cxx
  src/EveryObject.cxx
  src/ExampleTask.cxx
  src/AnalysisTask.cxx
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
                            include/Example/AnalysisTask.h
                            include/Example/ExampleTask.h
                            include/Example/FakeCheck.h
                            include/Example/ExampleCondition.h
                            include/Example/CustomTH2F.h
                    LINKDEF include/Example/LinkDef.h
                    BASENAME O2QcExample)

install(TARGETS O2QcExample
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Example
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Workflows for analysis ----

# This is disabled because it can create problems when building on an FLP. This is due to the fact that we use the install directory before the install stage.

# Install the config files for the analysis.
# The command "install" cannot be used because it would be executed after o2_add_qc_workflow which relies on
# the config files being already installed in the right place.
# Note: There is an issue with ninja when a "configured" file is deleted. https://gitlab.kitware.com/cmake/cmake/-/issues/18985
#       The set_property should fix it.
#configure_file("etc/analysisDirect.json" "${CMAKE_INSTALL_PREFIX}/etc/analysisDirect.json")
#set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_INSTALL_PREFIX}/etc/analysisDirect.json")
#configure_file("etc/analysisDerived.json" "${CMAKE_INSTALL_PREFIX}/etc/analysisDerived.json")
#set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_INSTALL_PREFIX}/etc/analysisDerived.json")
#
## We cannot point to the json files in the source dir because the path will be reused in the produced json.
#o2_add_qc_workflow(WORKFLOW_NAME o2-qc-example-analysis-direct CONFIG_FILE_PATH ${CMAKE_INSTALL_PREFIX}/etc/analysisDirect.json)
#o2_add_qc_workflow(WORKFLOW_NAME o2-qc-example-analysis-derived CONFIG_FILE_PATH ${CMAKE_INSTALL_PREFIX}/etc/analysisDerived.json)

# ---- Tests ----

set(TEST_SRCS test/testFactory.cxx test/testQcExample.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})
  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcExample Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
               PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

target_sources(testFactory PRIVATE ${CMAKE_BINARY_DIR}/getTestDataDirectory.cxx)
target_include_directories(testFactory PRIVATE ${CMAKE_SOURCE_DIR})
