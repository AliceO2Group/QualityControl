####################################
# Files
####################################

set(SRCS
  src/TaskInterface.cxx
  src/MonitorObject.cxx
  src/Quality.cxx
  src/ObjectsManager.cxx
  src/Checker.cxx
  src/CheckerDataProcessor.cxx
  src/CheckerDataProcessorFactory.cxx
  src/CheckInterface.cxx
  src/DatabaseFactory.cxx
  src/ClientDataProvider.cxx
  src/AlfaReceiverForTests.cxx
  src/TaskDevice.cxx
  src/SpyDevice.cxx
  src/SpyMainFrame.cxx
  src/CcdbDatabase.cxx
  src/InformationService.cxx
  src/InformationServiceDump.cxx
  src/TaskDataProcessor.cxx
  src/TaskDataProcessorFactory.cxx
  src/TaskInterfaceDPL.cxx
  src/RepositoryBenchmark.cxx
  src/HistoMerger.cxx
  )

set(HEADERS # needed for the dictionary generation
  include/QualityControl/MonitorObject.h
  include/QualityControl/Quality.h
  include/QualityControl/CheckInterface.h
  include/QualityControl/CheckerDataProcessor.h
  include/QualityControl/CheckerDataProcessorFactory.h
  include/QualityControl/SpyMainFrame.h
  include/QualityControl/DatabaseInterface.h
  include/QualityControl/CcdbDatabase.h
  include/QualityControl/TaskDataProcessor.h
  include/QualityControl/TaskDataProcessorFactory.h
  include/QualityControl/HistoMerger.h
  )

set(SRCS_TOBJECT2JSON
  src/TObject2Json.cxx
  src/TObject2JsonServer.cxx
  src/TObject2JsonWorker.cxx
  src/TObject2JsonMySql.cxx
  src/TObject2JsonBackendFactory.cxx
  src/TObject2JsonCcdb.cxx
  )

if (MYSQL_FOUND)
  list(APPEND SRCS src/MySqlDatabase.cxx)
endif ()

# Produce the final Version.h using template Version.h.in and substituting variables.
# We don't want to polute our source tree with it, thus putting it in binary tree.
configure_file("include/QualityControl/Version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/${MODULE_NAME}/Version.h"
  @ONLY
  )

####################################
# ROOT dictionary
####################################

# ROOT dictionary
# the following root macros expect include dirs to be set as directory property
# TODO how to generate this automatically ? ? ?
get_directory_property(include_dirs INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/include")
get_target_property(config_inc_dir Configuration::Configuration INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${config_inc_dir}")
get_target_property(arrow_inc_dir Arrow::Arrow INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${arrow_inc_dir}")
get_target_property(monitoring_inc_dir Monitoring::Monitoring INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${monitoring_inc_dir}")
get_target_property(infologger_inc_dir AliceO2::InfoLogger INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${infologger_inc_dir}")
get_target_property(o2_inc_dir AliceO2::AliceO2 INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${o2_inc_dir}")
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

ROOT_GENERATE_DICTIONARY("${dict}" ${HEADERS}
  LINKDEF include/QualityControl/LinkDef.h)

# TODO review how and what to install for dictionary
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME_CORE}Dict_rdict.pcm
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIBRARY_NAME_CORE}Dict.rootmap
  DESTINATION lib)

####################################
# Library
####################################

# Create library
add_library(QualityControl SHARED ${SRCS} QualityControlDict.cxx)

target_include_directories(QualityControl
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link targets
target_link_libraries(QualityControl
  PUBLIC
    Boost::boost
    FairLogger::FairLogger
    DataSampling::DataSampling
    FairMQ::FairMQ
    ROOT::Hist
    AliceO2::Common
    AliceO2::InfoLogger
    Monitoring::Monitoring
    Configuration::Configuration
    ROOT::Net
    AliceO2::AliceO2
    Boost::container
  PRIVATE
    Boost::system
    MySQL::MySQL
    ROOT::Gui
    ROOT::RMySQL
    CURL::CURL
)

####################################
# Executables
####################################

set(EXE_SRCS
  src/TaskDevice.cxx
  src/qcCheckerLauncher.cxx
  src/qcSpy.cxx
  src/runInformationService.cxx
  src/runInformationServiceDump.cxx
  src/runTaskDPL.cxx
  src/runReadoutChainTemplate.cxx
  src/runMergerTest.cxx
  src/runReadoutDataSampling.cxx
  src/alfaTestReceiver.cxx
  src/Consumer.cxx
  src/runRepositoryBenchmark.cxx
)

set(EXE_NAMES
  qcTaskLauncher
  qcCheckerLauncher
  qcSpy
  qcInfoService
  qcInfoServiceDump
  taskDPL
  runReadoutChainTemplate
  runMergerTest
  runReadoutDataSampling
  alfaTestReceiver
  qcConsumer
  repositoryBenchmark
)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name}
    PRIVATE
    QualityControl
  )
endforeach()

# tobject2json
add_executable(tobject2json ${SRCS_TOBJECT2JSON})
target_link_libraries(tobject2json
  PRIVATE
    QualityControl
    ZeroMQ::ZeroMQ
    MySQL::MySQL
)

####################################
# Gui
####################################

if (GLFW_FOUND)
  set(GUI_SRCS
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_impl_glfw_gl3.cpp
    src/imgui/gl3w.c
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_demo.cpp
    src/imgui/BaseGui.cxx
    src/DataDump.cxx
    )

  add_executable(dataDump ${GUI_SRCS})

  target_link_libraries(dataDump
    PRIVATE
      QualityControl
      GLFW::GLFW
  )
else ()
  message(STATUS "GLFW not found, DataDump will not be built")
endif ()

####################################
# Tests
####################################

set(TEST_SRCS
        test/testDbFactory.cxx
        test/testMonitorObject.cxx
        test/testPublisher.cxx
        test/testQcInfoLogger.cxx
        test/testQCTask.cxx
        test/testQuality.cxx
        )

foreach (test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
    PRIVATE
      QualityControl
      Boost::unit_test_framework
      MySQL::MySQL
  )
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()

####################################
# Install
####################################

include(GNUInstallDirs)

# Build targets with install rpath on Mac to dramatically speed up installation
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
  endif()
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# Install library
install(TARGETS QualityControl
  EXPORT QualityControlTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

# Create version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
  )

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/QualityControl DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Export targets
install(EXPORT QualityControlTargets
  FILE
    QualityControlTargets.cmake
  NAMESPACE
    QualityControl::
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl
  )

# Configure and install Config files
configure_package_config_file(
  ../cmake/QualityControlConfig.cmake.in ../cmake/QualityControlConfig.cmake
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl"
  PATH_VARS CMAKE_INSTALL_PREFIX
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/QualityControlConfigVersion.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/QualityControl
)

####################################
# Extra scripts
####################################

install(PROGRAMS script/qcDatabaseSetup.sh DESTINATION bin)
install(FILES example-default.ini example-default.json alfa.json dataDump.json DESTINATION etc)
Install(FILES qcTaskDplConfig.ini qcTaskDplConfig.json readoutChainTemplate.json readoutDataSampling.json
        DESTINATION etc)
