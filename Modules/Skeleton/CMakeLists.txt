set(MODULE_NAME "QcSkeleton")


# ---- Files ----

set(
  SRCS
  src/SkeletonTask.cxx
  src/SkeletonCheck.cxx
  /data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/src/HisAnalyzerSpec.cxx 
  /data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/base/src/GeometryTGeo.cxx
  /data/zhaozhong/alice/O2/Detectors/ITSMFT/common/reconstruction/src/DigitPixelReader.cxx 
)

set(
  HEADERS
  include/Skeleton/SkeletonTask.h
  /data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/include/ITSQCWorkflow/HisAnalyzerSpec.h
  include/Skeleton/SkeletonCheck.h
  /data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/base/include/ITSBase/GeometryTGeo.h
  /data/zhaozhong/alice/O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/DigitPixelReader.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS} ${MODULE_NAME}Dict.cxx)



include_directories(/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/include/ /data/zhaozhong/alice/O2/Detectors/ITSMFT/common/reconstruction/include /data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/base/include)

link_directories(/data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/)

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_library( libITSBase SHARED IMPORTED )
add_library( libITSSimulation SHARED IMPORTED )
add_library( libITSReconstruction SHARED IMPORTED )
add_library( libITSWorkflow SHARED IMPORTED )
add_library( libITSMFTReconstruction SHARED IMPORTED )
add_library( libITSMFTBase SHARED IMPORTED )

set_target_properties( libITSBase PROPERTIES IMPORTED_LOCATION /data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSBase.so )
set_target_properties( libITSSimulation PROPERTIES IMPORTED_LOCATION /data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSSimulation.so )
set_target_properties( libITSReconstruction PROPERTIES IMPORTED_LOCATION /data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSReconstruction.so )
set_target_properties( libITSWorkflow PROPERTIES IMPORTED_LOCATION /data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSWorkflow.so )
set_target_properties( libITSMFTReconstruction PROPERTIES IMPORTED_LOCATION /data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSMFTReconstruction.so )
set_target_properties( libITSMFTBase PROPERTIES IMPORTED_LOCATION /data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSMFTBase.so )

target_link_libraries(${MODULE_NAME} PUBLIC QualityControl libITSBase libITSSimulation libITSReconstruction libITSWorkflow libITSMFTReconstruction libITSMFTBase)
#target_link_libraries(${MODULE_NAME} PRIVATE libITSBase libITSSimulation)

#target_link_libraries(${MODULE_NAME} PUBLIC QualityControl)

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

generate_root_dict(MODULE_NAME ${MODULE_NAME} LINKDEF "include/Skeleton/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

# ---- Tests ----

set(TEST_SRCS test/testQcSkeleton.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
