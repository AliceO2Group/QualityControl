
####################################
# Project
####################################

project(QualityControl
        #        VERSION 1.0   # TODO update this automatically when there are new releases
        DESCRIPTION "O2 Quality Control"
        LANGUAGES CXX
        )

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(INCLUDE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/include")

####################################
# Compilation flags and build options
####################################

# Set the default build type to "RelWithDebInfo"
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo"
            CACHE
            STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
            FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# Build targets with install rpath on Mac to dramatically speed up installation
# TODO reevaluate if this is needed
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if ("${isSystemDir}" STREQUAL "-1")
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
    endif ()
endif ()
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif ()
unset(isSystemDir)

# C++ standard
set(CMAKE_CXX_STANDARD 14)

# Add compiler flags for warnings and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

# Set fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TODO check how it was done in flpproto and reenable
# Add coverage flags to Debug build
#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
#endif()

####################################
# Dependencies
####################################

# Load some basic macros which are needed later on
#include(O2Utils)
#include(QualityControlDependencies)
#
#message(STATUS "ROOT ${ROOT_VERSION} found in '${ROOTSYS}'")
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ROOTSYS}/share/root/cmake/modules)
#include(RootNewMacros)

find_package(Boost 1.58 COMPONENTS container unit_test_framework program_options system log signals)
find_package(Git QUIET)
find_package(Configuration REQUIRED)
find_package(Monitoring REQUIRED)
find_package(MySQL REQUIRED)
find_package(Common REQUIRED)
find_package(InfoLogger REQUIRED)
find_package(DataSampling REQUIRED)
find_package(AliceO2 REQUIRED)
find_package(ROOT 6.06.02 COMPONENTS RHTTP RMySQL Gui REQUIRED)
find_package(CURL REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(nanomsg REQUIRED)
find_package(Arrow REQUIRED)
find_package(GLFW)
find_package(FairRoot REQUIRED)
message("AAAAABBBBB")
find_package(FairMQ REQUIRED)
find_package(FairLogger REQUIRED)
#include_directories(${MS_GSL_INCLUDE_DIR})

# TODO reevaluate this
if (NOT MYSQL_FOUND)
    message(WARNING "MySQL not found, the corresponding classes won't be built.")
else ()
    add_definitions(-D_WITH_MYSQL)
endif ()

# ROOT mysteries (https://root-forum.cern.ch/t/incostistency-in-building-library-with-cmake-for-root-5-6/19315/2)
#list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT 6.06.02 COMPONENTS RHTTP RMySQL Gui REQUIRED)
#include(RootNewMacros)

####################################
# Library
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

# ROOT dictionary
# the following root macros expect include dirs to be set as directory property
get_directory_property(include_dirs INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/include")
get_target_property(config_inc_dir Configuration::Configuration INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${config_inc_dir}")
get_target_property(arrow_inc_dir Arrow::Arrow INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${arrow_inc_dir}")
get_target_property(monitoring_inc_dir Monitoring::Monitoring INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${monitoring_inc_dir}")
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

# Create library
add_library(QualityControl SHARED ${SRCS} QualityControlDict.cxx )

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
        PRIVATE
        Boost::system
        )
#
## Task Launcher
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME qcTaskLauncher
#        SOURCES src/TaskDevice.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME qcCheckerLauncher
#        SOURCES src/qcCheckerLauncher.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME qcSpy
#        SOURCES src/qcSpy.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME qcInfoService
#        SOURCES src/runInformationService.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME qcInfoServiceDump
#        SOURCES src/runInformationServiceDump.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME taskDPL
#        SOURCES src/runTaskDPL.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME runReadoutChainTemplate
#        SOURCES src/runReadoutChainTemplate.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME runMergerTest
#        SOURCES src/runMergerTest.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME runReadoutDataSampling
#        SOURCES src/runReadoutDataSampling.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME alfaTestReceiver
#        SOURCES src/alfaTestReceiver.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME qcConsumer
#        SOURCES src/Consumer.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME repositoryBenchmark
#        SOURCES src/runRepositoryBenchmark.cxx
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#)
#
#set(TEST_SRCS
#        test/testDbFactory.cxx
#        test/testMonitorObject.cxx
#        test/testPublisher.cxx
#        test/testQcInfoLogger.cxx
#        test/testQCTask.cxx
#        test/testQuality.cxx
#        )
#
#O2_GENERATE_TESTS(
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME ${BUCKET_NAME}
#        TEST_SRCS ${TEST_SRCS}
#)
#
#O2_GENERATE_EXECUTABLE(
#        EXE_NAME "tobject2json"
#        SOURCES ${SRCS_TOBJECT2JSON}
#        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#        BUCKET_NAME "o2_qc_tobject2json"
#)
#
#if (GLFW_FOUND)
#    set(GUI_BACKEND
#            src/imgui/imgui.cpp
#            src/imgui/imgui_draw.cpp
#            src/imgui/imgui_impl_glfw_gl3.cpp
#            src/imgui/gl3w.c
#            src/imgui/imgui_widgets.cpp
#            src/imgui/imgui_demo.cpp
#            src/imgui/BaseGui.cxx
#            src/DataDump.cxx
#            )
#
#    O2_GENERATE_EXECUTABLE(
#            EXE_NAME dataDump
#            SOURCES src/runDataDump.cxx ${GUI_BACKEND}
#            MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#            BUCKET_NAME "dumpData_bucket"
#    )
#else ()
#    message(STATUS "GLFW not found, DataDump will not be built")
#endif ()
#
#
## Install extra scripts
#install(PROGRAMS script/qcDatabaseSetup.sh DESTINATION bin)
#install(FILES example-default.ini example-default.json alfa.json dataDump.json DESTINATION etc)
#Install(FILES qcTaskDplConfig.ini qcTaskDplConfig.json readoutChainTemplate.json readoutDataSampling.json
#        DESTINATION etc/)
