# - Try to find the O2 DataSampling package include dirs and libraries
# Author: Barthelemy von Haller
#
# This script will set the following variables:
#  DataSampling_FOUND - System has DataSampling
#  DataSampling_INCLUDE_DIRS - The DataSampling include directories
#  DataSampling_LIBRARIES - The libraries needed to use DataSampling
#  DataSampling_DEFINITIONS - Compiler switches required for using DataSampling
#
# This script can use the following variables:
#  DataSampling_ROOT - Installation root to tell this module where to look. (it tries LD_LIBRARY_PATH otherwise)

# Init
include(FindPackageHandleStandardArgs)

# find includes
find_path(DATASAMPLING_INCLUDE_DIR SamplerInterface.h
        HINTS ${DataSampling_ROOT}/include ENV LD_LIBRARY_PATH PATH_SUFFIXES "../include/DataSampling" "../../include/DataSampling" )
# Remove the final "DataSampling"
get_filename_component(DATASAMPLING_INCLUDE_DIR ${DATASAMPLING_INCLUDE_DIR} DIRECTORY)
set(DataSampling_INCLUDE_DIRS ${DATASAMPLING_INCLUDE_DIR})

# find library
find_library(DATASAMPLING_LIBRARY NAMES DataSampling HINTS ${DataSampling_ROOT}/lib ENV LD_LIBRARY_PATH)
set(DataSampling_LIBRARIES ${DATASAMPLING_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set DATASAMPLING_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(DataSampling  "DataSampling could not be found. Install package DataSampling or set DataSampling_ROOT to its root installation directory."
        DATASAMPLING_LIBRARY DATASAMPLING_INCLUDE_DIR)

if(${DataSampling_FOUND})
    message(STATUS "DataSampling found : ${DataSampling_LIBRARIES}")
endif()

mark_as_advanced(DATASAMPLING_INCLUDE_DIR DATASAMPLING_LIBRARY)
