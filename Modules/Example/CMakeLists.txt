# ---- Library ----

add_library(QcExample src/BenchmarkTask.cxx src/ExampleTask.cxx src/AnalysisTask.cxx
                      src/FakeCheck.cxx src/ExampleCondition.cxx)

target_include_directories(
  QcExample
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcExample PUBLIC QualityControl)

add_root_dictionary(QcExample
                    HEADERS include/Example/BenchmarkTask.h
                            include/Example/AnalysisTask.h
                            include/Example/ExampleTask.h
                            include/Example/FakeCheck.h
                            include/Example/ExampleCondition.h
                    LINKDEF include/Example/LinkDef.h
                    BASENAME QcExample)

install(TARGETS QcExample
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Workflows for analysis ----

# Install the config files for the analysis.
# The command "install" cannot be used because it would be executed after o2_add_qc_workflow which relies on
# the config files being already installed in the right place.
MESSAGE(STATUS "Configuration of the files etc/analysisDirect.json and etc/analysisDerived.json")
set(ANALYSIS_DIRECT_JSON "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}/analysisDirect.json")
set(ANALYSIS_DERIVED_JSON "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}/analysisDerived.json")
configure_file("etc/analysisDirect.json" "${ANALYSIS_DIRECT_JSON}")
configure_file("etc/analysisDerived.json" "${ANALYSIS_DERIVED_JSON}")
MESSAGE(STATUS "- Configured : ${ANALYSIS_DIRECT_JSON}")
MESSAGE(STATUS "- Configured : ${ANALYSIS_DERIVED_JSON}")

o2_add_qc_workflow(WORKFLOW_NAME o2-qc-example-analysis-direct CONFIG_FILE_PATH ${ANALYSIS_DIRECT_JSON})
o2_add_qc_workflow(WORKFLOW_NAME o2-qc-example-analysis-derived CONFIG_FILE_PATH ${ANALYSIS_DERIVED_JSON})

# ---- Tests ----

set(TEST_SRCS test/testFactory.cxx test/testQcExample.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})
  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcExample Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
               PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

target_sources(testFactory PRIVATE ${CMAKE_BINARY_DIR}/getTestDataDirectory.cxx)
target_include_directories(testFactory PRIVATE ${CMAKE_SOURCE_DIR})
