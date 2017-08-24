# - Try to find the O2 InfoLogger package include dirs and libraries
# Author: Barthelemy von Haller
#
# This script will set the following variables:
#  InfoLogger_FOUND - System has InfoLogger
#  InfoLogger_INCLUDE_DIRS - The InfoLogger include directories
#  InfoLogger_LIBRARIES - The libraries needed to use InfoLogger
#  InfoLogger_DEFINITIONS - Compiler switches required for using InfoLogger
#
# This script can use the following variables:
#  InfoLogger_ROOT - Installation root to tell this module where to look. (it tries LD_LIBRARY_PATH otherwise)

# Init
include(FindPackageHandleStandardArgs)

# find includes
find_path(INFOLOGGER_INCLUDE_DIR InfoLogger.h
        HINTS ${InfoLogger_ROOT}/include ENV LD_LIBRARY_PATH PATH_SUFFIXES "../include/InfoLogger" "../../include/InfoLogger" )
# Remove the final "InfoLogger"
get_filename_component(INFOLOGGER_INCLUDE_DIR ${INFOLOGGER_INCLUDE_DIR} DIRECTORY)
set(InfoLogger_INCLUDE_DIRS ${INFOLOGGER_INCLUDE_DIR})

# find library
find_library(INFOLOGGER_LIBRARY NAMES InfoLogger HINTS ${InfoLogger_ROOT}/lib ENV LD_LIBRARY_PATH)
set(InfoLogger_LIBRARIES ${INFOLOGGER_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set INFOLOGGER_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(InfoLogger  "InfoLogger could not be found. Install package InfoLogger or set InfoLogger_ROOT to its root installation directory."
        INFOLOGGER_LIBRARY INFOLOGGER_INCLUDE_DIR)

if(${InfoLogger_FOUND})
    message(STATUS "InfoLogger found : ${InfoLogger_LIBRARIES}")
endif()

mark_as_advanced(INFOLOGGER_INCLUDE_DIR INFOLOGGER_LIBRARY)
