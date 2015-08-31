# REFERENCE PROJECT CMAKELISTS.TXT
#
# @author  bvonhall

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake) # main (top) cmake dir
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) # project specific cmake dir

# Set the name of your project here
project("QualityControl")

# Set the version number of your project here (format is MAJOR.MINOR.PATCHLEVEL - e.g. 1.0.0)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# Set default installation
set(CMAKE_INSTALL_PREFIX "/opt/${CMAKE_PROJECT_NAME}")

# Git (and its revision)
find_package(Git)
# GetGitRevisionDescription module to retreive branch and revision information from Git
# Starting with Git 1.9 the module will be part of official cMake distribution, until then it has to be
# part of the application
# The Git module will trigger a reconfiguration for each pull that will bring a new revision on the local repository
set (VCS_REVISION "-1")
if(GIT_FOUND)
    include(GetGitRevisionDescription)
    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
    message(STATUS "GIT branch ${GIT_REFSPEC}")
    message(STATUS "GIT revision ${GIT_SHA1}")
    set (VCS_REVISION ${GIT_SHA1})
endif()

# C++11 needed
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-std=c++11 COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  message(ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Mac needed variables (adapt according to your needs)
set(CMAKE_MACOSX_RPATH ON)
#set(CMAKE_SKIP_BUILD_RPATH FALSE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Boost
set(Boost_NO_BOOST_CMAKE ON) #  Don't do a find_package in config mode before searching for a regular boost install.
# TODO Don't hard code this...
#set(Boost_INCLUDE_DIR /opt/boost_1_58_0)
#set(Boost_LIBRARY_DIR /opt/boost_1_58_0/stage/lib)
find_package(Boost 1.58 REQUIRED COMPONENTS unit_test_framework program_options)

# Alfa
find_package(FairRoot)
if(FAIRROOT_FOUND)
    # this should go away when fairrot provides a proper Find script or proper config scripts
    # See : http://www.cmake.org/cmake/help/v3.0/command/link_directories.html
    link_directories(${FAIRROOT_LIBRARY_DIR})
else(FAIRROOT_FOUND)
    message(WARNING "FairRoot not found, corresponding classes will not be compiled.")
endif(FAIRROOT_FOUND)

# ROOT
if(FAIRROOT_FOUND)
    SET(ROOT_DIR "/home/flpprotodev/dev/ALFA-Externals/lib/root/")
    set(ROOT_CONFIG_SEARCHPATH "/home/flpprotodev/dev/ALFA-Externals/lib/root/")
    find_package(ROOT)
    set(FAIRROOT_LIBRARIES Base FairMQ BaseMQ)
else(FAIRROOT_FOUND)
    #set(ROOT_CONFIG_DEBUG true)
    # this is an ugly hack to find root as it is not installed in a standard place
    SET(ROOT_DIR "/opt/root-6/cmake/")
    find_package(ROOT CONFIG COMPONENTS Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore QUIET)
    set(FAIRROOT_LIBRARIES "")
endif(FAIRROOT_FOUND)
# TODO make it required once in its own repo
if(ROOT_FOUND)
    message(STATUS "ROOT ${ROOT_VERSION} found")
else()
    message(WARNING "ROOT not found, we won't compile the QC (skip, no error)")
endif()

# O2 deps (we check whether the target exists and find the package only if we don't know it yet)
#         (it would be the case if we are compiling from the top dir)
if(NOT TARGET InfoLogger)
  find_package(InfoLogger CONFIG REQUIRED)
endif()
if(NOT TARGET Configuration)
  find_package(Configuration CONFIG REQUIRED)
endif()
if(NOT TARGET DataSampling)
  find_package(DataSampling CONFIG REQUIRED)
endif()

# This sets the include directory for the reference project. This is
# the -I flag in gcc. All the includes should be in this variable To
# add new directories to the include path, just add them in this list
# (after a space), or add an include_directories statement in a
# subdirectory's CMakeLists.
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/include   # add the binary tree to the search path so that we will find Version.h
    ${Boost_INCLUDE_DIRS}
    ${InfoLogger_INCLUDE_DIR}
    ${Configuration_INCLUDE_DIR}
    ${DataSampling_INCLUDE_DIR}
    ${ROOT_INCLUDE_DIRS}
)
if(FAIRROOT_FOUND)
    include_directories(
            ${FAIRROOT_INCLUDE_DIR}
    )
endif(FAIRROOT_FOUND)

if(ROOT_FOUND) # TODO remove this once we have our own repo
    enable_testing() # needed on top-level CMakeLists.txt
    add_subdirectory(include)
    add_subdirectory(src)
    add_subdirectory(doc)
    add_subdirectory(test)


    #
    # SYSTEM PACKAGING (RPM, TGZ, ...)
    # _____________________________________________________________________________
    
    include(CPackConfig)
    
    
    #
    # CMAKE PACKAGING (for other CMake projects to use this one easily)
    # _____________________________________________________________________________
    
    include(PackageConfigurator)
endif()