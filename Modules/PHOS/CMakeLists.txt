# ---- Library ---- 

add_library(O2QcPHOS)

target_sources(O2QcPHOS PRIVATE src/RawQcTask.cxx 
                                src/RawCheck.cxx 
                                src/ClusterQcTask.cxx 
                                src/ClusterCheck.cxx
                                src/CalibQcTask.cxx)

target_include_directories(
  O2QcPHOS
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(O2QcPHOS PUBLIC O2QualityControl O2::PHOSBase O2::PHOSReconstruction)

add_root_dictionary(O2QcPHOS
                    HEADERS include/PHOS/ClusterQcTask.h
		            include/PHOS/ClusterCheck.h
                            include/PHOS/RawQcTask.h
                            include/PHOS/RawCheck.h
                            include/PHOS/CalibQcTask.h
                    LINKDEF include/PHOS/LinkDef.h
                    BASENAME O2QcPHOS)

install(TARGETS O2QcPHOS
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install (
  DIRECTORY etc DESTINATION Modules/PHOS
)

# ---- Executables ----

add_executable(o2-qc-run-phos-raw src/runQCPHOSRaw.cxx)
target_link_libraries(o2-qc-run-phos-raw PUBLIC O2QualityControl O2QcPHOS O2::DataFormatsPHOS O2::PHOSBase O2::PHOSWorkflow)
install(
  TARGETS o2-qc-run-phos-raw
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Tests ----
#
#set(
#  TEST_SRCS
#)
#
#foreach(test ${TEST_SRCS})
#  get_filename_component(test_name ${test} NAME)
#  string(REGEX REPLACE ".cxx" "" test_name ${test_name})
#
#  add_executable(${test_name} ${test})
#  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
#  add_test(NAME ${test_name} COMMAND ${test_name})
#  set_property(TARGET ${test_name}
#    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
#  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
#endforeach()

# ---- Install config files ----

install(FILES etc/phosCalibLED.json
              etc/phosCalibPed.json
              etc/phosClusters.json
              etc/phosPedestal.json
              etc/phosRawClu.json
              etc/phosRaw.json
        DESTINATION etc)
