# ---- Library ----
set(MODULE_NAME "O2QcFIT")

add_library(${MODULE_NAME})

target_sources(${MODULE_NAME} PRIVATE src/RecoFITQcTask.cxx)

target_include_directories(
  O2QcFIT
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${MODULE_NAME} PUBLIC O2QualityControl
                                     O2::DataFormatsFDD
                                     O2::DataFormatsFT0
                                     O2::DataFormatsFV0
                                     O2::CommonDataFormat
                                     O2QcCommon
                                     O2QcFITCommon)

install(TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(${MODULE_NAME}
  HEADERS include/FIT/RecoFITQcTask.h
  LINKDEF include/FIT/LinkDef.h)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/FIT
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Executables ----
# keep commented as an example

#set(EXE_SRCS
#     src/runDataProducer.cxx )
#set(EXE_NAMES
#     o2-qc-ft0-data-producer)
#
#list(LENGTH EXE_SRCS count)
#math(EXPR count "${count}-1")
#foreach(i RANGE ${count})
#  list(GET EXE_SRCS ${i} src)
#  list(GET EXE_NAMES ${i} name)
#  add_executable(${name} ${src})
#  target_link_libraries(${name} PRIVATE O2QualityControl O2QcFT0 O2::DataFormatsFT0 O2::FITCalibration)
#endforeach()

#install(
#  TARGETS o2-qc-ft0-data-producer
#  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)


# ---- Test(s) ----

#set(TEST_SRCS test/testQcFT0.cxx) # uncomment to reenable the test which was empty

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcFIT Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

# ----- Configs -------
#install(FILES
#        ft0-reconstruction-config.json
#        DESTINATION etc)

get_property(dirs
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
