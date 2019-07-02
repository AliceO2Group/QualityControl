#
# FIXME: this file should disappear as soon as the O2 cmake migration has been
# completed
#

function(define_o2_target target inc)
  find_path(O2_${target}_INCLUDE_PATH ${inc} ${O2_ROOT}/include/${target})
  find_library(O2_${target}_LIBRARY O2${target} ${O2_ROOT})
  if(O2_${target}_INCLUDE_PATH AND O2_${target}_LIBRARY)
    add_library(O2::${target} IMPORTED INTERFACE)
    target_include_directories(O2::${target} INTERFACE ${O2_ROOT}/include)
    target_link_libraries(O2::${target} INTERFACE ${O2_${target}_LIBRARY})
  else()
    message(FATAL_ERROR "Could not create target O2::${target}")
  endif()
endfunction()

# see first if we have a modern O2 available
find_package(O2 CONFIG QUIET)

if(O2_FOUND)
  if(NOT O2_FIND_QUIETLY)
    message(
      WARNING
        "Good news. We are using the modern cmake version of our O2 dependency.
        Which means this file (FindO2.cmake) could go away...")
  endif()
  if(NOT COMMAND add_root_dictionary)
    message(
      FATAL_ERROR "Bad news though: missing add_root_dictionary function !")
  endif()
  set(AliceO2_FOUND TRUE)
  return()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/AddRootDictionary.cmake")

if(NOT O2_FIND_QUIETLY)
  message(
    STATUS "Creating ourselves the O2 targets. That should be only temporary")
endif()

find_path(O2_FRAMEWORK_INCLUDE_PATH runDataProcessing.h
          ${O2_ROOT}/include/Framework)

find_library(O2_FRAMEWORK_LIBRARY O2Framework ${O2_ROOT})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(O2
                                  DEFAULT_MSG O2_FRAMEWORK_LIBRARY
                                              O2_FRAMEWORK_INCLUDE_PATH)

if(NOT O2_FOUND)
  return()
endif()

# create the minimal set of O2 target we need

if(NOT TARGET O2::CCDB)
  define_o2_target(CCDB CcdbApi.h)
endif()

if(NOT TARGET O2::Headers)
  define_o2_target(Headers DataHeader.h)
endif()

if(NOT TARGET O2::Framework)
  find_package(arrow CONFIG REQUIRED)
  define_o2_target(Framework runDataProcessing.h)
  target_link_libraries(O2::Framework INTERFACE arrow_shared O2::Headers)
endif()
