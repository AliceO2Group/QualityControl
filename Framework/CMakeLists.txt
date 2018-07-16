####################################
# General project definition
####################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) # project specific cmake dir
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

project(QualityControl)

# Load some basic macros which are needed later on
include(O2Utils)
include(QualityControlDependencies)

message(STATUS "ROOT ${ROOT_VERSION} found in '${ROOTSYS}'")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ROOTSYS}/share/root/cmake/modules)
include(RootNewMacros)

# Set the default build type to "RelWithDebInfo"
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo"
            CACHE
            STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
            FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Set the version number of your project here (format is MAJOR.MINOR.PATCHLEVEL - e.g. 1.0.0)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# C++14
IF (CMAKE_VERSION VERSION_LESS 3.1)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-std=c++14 COMPILER_SUPPORTS_CXX14)
    if (COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else ()
        message(ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
    endif ()
ELSE ()
    set(CMAKE_CXX_STANDARD 14)  # proper way in CMake >= 3.1
ENDIF ()

# Add compiler flags for warnings and (more importantly) fPIC and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -fPIC")

####################################
# Module, library and executable definition
####################################

set(MODULE_NAME "QualityControl")

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
        src/TaskInterface.cxx
        src/MonitorObject.cxx
        src/Quality.cxx
        src/ObjectsManager.cxx
        src/Checker.cxx
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
        )

set(HEADERS # needed for the dictionary generation
        include/QualityControl/MonitorObject.h
        include/QualityControl/Quality.h
        include/QualityControl/CheckInterface.h
        include/QualityControl/SpyMainFrame.h
        include/QualityControl/DatabaseInterface.h
        include/QualityControl/CcdbDatabase.h
        include/QualityControl/TaskDataProcessor.h
        include/QualityControl/TaskDataProcessorFactory.h
        )

set(SRCS_TOBJECT2JSON
        src/TObject2Json.cxx
        src/TObject2JsonServer.cxx
        src/TObject2JsonWorker.cxx
        src/TObject2JsonMySql.cxx
        src/TObject2JsonBackendFactory.cxx
        src/TObject2JsonCcdb.cxx
        )

if(MYSQL_FOUND)
    list(APPEND SRCS src/MySqlDatabase.cxx)
endif()

# Produce the final Version.h using template Version.h.in and substituting variables.
# We don't want to polute our source tree with it, thus putting it in binary tree.
configure_file("include/${MODULE_NAME}/Version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/${MODULE_NAME}/Version.h"
        @ONLY
        )

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(LIBRARY_NAME QualityControl)

if(FAIRROOT_FOUND)
    set(BUCKET_NAME o2_qualitycontrol_with_fairroot_bucket)
else()
    set(BUCKET_NAME o2_qualitycontrol_bucket)
endif()

set(LINKDEF include/QualityControl/LinkDef.h)

O2_GENERATE_LIBRARY()

# Task Launcher
O2_GENERATE_EXECUTABLE(
        EXE_NAME qcTaskLauncher
        SOURCES src/TaskDevice.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME qcCheckerLauncher
        SOURCES src/qcCheckerLauncher.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME qcSpy
        SOURCES src/qcSpy.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME qcInfoService
        SOURCES src/runInformationService.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME qcInfoServiceDump
        SOURCES src/runInformationServiceDump.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME taskDPL
        SOURCES src/runTaskDPL.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME runReadoutChainTemplate
        SOURCES src/runReadoutChainTemplate.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME alfaTestReceiver
        SOURCES src/alfaTestReceiver.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME qcConsumer
        SOURCES src/Consumer.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

O2_GENERATE_EXECUTABLE(
        EXE_NAME repositoryBenchmark
        SOURCES src/runRepositoryBenchmark.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
)

set(TEST_SRCS
        test/testDbFactory.cxx
        test/testMonitorObject.cxx
        test/testPublisher.cxx
        test/testQcInfoLogger.cxx
        test/testQCTask.cxx
        test/testQuality.cxx
        )

O2_GENERATE_TESTS(
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
        TEST_SRCS ${TEST_SRCS}
)

O2_GENERATE_EXECUTABLE(
    EXE_NAME "tobject2json"
    SOURCES ${SRCS_TOBJECT2JSON}
    MODULE_LIBRARY_NAME ${LIBRARY_NAME}
    BUCKET_NAME "o2_qc_tobject2json"
)

# Install extra scripts
install(PROGRAMS script/qcDatabaseSetup.sh DESTINATION bin)
install(FILES example-default.ini example-default.json alfa.json DESTINATION etc)
Install(FILES qcTaskDplConfig.ini qcTaskDplConfig.json readoutChainTemplate.json DESTINATION etc/)
