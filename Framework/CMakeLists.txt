# ---- Files ----

set(
  SRCS
  src/MonitorObject.cxx
  src/Quality.cxx
  src/ObjectsManager.cxx
  src/Checker.cxx
  src/CheckerFactory.cxx
  src/CheckInterface.cxx
  src/DatabaseFactory.cxx
  src/CcdbDatabase.cxx
  src/InformationService.cxx
  src/InformationServiceDump.cxx
  src/TaskRunner.cxx
  src/TaskRunnerFactory.cxx
  src/TaskInterface.cxx
  src/RepositoryBenchmark.cxx
  src/HistoMerger.cxx
  src/InfrastructureGenerator.cxx
  src/runnerUtils.h
  src/ServiceDiscovery.cxx
)

set(
  HEADERS # needed for the dictionary generation
  include/QualityControl/MonitorObject.h
  include/QualityControl/Quality.h
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
)

if(ENABLE_MYSQL)
  list(APPEND SRCS src/MySqlDatabase.cxx)
endif()

# Produce the final Version.h using template Version.h.in and substituting variables. We don't want to polute our source
# tree with it, thus putting it in binary tree.
configure_file(
  "include/QualityControl/Version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/${MODULE_NAME}/Version.h" @ONLY
)

# ---- ROOT dictionary ----

# ROOT dictionary the following root macros expect include dirs to be set as directory property TODO how to generate
# this automatically ? ? ?
get_directory_property(include_dirs INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/include")
get_target_property(config_inc_dir AliceO2::Configuration INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${config_inc_dir}")
get_target_property(arrow_inc_dir Arrow::Arrow INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${arrow_inc_dir}")
get_target_property(monitoring_inc_dir AliceO2::Monitoring INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${monitoring_inc_dir}")
get_target_property(infologger_inc_dir AliceO2::InfoLogger INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${infologger_inc_dir}")
get_target_property(o2_inc_dir AliceO2::AliceO2 INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${o2_inc_dir}")
get_target_property(common_inc_dir AliceO2::Common INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${common_inc_dir}")
get_target_property(boost_inc_dir Boost::container INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${boost_inc_dir}")
get_target_property(fairlogger_inc_dir FairLogger::FairLogger INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${fairlogger_inc_dir}")
list(APPEND include_dirs "${FAIRROOT_INCLUDE_DIR}")
list(APPEND include_dirs "${FairMQ_INCDIR}")
list(APPEND include_dirs "${FairMQ_INCDIR}/fairmq")
list(REMOVE_DUPLICATES include_dirs)
include_directories(${include_dirs})

set(dict "QualityControlDict")
set(dict_src ${CMAKE_CURRENT_BINARY_DIR}/${dict}.cxx)
set_source_files_properties(${dict_src} PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
set_source_files_properties(${dict_src} PROPERTIES GENERATED TRUE)

root_generate_dictionary("${dict}" ${HEADERS} LINKDEF include/QualityControl/LinkDef.h)

# TODO review how and what to install for dictionary
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${dict}_rdict.pcm ${CMAKE_CURRENT_BINARY_DIR}/lib${dict}.rootmap
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# ---- Library ----

add_library(QualityControl SHARED ${SRCS} QualityControlDict.cxx)

target_include_directories(
  QualityControl
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
  QualityControl
  PUBLIC
    Boost::boost
    FairLogger::FairLogger
    FairMQ::FairMQ
    ROOT::Hist
    AliceO2::Common
    AliceO2::InfoLogger
    AliceO2::Monitoring
    AliceO2::Configuration
    ROOT::Net
    AliceO2::AliceO2
    Boost::container
  PRIVATE
    Boost::system
    $<$<BOOL:${ENABLE_MYSQL}>:MySQL::MySQL>
    $<$<BOOL:${ENABLE_MYSQL}>:ROOT::RMySQL>
    ROOT::Gui
    CURL::CURL
)

target_compile_definitions(QualityControl
  PRIVATE
  $<$<BOOL:${ENABLE_MYSQL}>:_WITH_MYSQL>
)

# ---- Executables ----

set(
  EXE_SRCS
  src/runInformationService.cxx
  src/runInformationServiceDump.cxx
  src/runBasic.cxx
  src/runAdvanced.cxx
  src/runReadout.cxx
  src/runMergerTest.cxx
  src/runReadoutForDataDump.cxx
  src/runRepositoryBenchmark.cxx
)

set(
  EXE_NAMES
  o2-qc-info-service
  o2-qc-info-service-dump
  o2-qc-run-basic
  o2-qc-run-advanced
  o2-qc-run-readout
  o2-qc-run-merger-test
  o2-qc-run-readout-for-data-dump
  o2-qc-repository-benchmark
)

# These were the original names before the convention changed.
# We will get rid of them but for the time being we want to create symlinks to avoid confusion.
set(
  EXE_OLD_NAMES
  qcInfoService
  qcInfoServiceDump
  qcRunBasic
  qcRunAdvanced
  qcRunReadout
  runMergerTest
  qcRunReadoutForDataDump
  repositoryBenchmark
)

# As per https://stackoverflow.com/questions/35765106/symbolic-links-cmake
macro(install_symlink filepath sympath)
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})")
  install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
endmacro(install_symlink)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  list(GET EXE_OLD_NAMES ${i} oldname)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QualityControl CURL::CURL)
  install_symlink(${CMAKE_INSTALL_FULL_BINDIR}/${name} ${CMAKE_INSTALL_FULL_BINDIR}/${oldname})
endforeach()

# ---- Gui ----

set(DATADUMP "")
if(GLFW_FOUND)
  set(DATADUMP "o2-qc-data-dump")

  set(
    GUI_SRCS
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_impl_glfw_gl3.cpp
    src/imgui/gl3w.c
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_demo.cpp
    src/imgui/BaseGui.cxx
    src/DataDumpGui.cxx
    src/runDataDump.cxx
  )

  add_executable(${DATADUMP} ${GUI_SRCS})

  target_link_libraries(${DATADUMP} PRIVATE QualityControl GLFW::GLFW)
else()
  message(STATUS "GLFW not found, ${DATADUMP} will not be built")
endif()

# ---- Tests ----

set(
  TEST_SRCS
  test/testDbFactory.cxx
  test/testMonitorObject.cxx
  test/testPublisher.cxx
  test/testQcInfoLogger.cxx
  test/testInfrastructureGenerator.cxx
  test/testQCTask.cxx
  test/testQuality.cxx
)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE QualityControl Boost::unit_test_framework )
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()

install(
  FILES
  test/testQCFactory.json
  DESTINATION test
)

# ---- Install ----

# Build targets with install rpath on Mac to dramatically speed up installation
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
  endif()
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# Install library and binaries
install(
  TARGETS QualityControl ${EXE_NAMES} ${DATADUMP}
  EXPORT QualityControlTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/QualityControl DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Create version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfigVersion.cmake"
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY AnyNewerVersion
)

# Export targets
install(
  EXPORT QualityControlTargets
  FILE QualityControlTargets.cmake
  NAMESPACE QualityControl::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl
)

# Configure and install Config files
configure_package_config_file(
  ../cmake/QualityControlConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfig.cmake
  INSTALL_DESTINATION
  "${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl"
  PATH_VARS
  CMAKE_INSTALL_PREFIX
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl
)

# ---- Extra scripts ----

install(PROGRAMS script/o2-qc-database-setup.sh DESTINATION bin)
install(
  FILES
    example-default.json
    alfa.json
    dataDump.json
  DESTINATION etc
)
install(
  FILES
    basic.json
    basic-no-sampling.json
    advanced.json
    readout.json
    readout-no-sampling.json
    readoutForDataDump.json
  DESTINATION etc
)
install(
  PROGRAMS
    script/RepoCleaner/1_per_hour.py
    script/RepoCleaner/Ccdb.py
    script/RepoCleaner/repoCleaner.py
  DESTINATION
    bin
)
install(
  FILES
    script/RepoCleaner/config.yaml
  DESTINATION
    etc
)