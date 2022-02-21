# ---- Library ----

add_library(O2QcITS
            src/ITSRawTask.cxx
            src/TrendingTaskITSThr.cxx
            src/TrendingTaskITSFhr.cxx
            src/TrendingTaskITSCluster.cxx
            src/TrendingTaskITSTracks.cxx
            src/TrendingTaskConfigITS.cxx
            src/TH2XlineReductor.cxx
            src/ITSFhrTask.cxx
            src/ITSFeeTask.cxx
            src/ITSClusterTask.cxx
            src/ITSNoisyPixelTask.cxx
            src/ITSTrackTask.cxx
            src/ITSFhrCheck.cxx
            src/ITSClusterCheck.cxx
            src/ITSTrackCheck.cxx
            src/ITSFeeCheck.cxx
            src/ITSTrackSimTask.cxx
            )

target_sources(O2QcITS PRIVATE src/TH2XlineReductor.cxx)

target_include_directories(
  O2QcITS
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${O2_ROOT}/include/GPU)

target_link_libraries(O2QcITS PUBLIC O2QualityControl O2::ITSBase O2::ITSMFTBase O2::ITSMFTReconstruction ROOT::Hist O2::DataFormatsITS O2::Steer)

if (OpenMP_CXX_FOUND)
  target_compile_definitions(O2QcITS PRIVATE WITH_OPENMP)
  target_link_libraries(O2QcITS PRIVATE OpenMP::OpenMP_CXX)
endif()

install(TARGETS O2QcITS
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- ROOT dictionary  ----

add_root_dictionary(O2QcITS
                    HEADERS include/ITS/ITSRawTask.h
                            include/ITS/TrendingTaskITSThr.h
                            include/ITS/TrendingTaskITSFhr.h
                            include/ITS/TrendingTaskITSCluster.h
                            include/ITS/TrendingTaskITSTracks.h
                            include/ITS/TH2XlineReductor.h
                            include/ITS/ITSFhrTask.h
                            include/ITS/ITSFeeTask.h
                            include/ITS/ITSClusterTask.h
                            include/ITS/ITSNoisyPixelTask.h
                            include/ITS/ITSTrackTask.h
                            include/ITS/ITSFhrCheck.h
                            include/ITS/ITSClusterCheck.h
                            include/ITS/ITSTrackCheck.h
                            include/ITS/ITSFeeCheck.h
                            include/ITS/ITSTrackSimTask.h
                    LINKDEF include/ITS/LinkDef.h
                    BASENAME O2QcITS)

# ---- Test(s) ----

add_executable(testQcITS test/testITS.cxx)
target_link_libraries(testQcITS PRIVATE O2QcITS Boost::unit_test_framework O2::ITSMFTReconstruction O2::ITSBase O2::DetectorsBase O2QualityControl)
add_test(NAME testQcITS COMMAND testQcITS)
set_tests_properties(testQcITS PROPERTIES TIMEOUT 60)
install(
  TARGETS testQcITS
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

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
        itsQCTrendingCluster.json
        itsQCTrendingTracks.json
        itsFhr.json
        itsFee.json
        itsCluster.json
        itsNoisyPixel.json
        itsTrackSim.json 
        DESTINATION etc)

get_property(dirs
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
