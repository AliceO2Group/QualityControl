# ---- Library ----

add_library(O2QcTPC)

target_sources(O2QcTPC PRIVATE src/PID.cxx
                             src/Tracking.cxx
                             src/Tracks.cxx
                             src/ROCReductor.cxx
                             src/Clusters.cxx
                             src/CalDetPublisher.cxx
                             src/PadCalibrationCheck.cxx
                             src/Utility.cxx
                             src/RawDigits.cxx
                             src/CheckOfTrendings.cxx
                             src/LaserTracks.cxx
                             src/LtrCalibReductor.cxx
                             src/ClusterVisualizer.cxx
                             src/TrendingTaskTPC.cxx
                             src/TrendingTaskConfigTPC.cxx
                             src/TH1ReductorTPC.cxx
                             src/TH2ReductorTPC.cxx
                             src/CheckForEmptyPads.cxx
                             src/QualityReductorTPC.cxx
                             src/DCSPTemperature.cxx
                             src/IDCs.cxx
                             src/QualityObserver.cxx
                             src/RatioGeneratorTPC.cxx
                             src/CheckOfSlices.cxx
                             src/GenericHistogramCheck.cxx
                             src/JunkDetection.cxx
                             src/CheckOfPads.cxx
                             src/CalPadClusterReductor.cxx
                             src/IDCScaleReductor.cxx
                             src/SACs.cxx
                             src/TPCAggregator.cxx)

target_include_directories(
  O2QcTPC
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${O2_ROOT}/include/GPU)

target_link_libraries(O2QcTPC
                      PUBLIC O2QualityControl
                             O2::TPCQC
                             O2::TPCWorkflow
                             O2::TPCCalibration
                             O2QcCommon)



add_root_dictionary(O2QcTPC
                    HEADERS include/TPC/PID.h
                            include/TPC/Tracking.h
                            include/TPC/Tracks.h
                            include/TPC/ROCReductor.h
                            include/TPC/Clusters.h
                            include/TPC/CalDetPublisher.h
                            include/TPC/PadCalibrationCheck.h
                            include/TPC/Utility.h
                            include/TPC/RawDigits.h
                            include/TPC/ClustersData.h
                            include/TPC/CheckOfTrendings.h
                            include/TPC/LaserTracks.h
                            include/TPC/LtrCalibReductor.h
                            include/TPC/ClusterVisualizer.h
                            include/TPC/SliceInfo.h
                            include/TPC/TrendingTaskTPC.h
                            include/TPC/TrendingTaskConfigTPC.h
                            include/TPC/ReductorTPC.h
                            include/TPC/TH1ReductorTPC.h
                            include/TPC/TH2ReductorTPC.h
                            include/TPC/CheckForEmptyPads.h
                            include/TPC/QualityReductorTPC.h
                            include/TPC/DCSPTemperature.h
                            include/TPC/IDCs.h
                            include/TPC/QualityObserver.h
                            include/TPC/RatioGeneratorTPC.h
                            include/TPC/CheckOfSlices.h
                            include/TPC/GenericHistogramCheck.h
                            include/TPC/JunkDetection.h
                            include/TPC/CheckOfPads.h
                            include/TPC/CalPadClusterReductor.h
                            include/TPC/IDCScaleReductor.h
                            include/TPC/SACs.h
                            include/TPC/TPCAggregator.h
                    LINKDEF include/TPC/LinkDef.h)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/TPC
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Test(s) ----

#set(TEST_SRCS test/testQcTPC.cxx) # uncomment to reenable the test which was empty

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PUBLIC O2QcTPC
                        PRIVATE Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()

# ---- Executables ----

set(EXE_SRCS
    run/runTPCQCTrackReader.cxx)

set(EXE_NAMES
    o2-qc-run-tpctrackreader)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE O2QcTPC ROOT::Tree)
endforeach()

# ---- Install ----

install(TARGETS O2QcTPC ${EXE_NAMES}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Install config files ----

install(FILES run/tpcQCPID_sampled.json
              run/tpcQCPID_direct.json
              run/tpcQCTracks_sampled.json
              run/tpcQCTracks_direct.json
              run/tpcQCTracking_direct.json
              run/tpcQCSimpleTrending.json
              run/tpcQCTrending.json
              run/tpcQCClusters_direct.json
              run/tpcQCTrackingFromExternal_direct.json
              run/tpcQCCalDetPublisher.json
              run/tpcQCPadCalibration.json
              run/tpcQCRawDigits_direct.json
              run/tpcQCTasks_multinode.json
              run/tpcQCCheckTrending.json
              run/tpcQCLaserTracks.json
              run/tpcQCTrending_laserCalib.json
              run/tpcQCClusterVisualizer.json
              run/tpcQCRawDigitVisualizer.json
              run/tpcQCTrending_slicer.json
              run/tpcQCTrending_canvas.json
              run/tpcQCDCSPTemperature.json
              run/tpcQCIDCs.json
              run/tpcQCQualityObserver.json
              run/tpcQCRatio_generator.json
              run/tpcQCCheckSlices.json
              run/tpcQCTracks_Generic.json
              run/tpcQCJunkDetection.json
              run/tpcQCPadCalibration_GenericPad.json
              run/tpcQCROCTrending.json
              run/tpcQCSACs.json
        DESTINATION etc)
