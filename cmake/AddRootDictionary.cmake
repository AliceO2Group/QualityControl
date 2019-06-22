include_guard()

#
# add_root_dictionary generates one dictionary to be added to a target.
#
# Besides the dictionary source itself two files are also generated : a rootmap
# file and a pcm file. Those two will be installed alongside the target's
# library file
#
# arguments :
#
# * 1st parameter (required) is the target the dictionary should be added to
#
# * HEADERS (required) is a list of relative filepaths needed for the dictionary
#   definition
#
# * LINKDEF (required) is a single relative filepath to the LINKDEF file needed
#   by rootcling.
#
# * BASENAME (required) the basename used to compute the names of the extra
#   generated files (rootmap and pcm file)
#
# LINKDEF and HEADERS must contain relative paths only (relative to the
# CMakeLists.txt that calls this add_root_dictionary function).
#
# The target must be of course defined _before_ calling this function (i.e.
# add_library(target ...) has been called).
#
# In addition :
#
# * target_include_directories _must_ have be called as well, in order to be
#   able to compute the list of include directories needed to _compile_ the
#   dictionary
#
# Note also that the generated dictionary is added to PRIVATE SOURCES list of
# the target.
#
function(add_root_dictionary)
  cmake_parse_arguments(PARSE_ARGV
                        1
                        A
                        ""
                        "LINKDEF;BASENAME"
                        "HEADERS")
  if(A_UNPARSED_ARGUMENTS)
    message(
      FATAL_ERROR "Unexpected unparsed arguments: ${A_UNPARSED_ARGUMENTS}")
  endif()

  if(${ARGC} LESS 4)
    message(
      FATAL_ERROR "Wrong number of arguments. All arguments are required.")
  endif()

  set(target ${ARGV0})

  # check all given filepaths are relative ones
  foreach(h ${A_HEADERS} ${A_LINKDEF})
    if(IS_ABSOLUTE ${h})
      message(
        FATAL_ERROR "add_root_dictionary only accepts relative paths, but the"
                    "following path is absolute : ${h}")
    endif()
  endforeach()

  # convert all relative paths to absolute ones. LINKDEF must be the last one.
  foreach(h ${A_HEADERS} ${A_LINKDEF})
    get_filename_component(habs ${CMAKE_CURRENT_LIST_DIR}/${h} ABSOLUTE)
    list(APPEND headers ${habs})
  endforeach()

  # check all given filepaths actually exist
  foreach(h ${headers})
    get_filename_component(f ${h} ABSOLUTE)
    if(NOT EXISTS ${f})
      message(
        FATAL_ERROR
          "add_root_dictionary was given an inexistant input include ${f}")
    endif()
  endforeach()

  set(dictionaryFile ${CMAKE_CURRENT_BINARY_DIR}/G__${A_BASENAME}Dict.cxx)
  set(pcmFile G__${A_BASENAME}Dict_rdict.pcm)

  # get the list of compile_definitions and split it into -Dxxx pieces but only
  # if non empty
  set(prop "$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>")
  set(defs $<$<BOOL:${prop}>:-D$<JOIN:${prop}, -D>>)

  # Build the LD_LIBRARY_PATH required to get rootcling running fine
  #
  # Need at least root core library
  get_filename_component(LD_LIBRARY_PATH ${ROOT_Core_LIBRARY} DIRECTORY)
  # and possibly toolchain libs if we are using a toolchain
  if(DEFINED ENV{GCC_TOOLCHAIN_ROOT})
    set(LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:$ENV{GCC_TOOLCHAIN_ROOT}/lib")
    set(LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:$ENV{GCC_TOOLCHAIN_ROOT}/lib64")
  endif()

  if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  # add a custom command to generate the dictionary using rootcling
  # cmake-format: off
  add_custom_command(
    OUTPUT ${dictionaryFile}
    VERBATIM
    COMMAND
    ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${LD_LIBRARY_PATH} ${ROOT_rootcling_CMD}
      -f
      ${dictionaryFile}
      -inlineInputHeader
      -rmf ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${A_BASENAME}.rootmap
      -rml $<TARGET_FILE:${target}>
      $<GENEX_EVAL:-I$<JOIN:$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,\;-I>>
      # the generator expression above gets the list of all include 
      # directories that might be required using the transitive dependencies 
      # of the target ${target} and prepend each item of that list with -I 
      "${defs}"
      ${incdirs} ${headers}
    COMMAND
    ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${pcmFile} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${pcmFile}
    COMMAND_EXPAND_LISTS
    DEPENDS ${headers})
  # cmake-format: on

  # add dictionary source to the target sources
  target_sources(${target} PRIVATE ${dictionaryFile})

  get_property(libs TARGET ${target} PROPERTY INTERFACE_LINK_LIBRARIES)
  if(NOT ROOT::RIO IN_LIST libs)
    # add ROOT::IO if not already there as a target that has a Root dictionary
    # has to depend on ... Root
    target_link_libraries(${target} PUBLIC ROOT::RIO)
  endif()

  # Get the list of include directories that will be required to compile the
  # dictionary itself and add them as private include directories
  foreach(h IN LISTS A_HEADERS)
    if(IS_ABSOLUTE ${h})
      message(FATAL_ERROR "Path ${h} should be relative, not absolute")
    endif()
    get_filename_component(a ${h} ABSOLUTE)
    string(REPLACE "${h}" "" d "${a}")
    list(APPEND dirs ${d})
  endforeach()
  list(REMOVE_DUPLICATES dirs)
  target_include_directories(${target} PRIVATE ${dirs})

  # will install the rootmap and pcm files alongside the target's lib
  get_filename_component(dict ${dictionaryFile} NAME_WE)
  install(FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/lib${A_BASENAME}.rootmap
                ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${dict}_rdict.pcm
          DESTINATION ${CMAKE_INSTALL_LIBDIR})

endfunction()
