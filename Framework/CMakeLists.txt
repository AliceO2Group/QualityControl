####################################
# General project definition
####################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)

### CMP0025   Compiler id for Apple Clang is now AppleClang.
### CMP0042   MACOSX_RPATH is enabled by default.

FOREACH (p
        CMP0025 # CMake 3.0
        CMP0042 # CMake 3.0
        )
    IF (POLICY ${p})
        cmake_policy(SET ${p} NEW)
    ENDIF ()
endforeach ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) # project specific cmake dir
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

project(QualityControl)

# Load some basic macros which are needed later on
include(O2Utils)
include(QualityControlDependencies)
include(O2) # to be removed

# TODO make it required, once in its own repo
if(ROOT_FOUND)
    message(STATUS "ROOT ${ROOT_VERSION} found in '${ROOTSYS}'")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ROOTSYS}/share/root/cmake/modules)
    include(RootNewMacros)
else()
    message(WARNING "ROOT not found, we won't compile the QC (skip, no error)")
    return()
endif()

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
        src/TaskFactory.cxx
        src/Checker.cxx
        src/CheckInterface.cxx
        src/DatabaseFactory.cxx
        src/ClientDataProvider.cxx
        )

set(HEADERS # needed for the dictionary generation
        include/QualityControl/MonitorObject.h
        include/QualityControl/Quality.h
        include/QualityControl/CheckInterface.h
        include/QualityControl/SpyMainFrame.h
)

if (FAIRROOT_FOUND)
    list(APPEND SRCS
            src/AlfaReceiverForTests.cxx
            src/TaskDevice.cxx
            src/SpyDevice.cxx
            src/SpyMainFrame.cxx
            )
endif (FAIRROOT_FOUND)

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

if (FAIRROOT_FOUND)
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
endif()

if(ROOT_FOUND) # TODO remove this once we have our own repo
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

endif()

# Install extra scripts
install(PROGRAMS script/qcDatabaseSetup.sh DESTINATION bin)
install(FILES example-default.ini alfa.json DESTINATION etc)
