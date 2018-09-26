#------------------------------------------------------------------------------
# CHECK_VARIABLE
macro(CHECK_VARIABLE VARIABLE_NAME ERROR_MESSAGE)
  if (NOT ${VARIABLE_NAME})
    message(FATAL_ERROR "${ERROR_MESSAGE}")
  endif (NOT ${VARIABLE_NAME})
endmacro(CHECK_VARIABLE)

#------------------------------------------------------------------------------
# GENERATE_ROOT_DICT
# Generate ROOT dictionary.
# The bulk of the code is an ugly mess that will redeclare the dependencies
# because we need to set INCLUDE_DIRECTORIES.
# arg MODULE_NAME - Module name
# arg LINKDEF - Path to the linkdef
# arg DICT_CLASS - name of the class that is generated without extension
function(GENERATE_ROOT_DICT)
  cmake_parse_arguments(
    PARSED_ARGS
    "" # bool args
    "MODULE_NAME;LINKDEF;DICT_CLASS" # mono-valued arguments
    "" # multi-valued arguments
    ${ARGN} # arguments
  )
  CHECK_VARIABLE(PARSED_ARGS_MODULE_NAME "You must provide a module name")
  CHECK_VARIABLE(PARSED_ARGS_LINKDEF "You must provide a path to a linkdef")
  CHECK_VARIABLE(PARSED_ARGS_DICT_CLASS "You must provide a name for the generated class")
  # ROOT dictionary
  # the following root macros expect include dirs to be set as directory property
  # TODO how to generate this automatically ? ? ?
    # https://gitlab.kitware.com/cmake/cmake/issues/12435
    # Maybe using this property ?
    #  get_target_property(asdf ${MODULE_NAME} INCLUDE_DIRECTORIES)
    #  message("test : ${asdf}")
  get_directory_property(include_dirs INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/include") # this module
  list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/include") # the framework
  get_target_property(qc_inc_dir QualityControl INTERFACE_INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${qc_inc_dir}")
  get_target_property(o2_inc_dir AliceO2::AliceO2 INTERFACE_INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${o2_inc_dir}")
  get_target_property(config_inc_dir Configuration::Configuration INTERFACE_INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${config_inc_dir}")
  list(APPEND include_dirs "${FAIRROOT_INCLUDE_DIR}")
  list(APPEND include_dirs "${FairMQ_INCDIR}")
  list(APPEND include_dirs "${FairMQ_INCDIR}/fairmq")
  get_target_property(fairlogger_inc_dir FairLogger::FairLogger INTERFACE_INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${fairlogger_inc_dir}")
  get_target_property(arrow_inc_dir Arrow::Arrow INTERFACE_INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${arrow_inc_dir}")
  get_target_property(common_inc_dir AliceO2::Common INTERFACE_INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${common_inc_dir}")
  get_target_property(boost_inc_dir Boost::container INTERFACE_INCLUDE_DIRECTORIES)
  list(APPEND include_dirs "${boost_inc_dir}")
  list(REMOVE_DUPLICATES include_dirs)
  include_directories(${include_dirs})

  set(dict_src ${CMAKE_CURRENT_BINARY_DIR}/${PARSED_ARGS_DICT_CLASS}.cxx)
  set_source_files_properties(${dict_src} PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
  set_source_files_properties(${dict_src} PROPERTIES GENERATED TRUE)

  ROOT_GENERATE_DICTIONARY("${PARSED_ARGS_DICT_CLASS}" ${HEADERS} LINKDEF ${PARSED_ARGS_LINKDEF})

  # TODO review how and what to install for dictionary
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${PARSED_ARGS_MODULE_NAME}Dict_rdict.pcm
    ${CMAKE_CURRENT_BINARY_DIR}/lib${PARSED_ARGS_MODULE_NAME}Dict.rootmap
    DESTINATION lib)

endfunction()

