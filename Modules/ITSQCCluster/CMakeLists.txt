set(MODULE_NAME "QcITSQCCluster")

# ---- Files ----

set(SRCS
  src/ITSQCCluster.cxx
)

set(HEADERS
  include/ITSQCCluster/ITSQCCluster.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS} ${MODULE_NAME}Dict.cxx)

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${O2_ROOT}/include/AliTPCCommon
)

add_library( libITSBase SHARED IMPORTED )
add_library( libITSSimulation SHARED IMPORTED )
add_library( libITSReconstruction SHARED IMPORTED )
add_library( libITSWorkflow SHARED IMPORTED )
add_library( libITSMFTReconstruction SHARED IMPORTED )
add_library( libITSMFTBase SHARED IMPORTED )
add_library( libDetectorsCommonDataFormats SHARED IMPORTED )
add_library( libJupyROOT SHARED IMPORTED )
add_library( libROOTDataFrame SHARED IMPORTED )
add_library( libROOTVecOps SHARED IMPORTED )
add_library( libPyROOT SHARED IMPORTED )



set_target_properties( libITSBase PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/O2/1.0.0-1/lib/libITSBase.so )
set_target_properties( libITSSimulation PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/O2/1.0.0-1/lib/libITSSimulation.so )
set_target_properties( libITSReconstruction PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/O2/1.0.0-1/lib/libITSReconstruction.so )
set_target_properties( libITSWorkflow PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/O2/1.0.0-1/lib/libITSWorkflow.so )
set_target_properties( libITSMFTReconstruction PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/O2/1.0.0-1/lib/libITSMFTReconstruction.so )
set_target_properties( libITSMFTBase PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/O2/1.0.0-1/lib/libITSMFTBase.so )
set_target_properties( libDetectorsCommonDataFormats PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/O2/1.0.0-1/lib/libDetectorsCommonDataFormats.so )
set_target_properties( libJupyROOT PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/ROOT/latest-o2/lib/libJupyROOT.so )
set_target_properties( libROOTDataFrame PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/ROOT/latest-o2/lib/libROOTDataFrame.so )
set_target_properties( libROOTVecOps PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/ROOT/latest-o2/lib/libROOTVecOps.so )
set_target_properties( libPyROOT PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../sw/slc7_x86-64/ROOT/latest-o2/lib/libPyROOT.so )




#target_link_libraries(${MODULE_NAME} PUBLIC QualityControl)
target_link_libraries(${MODULE_NAME} PUBLIC QualityControl libJupyROOT libROOTDataFrame libROOTVecOps libPyROOT libITSBase libITSSimulation libITSReconstruction libITSWorkflow libITSMFTReconstruction libITSMFTBase libDetectorsCommonDataFormats)

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

generate_root_dict(MODULE_NAME ${MODULE_NAME} LINKDEF "include/ITSQCCluster/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

# ---- Tests ----

set(
  TEST_SRCS
)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
