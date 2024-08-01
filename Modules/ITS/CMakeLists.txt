# ---- Library ----

add_library(O2QcITS
            src/ITSRawTask.cxx
            src/TrendingTaskITSThr.cxx
            src/TrendingTaskITSFhr.cxx
	          src/TrendingTaskITSFEE.cxx
            src/TrendingTaskITSCluster.cxx
            src/TrendingTaskITSTracks.cxx
            src/TrendingTaskConfigITS.cxx
            src/TrendingTaskITSError.cxx
            src/TH2XlineReductor.cxx
	          src/ReductorBinContent.cxx
            src/ReductorIntegralContent.cxx 
            src/ITSFhrTask.cxx
            src/ITSFeeTask.cxx
            src/ITSClusterTask.cxx
            src/ITSNoisyPixelTask.cxx
            src/ITSTrackTask.cxx
            src/ITSThresholdCalibrationTask.cxx
            src/ITSFhrCheck.cxx
            src/ITSClusterCheck.cxx
            src/ITSTrackCheck.cxx
            src/ITSFeeCheck.cxx
      	    src/ITSTrackSimTask.cxx
            src/ITSThresholdCalibrationCheck.cxx 
	    src/ITSDecodingErrorTask.cxx 
            src/ITSDecodingErrorCheck.cxx 
            )

target_sources(O2QcITS PRIVATE src/ITSChipStatusCheck.cxx  src/ITSChipStatusTask.cxx 
               src/TH2XlineReductor.cxx
               src/ReductorIntegralContent.cxx
               src/ReductorBinContent.cxx)

target_include_directories(
  O2QcITS
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${O2_ROOT}/include/GPU)

target_link_libraries(O2QcITS PUBLIC O2QualityControl O2::ITSBase O2::ITSMFTBase O2::ITSMFTReconstruction ROOT::Hist O2::DataFormatsITS O2::Steer O2QcCommon)

if (OpenMP_CXX_FOUND)
  target_compile_definitions(O2QcITS PRIVATE WITH_OPENMP)
  target_link_libraries(O2QcITS PRIVATE OpenMP::OpenMP_CXX)
endif()

install(TARGETS O2QcITS
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ITS
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- ROOT dictionary  ----

add_root_dictionary(O2QcITS
                    HEADERS include/ITS/ITSRawTask.h
  include/ITS/ITSChipStatusCheck.h
  include/ITS/ITSChipStatusTask.h
                            include/ITS/TrendingTaskITSThr.h
                            include/ITS/TrendingTaskITSFhr.h
			    include/ITS/TrendingTaskITSFEE.h
                            include/ITS/TrendingTaskITSCluster.h
                            include/ITS/TrendingTaskITSTracks.h
                            include/ITS/TrendingTaskITSError.h
                            include/ITS/TH2XlineReductor.h
			    include/ITS/ReductorBinContent.h
                            include/ITS/ReductorIntegralContent.h
                            include/ITS/ITSFhrTask.h
                            include/ITS/ITSFeeTask.h
                            include/ITS/ITSClusterTask.h
                            include/ITS/ITSNoisyPixelTask.h
                            include/ITS/ITSTrackTask.h
                            include/ITS/ITSThresholdCalibrationTask.h
                            include/ITS/ITSFhrCheck.h
                            include/ITS/ITSClusterCheck.h
                            include/ITS/ITSTrackCheck.h
                            include/ITS/ITSFeeCheck.h
			    include/ITS/ITSTrackSimTask.h 
                            include/ITS/ITSThresholdCalibrationCheck.h 
                            include/ITS/ITSDecodingErrorTask.h 
                            include/ITS/ITSDecodingErrorCheck.h
                            include/ITS/ITSHelpers.h 
                    LINKDEF include/ITS/LinkDef.h)

# ---- Test(s) ----

#add_executable(testQcITS test/testITS.cxx) # uncomment to reenable the test which was empty
foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
    PRIVATE O2QcCPV Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

# ---- Executables ----

set(EXE_SRCS
    src/runITS.cxx
    src/runITSTracksRootFileReader.cxx
    src/runITSClustersRootFileReader.cxx)

set(EXE_NAMES
    o2-qc-run-its
    o2-qc-its-tracks-root-file-reader
    o2-qc-its-clusters-root-file-reader)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE O2QualityControl CURL::libcurl O2::ITSQCDataReaderWorkflow O2::DetectorsBase ROOT::Tree)
endforeach()

install(
  TARGETS ${EXE_NAMES}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Extra scripts ----

install(FILES
        its.json
        itsQCTrendingThr.json
        itsQCTrendingFhr.json
	itsQCTrendingFEE.json
        itsQCTrendingCluster.json
        itsQCTrendingTracks.json
        itsFhr.json
        itsFee.json
        itsCluster.json
	itsTrack.json
        itsNoisyPixel.json
	itsTrackSim.json
        itsThresholdCalibration.json
        DESTINATION etc)

get_property(dirs
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
