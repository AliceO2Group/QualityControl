# ---- Library ----

add_library(O2QcFT0)

target_sources(O2QcFT0 PRIVATE src/BasicDigitQcTask.cxx
                             src/DigitQcTask.cxx
                             src/ChannelsCheck.cxx
                             src/DigitsCheck.cxx
                             src/MergedTreeCheck.cxx
                             src/TreeReaderPostProcessing.cxx
                             src/CalibrationTask.cxx
                             src/ChannelTimeCalibrationCheck.cxx
                             src/BasicPPTask.cxx)

target_include_directories(
  O2QcFT0
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcFT0 PUBLIC O2QualityControl O2::DataFormatsFT0 O2::FT0Base O2::FITCalibration)

install(TARGETS O2QcFT0
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


add_root_dictionary(O2QcFT0
        HEADERS include/FT0/BasicDigitQcTask.h
                include/FT0/DigitQcTask.h
                include/FT0/ChannelsCheck.h
                include/FT0/DigitsCheck.h
                include/FT0/Utilities.h
                include/FT0/MergedTreeCheck.h
                include/FT0/TreeReaderPostProcessing.h
                include/FT0/CalibrationTask.h
                include/FT0/ChannelTimeCalibrationCheck.h
                include/FT0/BasicPPTask.h
        LINKDEF include/FT0/LinkDef.h
        BASENAME O2QcFT0)

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

set(TEST_SRCS test/testQcFT0.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcFT0 Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

# ----- Configs -------
install(FILES
        ft0-calibration-config.json
        ft0-multinode-config.json
        ft0-postprocessing-config.json
        ft0-basic-config.json
        DESTINATION etc)

get_property(dirs
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
