# ---- CMake options ----

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakeParseArguments)
include(GNUInstallDirs)
include(AddQCWorkflow)

# ---- Project ----

project(QualityControl
        VERSION 1.30.0 
        DESCRIPTION "O2 Data Quality Control Framework"
        LANGUAGES CXX)

if(ONLYDOC)
  add_subdirectory(doc)
  return()
endif()

# Set CMAKE_INSTALL_LIBDIR explicitly to lib (to avoid lib64 on CC7)
set(CMAKE_INSTALL_LIBDIR lib)

include(GNUInstallDirs)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()

# ---- End Project ----
# The line above is necessary for the generation of doxygen by travis

# ---- Compilation flags and build options ----

# Set the default build type to "RelWithDebInfo"
if(NOT CMAKE_BUILD_TYPE)
  set(
    CMAKE_BUILD_TYPE
    "RelWithDebInfo"
    CACHE
      STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

option(BUILD_SHARED_LIBS "Build shared libs" ON)

# Build targets with install rpath on Mac to dramatically speed up installation
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib"
          isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
  endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# C++ standard
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
endif()

# Add compiler flags for warnings and (more importantly) fPIC and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Werror")

# Set fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- Dependencies ----

find_package(Boost 1.58
             COMPONENTS container
                        unit_test_framework
                        program_options
                        system
                        log
                        system)
find_package(Git QUIET)
find_package(Configuration REQUIRED)
find_package(Monitoring REQUIRED)
find_package(Common REQUIRED)
find_package(InfoLogger REQUIRED)
find_package(O2 CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(GLFW NAMES glfw3 CONFIG)
find_package(FairMQ REQUIRED)
find_package(FairLogger REQUIRED)
find_package(Occ REQUIRED)
find_package(ROOT 6.06.02 COMPONENTS RHTTP Gui REQUIRED)

configure_file(getTestDataDirectory.cxx.in getTestDataDirectory.cxx)

# ---- Subdirectories ----

add_subdirectory(Framework)
add_subdirectory(Modules)
add_subdirectory(doc)
