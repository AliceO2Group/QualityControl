set(MODULE_NAME "O2QcMuonChambers")

# ---- Files ----

set(SRCS
  src/Helpers.cxx
  src/TH2ElecMapReductor.cxx
  src/ClusterChargeReductor.cxx
  src/ClusterSizeReductor.cxx
  src/GlobalHistogram.cxx
  src/DecodingErrorsPlotter.cxx
  src/HeartBeatPacketsPlotter.cxx
  src/FECSyncStatusPlotter.cxx
  src/RatesPlotter.cxx
  src/RatesTrendsPlotter.cxx
  src/OrbitsPlotter.cxx
  src/EfficiencyPlotter.cxx
  src/EfficiencyTrendsPlotter.cxx
  src/ClusterChargePlotter.cxx
  src/ClusterChargeTrendsPlotter.cxx
  src/ClusterSizePlotter.cxx
  src/ClusterSizeTrendsPlotter.cxx
  src/PedestalsTask.cxx
  src/DigitsTask.cxx
  src/RofsTask.cxx
  src/PreclustersTask.cxx
  src/DecodingTask.cxx
  src/ErrorTask.cxx
  src/PedestalsCheck.cxx
  src/DecodingCheck.cxx
  src/DigitsCheck.cxx
  src/PreclustersCheck.cxx
  src/TracksTask.cxx
  src/DecodingPostProcessing.cxx
  src/DigitsPostProcessing.cxx
  src/PreclustersPostProcessing.cxx
  src/PostProcessingConfigMCH.cxx
  src/TrendingTracks.cxx
  src/MCHAggregator.cxx
  src/QualityPostProcessing.cxx
  src/QualityCheck.cxx
  src/ClustersTask.cxx
)

set(HEADERS
  include/MCH/Helpers.h
  include/MCH/HistoOnCycle.h
  include/MCH/TH2ElecMapReductor.h
  include/MCH/ClusterChargeReductor.h
  include/MCH/ClusterSizeReductor.h
  include/MCH/GlobalHistogram.h
  include/MCH/DecodingErrorsPlotter.h
  include/MCH/HeartBeatPacketsPlotter.h
  include/MCH/FECSyncStatusPlotter.h
  include/MCH/RatesPlotter.h
  include/MCH/RatesTrendsPlotter.h
  include/MCH/OrbitsPlotter.h
  include/MCH/EfficiencyPlotter.h
  include/MCH/EfficiencyTrendsPlotter.h
  include/MCH/ClusterChargePlotter.h
  include/MCH/ClusterChargeTrendsPlotter.h
  include/MCH/ClusterSizePlotter.h
  include/MCH/ClusterSizeTrendsPlotter.h
  include/MCH/PedestalsTask.h
  include/MCH/DigitsTask.h
  include/MCH/PhysicsTaskDigits.h
  include/MCH/RofsTask.h
  include/MCH/PreclustersTask.h
  include/MCH/PhysicsTaskPreclusters.h
  include/MCH/DecodingTask.h
  include/MCH/ErrorTask.h
  include/MCH/PedestalsCheck.h
  include/MCH/DecodingCheck.h
  include/MCH/DigitsCheck.h
  include/MCH/PreclustersCheck.h
  include/MCH/TracksTask.h
  include/MCH/DedodingPostProcessing.h
  include/MCH/DigitsPostProcessing.h
  include/MCH/PreclustersPostProcessing.h
  include/MCH/PostProcessingConfigMCH.h
  include/MCH/TrendingTracks.h
  include/MCH/MCHAggregator.h
  include/MCH/QualityPostProcessing.h
  include/MCH/QualityCheck.h
  include/MCH/ClustersTask.h

  # legacy tasks
  include/MCH/PhysicsTaskDigits.h
  include/MCH/PhysicsTaskRofs.h
  include/MCH/PhysicsTaskPreclusters.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS})

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MODULE_NAME} PUBLIC
  $<TARGET_NAME_IF_EXISTS:O2::MCHMappingFactory>
  O2::CommonDataFormat
  O2::DataFormatsMCH
  O2::GPUCommon
  O2::MCHBase
  O2::MCHCalibration
  O2::MCHConstants
  O2::MCHDigitFiltering
  O2::MCHGlobalMapping
  O2::MCHMappingImpl4
  O2::MCHMappingSegContour
  O2::MCHPreClustering
  O2::MCHRawDecoder
  O2::MCHTracking
  O2QcMUONCommon
  O2QualityControl
)

target_compile_definitions(${MODULE_NAME} PRIVATE $<$<TARGET_EXISTS:O2::MCHMappingFactory>:MCH_HAS_MAPPING_FACTORY>)

# Digit.h is moving from MCHBase to DataFormatsMCH : let's handle both
# gracefully for the moment...
get_target_property(O2_INCLUDE_DIRS O2::CommonDataFormat INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(ROOT_INCLUDE_DIRS ROOT::Core INTERFACE_INCLUDE_DIRECTORIES)

set(CMAKE_REQUIRED_INCLUDES ${O2_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS})
check_include_file_cxx("DataFormatsMCH/Digit.h" HAVE_DIGIT_IN_DATAFORMATS)

if(HAVE_DIGIT_IN_DATAFORMATS)
    target_compile_definitions(${MODULE_NAME} PRIVATE HAVE_DIGIT_IN_DATAFORMATS)
endif()

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/MCH
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- ROOT dictionary ----

add_root_dictionary(${MODULE_NAME}
                    HEADERS
                            include/MCH/TH2ElecMapReductor.h
                            include/MCH/ClusterChargeReductor.h
                            include/MCH/GlobalHistogram.h
                            include/MCH/PedestalsTask.h
                            include/MCH/DigitsTask.h
                            include/MCH/RofsTask.h
                            include/MCH/PreclustersTask.h
                            include/MCH/DigitsCheck.h
                            include/MCH/PreclustersCheck.h
                            include/MCH/DecodingTask.h
                            include/MCH/DecodingCheck.h
                            include/MCH/ErrorTask.h
                            include/MCH/PedestalsCheck.h
                            include/MCH/sampa_header.h
                            include/MCH/TracksTask.h
                            include/MCH/DecodingPostProcessing.h
                            include/MCH/DigitsPostProcessing.h
                            include/MCH/PreclustersPostProcessing.h
                            include/MCH/PostProcessingConfigMCH.h
                            include/MCH/TrendingTracks.h
                            include/MCH/MCHAggregator.h
                            include/MCH/QualityPostProcessing.h
                            include/MCH/QualityCheck.h
                            include/MCH/ClustersTask.h

                            # legacy tasks
                            include/MCH/PhysicsTaskDigits.h
                            include/MCH/PhysicsTaskRofs.h
                            include/MCH/PhysicsTaskPreclusters.h
                    LINKDEF include/MCH/LinkDef.h)

# ---- Tests ----

set(
  TEST_SRCS
)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
