# ---- Library ---- 

add_library(QcPHOS)

target_sources(QcPHOS PRIVATE src/RawQcTask.cxx src/RawCheck.cxx src/ClusterQcTask.cxx src/ClusterCheck.cxx)

target_include_directories(
  QcPHOS
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QcPHOS PUBLIC QualityControl O2::PHOSBase O2::PHOSReconstruction)

add_root_dictionary(QcPHOS
                    HEADERS include/PHOS/ClusterQcTask.h
		            include/PHOS/ClusterCheck.h
                            include/PHOS/RawQcTask.h
                            include/PHOS/RawCheck.h
                    LINKDEF include/PHOS/LinkDef.h
                    BASENAME QcPHOS)

install(TARGETS QcPHOS
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install (
  DIRECTORY etc DESTINATION Modules/PHOS
)

# ---- Executables ----

add_executable(o2-qc-run-phos-raw src/runQCPHOSRaw.cxx)
target_link_libraries(o2-qc-run-phos-raw PUBLIC QualityControl QcPHOS O2::DataFormatsPHOS O2::PHOSBase O2::PHOSWorkflow)
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
