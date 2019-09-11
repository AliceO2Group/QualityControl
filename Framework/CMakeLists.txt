# Produce the final Version.h using template Version.h.in and substituting
# variables. We don't want to polute our source tree with it, thus putting it in
# binary tree.
configure_file("include/QualityControl/Version.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/${MODULE_NAME}/Version.h"
               @ONLY)

# ---- Library for the types ----

add_library(QualityControlTypes
  src/MonitorObject.cxx
  src/Quality.cxx
)

target_include_directories(
  QualityControlTypes
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QualityControlTypes
  PUBLIC
  ROOT::Hist
  AliceO2::Common
)

add_root_dictionary(QualityControlTypes
  HEADERS include/QualityControl/MonitorObject.h
  include/QualityControl/Quality.h
  LINKDEF include/QualityControl/TypesLinkDef.h
  BASENAME QualityControlTypes
  )

# ---- Library ----

add_library(QualityControl
            src/ObjectsManager.cxx
            src/Checker.cxx
            src/CheckerFactory.cxx
            src/CheckInterface.cxx
            src/DatabaseFactory.cxx
            src/CcdbDatabase.cxx
            src/InformationService.cxx
            src/InformationServiceDump.cxx
            src/TaskFactory.cxx
            src/TaskRunner.cxx
            src/TaskRunnerFactory.cxx
            src/TaskInterface.cxx
            src/RepositoryBenchmark.cxx
            src/HistoMerger.cxx
            src/InfrastructureGenerator.cxx
            src/ServiceDiscovery.cxx)

if(ENABLE_MYSQL)
  target_sources(QualityControl PRIVATE src/MySqlDatabase.cxx)
endif()

target_include_directories(
  QualityControl
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QualityControl
                      PUBLIC Boost::boost
                             FairLogger::FairLogger
                             FairMQ::FairMQ
                             ROOT::Hist
                             AliceO2::Common
                             AliceO2::InfoLogger
                             AliceO2::Monitoring
                             AliceO2::Configuration
                             ROOT::Net
                             Boost::container
                             O2::Framework
                             O2::CCDB
                             ms_gsl::ms_gsl
                             QualityControlTypes
                      PRIVATE Boost::system
                              $<$<BOOL:${ENABLE_MYSQL}>:MySQL::MySQL>
                              $<$<BOOL:${ENABLE_MYSQL}>:ROOT::RMySQL> ROOT::Gui
                              CURL::libcurl)

target_compile_definitions(QualityControl PRIVATE
                           $<$<BOOL:${ENABLE_MYSQL}>:_WITH_MYSQL>)

add_root_dictionary(QualityControl
                    HEADERS
                            include/QualityControl/CheckInterface.h
                            include/QualityControl/Checker.h
                            include/QualityControl/CheckerFactory.h
                            include/QualityControl/DatabaseInterface.h
                            include/QualityControl/CcdbDatabase.h
                            include/QualityControl/ServiceDiscovery.h
                            include/QualityControl/TaskRunner.h
                            include/QualityControl/TaskRunnerFactory.h
                            include/QualityControl/HistoMerger.h
                            include/QualityControl/InfrastructureGenerator.h
                    LINKDEF include/QualityControl/LinkDef.h
                    BASENAME QualityControl)

# ---- Executables ----

set(EXE_SRCS
    src/runInformationService.cxx
    src/runInformationServiceDump.cxx
    src/runDataProducer.cxx
    src/runQC.cxx
    src/runBasic.cxx
    src/runTPCQCPID.cxx
    src/runAdvanced.cxx
    src/runReadout.cxx
    src/runMergerTest.cxx
    src/runReadoutForDataDump.cxx
    src/runRepositoryBenchmark.cxx)

set(EXE_NAMES
    o2-qc-info-service
    o2-qc-info-service-dump
    o2-qc-run-producer
    o2-qc
    o2-qc-run-basic
    o2-qc-run-tpcpid
    o2-qc-run-advanced
    o2-qc-run-readout
    o2-qc-run-merger-test
    o2-qc-run-readout-for-data-dump
    o2-qc-repository-benchmark)

# These were the original names before the convention changed. We will get rid
# of them but for the time being we want to create symlinks to avoid confusion.
set(EXE_OLD_NAMES
    qcInfoService
    qcInfoServiceDump
    qcRunProducer
    o2-qc-run-qc
    qcRunBasic
    qcRunTPCPID
    qcRunAdvanced
    qcRunReadout
    runMergerTest
    qcRunReadoutForDataDump
    repositoryBenchmark)

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
  target_link_libraries(${name} PRIVATE QualityControl CURL::libcurl ROOT::Tree)
  install_symlink(${name} ${CMAKE_INSTALL_FULL_BINDIR}/${oldname})
endforeach()

# ---- Gui ----

set(DATADUMP "")
if(GLFW_FOUND)
  set(DATADUMP "o2-qc-data-dump")

  set(IMGUI_SRCS
      src/imgui/imgui.cpp
      src/imgui/imgui_draw.cpp
      src/imgui/imgui_impl_glfw_gl3.cpp
      src/imgui/gl3w.c
      src/imgui/imgui_widgets.cpp
      src/imgui/imgui_demo.cpp
      src/imgui/BaseGui.cxx)
  set_source_files_properties(${IMGUI_SRCS} PROPERTIES COMPILE_FLAGS -w)

  set(GUI_SRCS src/DataDumpGui.cxx src/runDataDump.cxx)

  add_executable(${DATADUMP} ${GUI_SRCS} ${IMGUI_SRCS})

  target_link_libraries(${DATADUMP} PRIVATE QualityControl glfw)
else()
  message(STATUS "GLFW not found, ${DATADUMP} will not be built")
endif()

# ---- Tests ----

set(TEST_SRCS
    test/testDbFactory.cxx
    test/testMonitorObject.cxx
    test/testPublisher.cxx
    test/testQcInfoLogger.cxx
    test/testInfrastructureGenerator.cxx
    test/testTaskInterface.cxx
    test/testTaskRunner.cxx
    test/testCheckInterface.cxx
    test/testChecker.cxx
    test/testQuality.cxx
    test/testObjectsManager.cxx
    test/testCcdbDatabase.cxx
    test/testCcdbDatabaseExtra.cxx
    test/testWorkflow.cxx)

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
    "-b --run")

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
                        PRIVATE QualityControl Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name} ${arg})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 30)
endforeach()

foreach(t testTaskInterface testWorkflow testTaskRunner
        testInfrastructureGenerator)
  target_sources(${t} PRIVATE
                 ${CMAKE_BINARY_DIR}/getTestDataDirectory.cxx)
  target_include_directories(${t} PRIVATE ${CMAKE_SOURCE_DIR})
endforeach()

set_property(TEST testWorkflow PROPERTY TIMEOUT 10)
set_property(TEST testWorkflow PROPERTY LABELS slow)
set_property(TEST testObjectsManager PROPERTY TIMEOUT 60)
set_property(TEST testObjectsManager PROPERTY LABELS slow)
set_property(TEST testCcdbDatabase PROPERTY TIMEOUT 60)
set_property(TEST testCcdbDatabase PROPERTY LABELS slow)
# Temporarily disable tests that fail on CI
set_property(TEST testWorkflow PROPERTY LABELS manual)

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
install(TARGETS QualityControl QualityControlTypes ${EXE_NAMES} ${DATADUMP}
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

# ---- Extra scripts ----

install(FILES example-default.json
              alfa.json
              dataDump.json
              basic.json
              tpcQCPID.json
              basic-no-sampling.json
              advanced.json
              readout.json
              readout-no-sampling.json
              readoutForDataDump.json
              script/RepoCleaner/config.yaml
              streamerinfos.root
        DESTINATION etc)

install(PROGRAMS script/RepoCleaner/1_per_hour.py script/RepoCleaner/Ccdb.py
                 script/RepoCleaner/repoCleaner.py
                 script/o2-qc-database-setup.sh
        DESTINATION bin)

file(COPY test/testSharedConfig.json test/testWorkflow.json
     DESTINATION ${CMAKE_BINARY_DIR}/tests/)

file(COPY streamerinfos.root DESTINATION ${CMAKE_BINARY_DIR}/etc/)
