
# - Try to find the O2 framework package include dirs and libraries
# Author: Barthelemy von Haller
#
# This script will set the following variables:
#  AliceO2_FOUND - System has AliceO2
#  AliceO2_INCLUDE_DIRS - The AliceO2 include directories
#  AliceO2_LIBRARIES - The libraries needed to use AliceO2
#  AliceO2_DEFINITIONS - Compiler switches required for using AliceO2

# Init
include(FindPackageHandleStandardArgs)

# find includes
find_path(AliceO2_INCLUDE_DIR runDataProcessing.h
        HINTS ${O2_ROOT}/include ENV LD_LIBRARY_PATH PATH_SUFFIXES "../include/Framework" "../../include/Framework")

# Remove the final "Headers"
get_filename_component(AliceO2_INCLUDE_DIR ${AliceO2_INCLUDE_DIR} DIRECTORY)
set(AliceO2_INCLUDE_DIRS ${AliceO2_INCLUDE_DIR})
list(APPEND AliceO2_INCLUDE_DIRS ${MS_GSL_INCLUDE_DIR})

# find libraries
# TODO SEARCH *ALL* LIBRARIES --> AliceO2 should ideally provide the list !!!
set(O2_LIBRARIES_NAMES
        O2FrameworkFoundation
        O2Framework
        O2Headers
        O2CCDB
        O2DebugGUI
        O2DetectorsBase
        O2ITSBase
        O2ITSSimulation
        O2ITSReconstruction
        O2ITSWorkflow
        O2ITSMFTReconstruction
        O2ITSMFTBase
        O2DetectorsCommonDataFormats
        )
foreach(lib_name ${O2_LIBRARIES_NAMES})
    find_library(AliceO2_LIBRARY_${lib_name} NAMES ${lib_name} HINTS ${O2_ROOT}/lib ENV LD_LIBRARY_PATH)
    list(APPEND AliceO2_LIBRARIES_VAR_NAMES AliceO2_LIBRARY_${lib_name})
    list(APPEND AliceO2_LIBRARIES ${AliceO2_LIBRARY_${lib_name}})
endforeach()

# handle the QUIETLY and REQUIRED arguments and set AliceO2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(AliceO2  "AliceO2 could not be found. Install package AliceO2." ${AliceO2_LIBRARIES_VAR_NAMES} AliceO2_INCLUDE_DIR)

if(${AliceO2_FOUND})
    message(STATUS "AliceO2 found, libraries: ${AliceO2_LIBRARIES}")

    mark_as_advanced(AliceO2_INCLUDE_DIRS AliceO2_LIBRARIES)

    # add target
    if(NOT TARGET AliceO2::AliceO2)
        add_library(AliceO2::AliceO2 INTERFACE IMPORTED)
        set_target_properties(AliceO2::AliceO2 PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${AliceO2_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${AliceO2_LIBRARIES}"
                )
    endif()
endif()

