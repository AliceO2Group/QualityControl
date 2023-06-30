# Produce the final Version.h using template Version.h.in and substituting
# variables. We don't want to polute our source tree with it, thus putting it in
# binary tree.
configure_file("include/QualityControl/Version.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/QualityControl/Version.h"
               @ONLY)

# ---- Library for the types ----

add_library(O2QualityControlTypes
  src/MonitorObject.cxx
  src/QualityObject.cxx
  src/Quality.cxx
)

target_include_directories(
  O2QualityControlTypes
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(O2QualityControlTypes
  PUBLIC
  ROOT::Hist
  AliceO2::Common
  O2::DataFormatsQualityControl
  AliceO2::BookkeepingProtoApi
)

add_root_dictionary(O2QualityControlTypes
  HEADERS include/QualityControl/MonitorObject.h
  include/QualityControl/QualityObject.h
  include/QualityControl/Quality.h
  include/QualityControl/Activity.h
  LINKDEF include/QualityControl/TypesLinkDef.h)

# ---- Library ----

add_library(O2QualityControl
  src/Activity.cxx
  src/ActivityHelpers.cxx
  src/ObjectsManager.cxx
  src/CheckRunner.cxx
  src/AggregatorRunner.cxx
  src/CheckRunnerFactory.cxx
  src/AggregatorRunnerFactory.cxx
  src/CheckInterface.cxx
  src/DatabaseFactory.cxx
  src/CcdbDatabase.cxx
  src/QcInfoLogger.cxx
  src/TaskFactory.cxx
  src/TaskRunner.cxx
  src/TaskRunnerFactory.cxx
  src/TaskInterface.cxx
  src/UserCodeInterface.cxx
  src/RepositoryBenchmark.cxx
  src/InfrastructureGenerator.cxx
  src/InfrastructureSpecReader.cxx
  src/Check.cxx
  src/Aggregator.cxx
  src/ServiceDiscovery.cxx
  src/Triggers.cxx
  src/TriggerHelpers.cxx
  src/PostProcessingRunner.cxx
  src/PostProcessingFactory.cxx
  src/PostProcessingConfig.cxx
  src/PostProcessingInterface.cxx
  src/PostProcessingDevice.cxx
  src/TrendingTask.cxx
  src/TrendingTaskConfig.cxx
  src/DummyDatabase.cxx
  src/DataProducer.cxx
  src/HistoProducer.cxx
  src/DataProducerExample.cxx
  src/MonitorObjectCollection.cxx
  src/UpdatePolicyManager.cxx
  src/AdvancedWorkflow.cxx
  src/QualitiesToTRFCollectionConverter.cxx
  src/Calculators.cxx
  src/DataSourceSpec.cxx
  src/RootFileSink.cxx
  src/RootFileSource.cxx
  src/UpdatePolicyType.cxx
  src/RootClassFactory.cxx
  src/ConfigParamGlo.cxx
  src/SliceTrendingTask.cxx
  src/SliceTrendingTaskConfig.cxx
  src/Bookkeeping.cxx
  src/CustomParameters.cxx
  src/runnerUtils.cxx
  src/Timekeeper.cxx
  src/TimekeeperSynchronous.cxx
  src/TimekeeperAsynchronous.cxx)


target_include_directories(
  O2QualityControl
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

target_link_libraries(O2QualityControl
                      PUBLIC Boost::boost
                             FairLogger::FairLogger
                             FairMQ::FairMQ
                             ROOT::Hist
                             ROOT::TreePlayer
                             AliceO2::Common
                             AliceO2::InfoLogger
                             AliceO2::Monitoring
                             AliceO2::Configuration
                             AliceO2::Occ
                             ROOT::Net
                             Boost::container
                             O2::Framework
                             O2::CCDB
                             O2QualityControlTypes
                             O2::Mergers
                             O2::DataSampling
                             O2::DataFormatsQualityControl
                             O2::DetectorsBase
                             O2::GlobalTracking
                      PRIVATE Boost::system
                              CURL::libcurl)

add_root_dictionary(O2QualityControl
  HEADERS
  include/QualityControl/CheckInterface.h
  include/QualityControl/TaskInterface.h
  include/QualityControl/UserCodeInterface.h
  include/QualityControl/AggregatorInterface.h
  include/QualityControl/PostProcessingInterface.h
  include/QualityControl/TrendingTask.h
  include/QualityControl/SliceInfoTrending.h
  include/QualityControl/SliceTrendingTask.h
  include/QualityControl/MonitorObjectCollection.h
  LINKDEF include/QualityControl/LinkDef.h)

# ---- Executables ----

set(EXE_SRCS
  src/runDataProducer.cxx
  src/runDataProducerExample.cxx
  src/runHistoProducer.cxx
  src/runQC.cxx
  src/runBasic.cxx
  src/runAdvanced.cxx
  src/runReadout.cxx
  src/runRepositoryBenchmark.cxx
  src/runPostProcessing.cxx
  src/runPostProcessingOCC.cxx
  src/runLocationCalculator.cxx
  src/runMergerCalculator.cxx
  src/runUploadRootObjects.cxx
  src/runFileMerger.cxx
  src/runMetadataUpdater.cxx
  src/runBookkeepingBenchmark.cxx)

set(EXE_NAMES
  o2-qc-run-producer
  o2-qc-run-producer-basic
  o2-qc-run-histo-producer
  o2-qc
  o2-qc-run-basic
  o2-qc-run-advanced
  o2-qc-run-readout
  o2-qc-repository-benchmark
  o2-qc-run-postprocessing
  o2-qc-run-postprocessing-occ
  o2-qc-location-calculator
  o2-qc-merger-calculator
  o2-qc-upload-root-objects
  o2-qc-file-merger
  o2-qc-metadata-updater
  o2-qc-bk-benchmark)

# These were the original names before the convention changed. We will get rid
# of them but for the time being we want to create symlinks to avoid confusion.
set(EXE_OLD_NAMES
  qcRunProducer
  o2-qc-run-producer-basic
  o2-qc-run-histo-producer
  o2-qc-run-qc
  qcRunBasic
  qcRunAdvanced
  qcRunReadout
  qcRunReadoutForDataDump
  repositoryBenchmark
  qcRunPostProcessing
  qcRunPostProcessingOCC
  o2-qc-location-calculator
  o2-qc-merger-calculator
  o2-qc-upload-root-objects
  o2-qc-file-merger
  o2-qc-metadata-updater
  o2-qc-bk-benchmark)


# As per https://stackoverflow.com/questions/35765106/symbolic-links-cmake
macro(install_symlink filepath sympath)
  install(DIRECTORY DESTINATION bin) # just in case it is not there yet
  install(
    CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})"
  )
  install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
endmacro(install_symlink)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  list(GET EXE_OLD_NAMES ${i} oldname)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE O2QualityControl CURL::libcurl ROOT::Tree)
  install_symlink(${name} ${CMAKE_INSTALL_FULL_BINDIR}/${oldname})
endforeach()

# ---- Tests ----

add_executable(o2-qc-test-core 
               test/testActivity.cxx
               test/testActivityHelpers.cxx
               test/testAggregatorInterface.cxx
               test/testAggregatorRunner.cxx
               test/testCheckInterface.cxx
               test/testCheckRunner.cxx
               test/testInfrastructureGenerator.cxx
               test/testMonitorObject.cxx
               test/testPolicyManager.cxx
               test/testPostProcessingRunner.cxx
               test/testQuality.cxx
               test/testQualityObject.cxx
               test/testTaskInterface.cxx
               test/testTimekeeper.cxx
               test/testTriggerHelpers.cxx
               test/testVersion.cxx
               )
set_property(TARGET o2-qc-test-core
             PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
target_link_libraries(o2-qc-test-core PRIVATE O2QualityControl O2::Catch2)
target_link_libraries(o2-qc-test-core PRIVATE O2::EMCALBase O2::EMCALCalib)
target_include_directories(o2-qc-test-core PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME o2-qc-test-core COMMAND o2-qc-test-core)
set_tests_properties(o2-qc-test-core PROPERTIES TIMEOUT 30)
target_sources(o2-qc-test-core PRIVATE
               ${CMAKE_BINARY_DIR}/getTestDataDirectory.cxx)
target_include_directories(o2-qc-test-core PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(o2-qc-test-core PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

set(TEST_SRCS
    test/testDbFactory.cxx
    test/testPublisher.cxx
    test/testQcInfoLogger.cxx
    test/testTaskRunner.cxx
    test/testCheck.cxx
    test/testObjectsManager.cxx
    test/testCcdbDatabase.cxx
    test/testCcdbDatabaseExtra.cxx
    test/testTriggers.cxx
    test/testPostProcessingInterface.cxx
    test/testPostProcessingConfig.cxx
    test/testReductor.cxx
    test/testTrendingTask.cxx
    test/testCheckWorkflow.cxx
    test/testWorkflow.cxx
    test/testRepoPathUtils.cxx
    test/testQualitiesToTRFCollectionConverter.cxx
    test/testUserCodeInterface.cxx
    test/testCustomParameters.cxx
  )

set(TEST_ARGS
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    "-b --run"
    "-b --run"
    ""
    ""
    ""
    ""
  )

list(LENGTH TEST_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET TEST_SRCS ${i} test)
  list(GET TEST_ARGS ${i} arg)
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})
  string(REPLACE " " ";" arg "${arg}") # make list of string (arguments) out of
                                       # one string

  add_executable(${test_name} ${test})
  set_property(TARGET ${test_name}
               PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  target_link_libraries(${test_name}
                        PRIVATE O2QualityControl Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name} ${arg})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 30)
endforeach()

foreach(t testWorkflow testTaskRunner testCheckWorkflow
        testPostProcessingConfig testPostProcessingInterface
        testCheck testTrendingTask)
  target_sources(${t} PRIVATE
                 ${CMAKE_BINARY_DIR}/getTestDataDirectory.cxx)
  target_include_directories(${t} PRIVATE ${CMAKE_SOURCE_DIR})
endforeach()

target_include_directories(testCcdbDatabase PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

set_property(TEST testWorkflow PROPERTY TIMEOUT 40)
set_property(TEST testWorkflow PROPERTY LABELS slow)
set_property(TEST testCheckWorkflow PROPERTY TIMEOUT 50)
set_property(TEST testCheckWorkflow PROPERTY LABELS slow)
set_property(TEST testObjectsManager PROPERTY TIMEOUT 30)
set_property(TEST testObjectsManager PROPERTY LABELS slow)
set_property(TEST testCcdbDatabase PROPERTY TIMEOUT 60)
set_property(TEST testCcdbDatabase PROPERTY LABELS slow CCDB)
set_property(TEST testCcdbDatabaseExtra PROPERTY LABELS manual CCDB)
set_property(TEST testTrendingTask PROPERTY LABELS manual)
set_property(TEST testDbFactory PROPERTY LABELS CCDB)
set_property(TEST testUserCodeInterface PROPERTY LABELS CCDB)

# Add a functional test (QC-336)
string(RANDOM UNIQUE_ID)
configure_file(basic-functional.json.in ${CMAKE_BINARY_DIR}/tests/basic-functional.json) # substitute the unique id in the task name
add_test(NAME functional_test COMMAND o2-qc-functional-test.sh)
set_tests_properties(functional_test PROPERTIES ENVIRONMENT "JSON_DIR=${CMAKE_BINARY_DIR}/tests;UNIQUE_ID=${UNIQUE_ID}")
set_property(TEST functional_test PROPERTY LABELS slow)
set_property(TEST functional_test PROPERTY TIMEOUT 45)

include(GenerateUniquePort)
o2_generate_unique_port(UNIQUE_PORT_1)
# UNIQUE_PORT_2 is UNIQUE_PORT_1 + 1
# since we are guaranteed UNIQUE_PORT_1 is even.
math(EXPR UNIQUE_PORT_2 "${UNIQUE_PORT_1} + 1")
configure_file(multinode-test.json.in ${CMAKE_BINARY_DIR}/tests/multinode-test.json)
add_test(NAME multinode_test COMMAND o2-qc-multinode-test.sh)
set_tests_properties(multinode_test
  PROPERTIES ENVIRONMENT "JSON_DIR=${CMAKE_BINARY_DIR}/tests;UNIQUE_PORT_1=${UNIQUE_PORT_1};UNIQUE_PORT_2=${UNIQUE_PORT_2}")
set_property(TEST multinode_test PROPERTY LABELS slow)
set_property(TEST multinode_test PROPERTY TIMEOUT 75)

# Batch processing test
string(RANDOM UNIQUE_ID)
configure_file(batch-test.json.in ${CMAKE_BINARY_DIR}/tests/batch-test.json) # substitute the unique id in the task name
add_test(NAME batch_test COMMAND o2-qc-batch-test.sh)
set_tests_properties(batch_test PROPERTIES ENVIRONMENT "JSON_DIR=${CMAKE_BINARY_DIR}/tests;UNIQUE_ID=${UNIQUE_ID}")
set_property(TEST batch_test PROPERTY LABELS slow)
set_property(TEST batch_test PROPERTY TIMEOUT 45)

# ---- Install ----

# Build targets with install rpath on Mac to dramatically speed up installation
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
          "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
  endif()
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# Install library and binaries
install(TARGETS O2QualityControl O2QualityControlTypes ${EXE_NAMES}
        EXPORT QualityControlTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/QualityControl
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Create version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion)

# Export targets
install(EXPORT QualityControlTargets
        FILE QualityControlTargets.cmake
        NAMESPACE QualityControl::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl)

# Configure and install Config files
configure_package_config_file(
  ../cmake/QualityControlConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfig.cmake
  INSTALL_DESTINATION
  "${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl"
  PATH_VARS
  CMAKE_INSTALL_PREFIX)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl)

# ---- Install config files and scripts ----

install(FILES example-default.json
              alfa.json
              basic.json
              basic-aggregator.json
              basic-external-histo.json
              basic-no-sampling.json
              advanced.json
              advanced-aggregator.json
              advanced-external-histo.json
              multiNode.json
              readout.json
              readout-no-sampling.json
              postprocessing.json
              streamerinfos.root
              streamerinfos_v017.root
        DESTINATION etc)

install(PROGRAMS script/o2-qc-functional-test.sh
                 script/o2-qc-multinode-test.sh
                 script/o2-qc-batch-test.sh
        DESTINATION bin)

# ---- Copy test files ----
#
# Using file(COPY is wrong because cmake will not detect when the files are modified.
# Using install is also wrong because we don't want to install nor package the test files.
# The solution is to use configure_file with COPYONLY.

set(TEST_FILES
  "testSharedConfig.json"
  "testEmptyConfig.json"
  "testCheckWorkflow.json"
  "testTrendingTask.json"
  "testWorkflow.json")
set(TEST_FILES_PREFIXED ${TEST_FILES})
list(TRANSFORM TEST_FILES_PREFIXED PREPEND ${CMAKE_BINARY_DIR}/tests/)

foreach(test_file ${TEST_FILES})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test/${test_file} ${CMAKE_BINARY_DIR}/tests/${test_file} COPYONLY)
endforeach()
