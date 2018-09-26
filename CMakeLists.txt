####################################
# CMake options
####################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)

# Set cmake policy by version: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.12)
endif()

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

####################################
# Project
####################################

project(QualityControl
  VERSION 0.6.2   # TODO update this automatically when there are new releases
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
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo"
      CACHE
      STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

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

# Add compiler flags for warnings and (more importantly) fPIC and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

# Set fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

####################################
# Subdirectories
####################################

add_subdirectory(Framework)
add_subdirectory(Modules)
#add_subdirectory(doc)
