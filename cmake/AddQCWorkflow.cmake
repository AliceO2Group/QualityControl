# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

include_guard()

#
# o2_add_qc_workflow(workflow-name configuration-file SOURCES ...) generates
# a configuration json file via o2-qc --config <configuration-file> --dump
# so that it can be easily registered and deployed e.g. in the train infrastructure.
#
# The installed JSON file will be named <workflow-name>.json and installed
# under share/dpl directory
#

#function(o2_add_qc_workflow workflowName configurationFile)
#
#  set(jsonDumpFile ${workflowName}.json)
#  set(qcExecutable o2-qc)
#
#  install(CODE "message(\"Generating a workflow dump with the command: ${CMAKE_INSTALL_PREFIX}/bin/${qcExecutable} -b --config json:/${CMAKE_INSTALL_PREFIX}/${configurationFile} --dump-workflow --dump-workflow-file ${CMAKE_CURRENT_BINARY_DIR}/${jsonDumpFile}\")")
#  install(CODE "execute_process(
#    COMMAND ${CMAKE_INSTALL_PREFIX}/bin/${qcExecutable} -b --config json:/${CMAKE_INSTALL_PREFIX}/${configurationFile} --dump-workflow --dump-workflow-file ${CMAKE_CURRENT_BINARY_DIR}/${jsonDumpFile}
#    )")
##  install(CODE "execute_process(
##    COMMAND echo 'QUALITYCONTROL_ROOT: ${QUALITYCONTROL_ROOT}'
##  )")
##  install(CODE "execute_process(
##    COMMAND pwd
##  )")
##  install(CODE "execute_process(
##    COMMAND which bin/o2-qc
##  )")
#  install(
#    FILES ${CMAKE_CURRENT_BINARY_DIR}/${jsonDumpFile}
#    DESTINATION ${CMAKE_INSTALL_DATADIR}/dpl)
#
#endfunction()

function(o2_add_qc_workflow)

  cmake_parse_arguments(
    PARSED_ARGS # prefix of output variables
    "" # list of names of the boolean arguments (only defined ones will be true)
    "WORKFLOW_NAME;CONFIG_FILE_PATH" # list of names of mono-valued arguments
    "" # list of names of multi-valued arguments (output variables are lists)
    ${ARGN} # arguments of the function to parse, here we take the all original ones
  )
  # note: if it remains unparsed arguments, here, they can be found in variable PARSED_ARGS_UNPARSED_ARGUMENTS
  if(NOT PARSED_ARGS_WORKFLOW_NAME OR NOT PARSED_ARGS_CONFIG_FILE_PATH)
    message(FATAL_ERROR "You must provide a workflow name and a config file path")
  endif()
  message(STATUS "o2_add_qc_workflow called with config file ${PARSED_ARGS_CONFIG_FILE_PATH} to generate ${PARSED_ARGS_WORKFLOW_NAME}")

  set(jsonDumpFile ${CMAKE_CURRENT_BINARY_DIR}/${PARSED_ARGS_WORKFLOW_NAME}.json)
  message(STATUS "jsonDumpFile: ${jsonDumpFile}" )
  set(qcExecutable o2-qc)

  add_custom_command(
    OUTPUT ${jsonDumpFile}
    VERBATIM
    COMMAND echo "${qcExecutable} -b --config json:/${CMAKE_INSTALL_PREFIX}/${PARSED_ARGS_CONFIG_FILE_PATH} --dump-workflow --dump-workflow-file ${jsonDumpFile}"
    COMMAND ${qcExecutable} -b --config json:/${CMAKE_INSTALL_PREFIX}/${PARSED_ARGS_CONFIG_FILE_PATH} --dump-workflow --dump-workflow-file ${jsonDumpFile}
    DEPENDS ${qcExecutable} ${CMAKE_INSTALL_PREFIX}/${PARSED_ARGS_CONFIG_FILE_PATH})
  # gmake[2]: *** No rule to make target `/home/pkonopka/alice/sw/slc7_x86-64/QualityControl/latest/etc/analysisDirect.json', needed by `Modules/Example/o2-qc-example-analysis-direct.json'.  Stop.

  get_filename_component(filename ${jsonDumpFile} NAME_WE)
  add_custom_target(${filename} ALL DEPENDS ${jsonDumpFile})

  # will install the rootmap and pcm files alongside the target's lib
  install(FILES ${jsonDumpFile}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dpl)

endfunction()