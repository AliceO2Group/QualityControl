# Produce the final Version.h using template Version.h.in and substituting
# variables. We don't want to polute our source tree with it, thus putting it in
# binary tree.
configure_file("include/Common/Version.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/Common/Version.h" @ONLY)

# ---- Library ----


add_library(O2QcCommon)

target_sources(O2QcCommon
               PRIVATE src/IncreasingEntries.cxx
                       src/WorstOfAllAggregator.cxx
                       src/QualityTask.cxx
                       src/QualityTaskConfig.cxx
                       src/BigScreenCanvas.cxx
                       src/BigScreen.cxx
                       src/BigScreenConfig.cxx
                       src/CcdbInspectorTask.cxx
                       src/CcdbInspectorTaskConfig.cxx
                       src/CcdbInspectorCheck.cxx
                       src/ObjectComparatorInterface.cxx
                       src/ObjectComparatorDeviation.cxx
                       src/ObjectComparatorBinByBinDeviation.cxx
                       src/ObjectComparatorChi2.cxx
                       src/ObjectComparatorKolmogorov.cxx
                       src/ReferenceComparatorPlot.cxx
                       src/ReferenceComparatorTask.cxx
                       src/ReferenceComparatorTaskConfig.cxx
                       src/ReferenceComparatorCheck.cxx
                       src/CheckerThresholdsConfig.cxx
                       src/TrendCheck.cxx
                       src/NonEmpty.cxx
                       src/MeanIsAbove.cxx
                       src/TH1Reductor.cxx
                       src/TH2Reductor.cxx
                       src/THnSparse5Reductor.cxx
                       src/QualityReductor.cxx
                       src/EverIncreasingGraph.cxx
                       src/TH1SliceReductor.cxx
                       src/TH2SliceReductor.cxx
                       src/LHCClockPhaseReductor.cxx)

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
                            include/Common/QualityTask.h
                            include/Common/BigScreen.h
                            include/Common/CcdbInspectorTask.h
                            include/Common/CcdbInspectorCheck.h
                            include/Common/ObjectComparatorInterface.h
                            include/Common/ObjectComparatorDeviation.h
                            include/Common/ObjectComparatorBinByBinDeviation.h
                            include/Common/ObjectComparatorChi2.h
                            include/Common/ObjectComparatorKolmogorov.h
                            include/Common/ReferenceComparatorTask.h
                            include/Common/ReferenceComparatorCheck.h
                            include/Common/TrendCheck.h
                            include/Common/MeanIsAbove.h
                            include/Common/TH1Ratio.h
                            include/Common/TH2Ratio.h
                            include/Common/TH1Reductor.h
                            include/Common/TH2Reductor.h
                            include/Common/THnSparse5Reductor.h
                            include/Common/QualityReductor.h
                            include/Common/EverIncreasingGraph.h
                            include/Common/IncreasingEntries.h
                            include/Common/TH1SliceReductor.h
                            include/Common/TH2SliceReductor.h
                            include/Common/LHCClockPhaseReductor.h
                    LINKDEF include/Common/LinkDef.h)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Common
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Tests ----

add_executable(o2-qc-test-common
               test/testMeanIsAbove.cxx
               test/testNonEmpty.cxx
               test/testCommonReductors.cxx
               test/testCommonHistRatios.cxx
               test/testWorstOfAllAggregator.cxx)
target_link_libraries(o2-qc-test-common PRIVATE O2QcCommon O2::Catch2)
set_property(TARGET o2-qc-test-common
             PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
add_test(NAME o2-qc-test-common COMMAND o2-qc-test-common)
set_tests_properties(o2-qc-test-common PROPERTIES TIMEOUT 20)
