# ---- Library ----

add_library(O2QcEMCAL)

target_sources(O2QcEMCAL PRIVATE src/FECRateVisualization.cxx src/TriggerTask.cxx src/PedestalTask.cxx src/BCTask.cxx src/RawErrorCheck.cxx src/RawTask.cxx src/RawCheck.cxx src/CellTask.cxx src/CellCheck.cxx src/DigitsQcTask.cxx src/DigitCheck.cxx src/OccupancyReductor.cxx src/OccupancyToFECReductor.cxx src/ClusterTask.cxx src/RawErrorTask.cxx src/CalibMonitoringTask.cxx src/SupermoduleProjectorTask.cxx src/BadChannelMapReductor.cxx src/TimeCalibParamReductor.cxx src/SupermoduleProjectionReductor.cxx src/SubdetectorProjectionReductor.cxx src/BCVisualization.cxx src/CalibCheck.cxx src/NumPatchesPerFastORCheck.cxx src/PedestalChannelCheck.cxx src/PayloadPerEventDDLCheck.cxx src/RawErrorCheckAll.cxx src/CellTimeCalibCheck.cxx src/CellAmpCheck.cxx src/TrendGraphCheck.cxx src/NumPhysTriggCheck.cxx)

target_include_directories(
  O2QcEMCAL
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(O2QcEMCAL PUBLIC O2QualityControl O2::DetectorsBase O2::EMCALBase O2::EMCALReconstruction O2::CCDB O2::EMCALCalib O2::DataFormatsCTP ROOT::Spectrum)

add_root_dictionary(O2QcEMCAL
  HEADERS include/EMCAL/DigitsQcTask.h
  include/EMCAL/BCTask.h
  include/EMCAL/DigitCheck.h
  include/EMCAL/RawTask.h
  include/EMCAL/RawCheck.h
  include/EMCAL/CellTask.h
  include/EMCAL/CellCheck.h
  include/EMCAL/OccupancyReductor.h
  include/EMCAL/OccupancyToFECReductor.h
  include/EMCAL/ClusterTask.h
  include/EMCAL/RawErrorTask.h
  include/EMCAL/RawErrorCheck.h
  include/EMCAL/CalibMonitoringTask.h
  include/EMCAL/SupermoduleProjectorTask.h
  include/EMCAL/BadChannelMapReductor.h
  include/EMCAL/TimeCalibParamReductor.h
  include/EMCAL/SupermoduleProjectionReductor.h
  include/EMCAL/SubdetectorProjectionReductor.h
  include/EMCAL/BCVisualization.h
  include/EMCAL/PedestalTask.h
  include/EMCAL/TriggerTask.h
  include/EMCAL/CalibCheck.h
  include/EMCAL/NumPatchesPerFastORCheck.h
  include/EMCAL/PedestalChannelCheck.h
  include/EMCAL/PayloadPerEventDDLCheck.h
  include/EMCAL/RawErrorCheckAll.h
  include/EMCAL/CellTimeCalibCheck.h
  include/EMCAL/CellAmpCheck.h
  include/EMCAL/TrendGraphCheck.h
  include/EMCAL/DrawGridlines.h
  include/EMCAL/FECRateVisualization.h
  include/EMCAL/NumPhysTriggCheck.h
  include/EMCAL/NumPhysTriggCheck.h
  include/EMCAL/IndicesConverter.h
  LINKDEF include/EMCAL/LinkDef.h)

install(TARGETS O2QcEMCAL
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
  DIRECTORY etc DESTINATION Modules/EMCAL
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/EMCAL
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Executables ----

# ---- Tests ----
set(
  TEST_SRCS
)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework O2::CCDB O2::EMCALCalib)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()
