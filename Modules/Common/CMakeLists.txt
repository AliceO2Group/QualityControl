# Produce the final Version.h using template Version.h.in and substituting
# variables. We don't want to polute our source tree with it, thus putting it in
# binary tree.
configure_file("include/Common/Version.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/Common/Version.h" @ONLY)

# ---- Library ----


add_library(O2QcCommon)

target_sources(O2QcCommon
               PRIVATE src/WorstOfAllAggregator.cxx
                       src/TRFCollectionTask.cxx
                       src/TRFCollectionTaskConfig.cxx
                       src/NonEmpty.cxx
                       src/MeanIsAbove.cxx
                       src/TH1Reductor.cxx
                       src/TH2Reductor.cxx
                       src/THnSparse5Reductor.cxx
                       src/QualityReductor.cxx
                       src/EverIncreasingGraph.cxx)

target_include_directories(
  O2QcCommon
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcCommon PUBLIC O2QualityControl O2::DataFormatsQualityControl PRIVATE ROOT::Graf)

install(TARGETS O2QcCommon
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcCommon
                    HEADERS include/Common/NonEmpty.h
                            include/Common/WorstOfAllAggregator.h
                            include/Common/TRFCollectionTask.h
                            include/Common/MeanIsAbove.h
                            include/Common/TH1Reductor.h
                            include/Common/TH2Reductor.h
                            include/Common/THnSparse5Reductor.h
                            include/Common/QualityReductor.h
                            include/Common/EverIncreasingGraph.h
                    LINKDEF include/Common/LinkDef.h
                    BASENAME O2QcCommon)

install(FILES etc/trfcollection-example.json
  DESTINATION Modules/Common)

# ---- Tests ----

set(TEST_SRCS
        test/testMeanIsAbove.cxx
        test/testNonEmpty.cxx
        test/testCommonReductors.cxx
        test/testWorstOfAllAggregator.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcCommon Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()
