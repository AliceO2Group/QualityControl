# ---- Library ----

add_library(QcFT0)

target_sources(QcFT0 PRIVATE src/BasicDigitQcTask.cxx
                             src/ChannelsCheck.cxx
                             src/DigitsCheck.cxx
                             src/MergedTreeCheck.cxx
                             src/TreeReaderPostProcessing.cxx)

target_include_directories(
  QcFT0
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcFT0 PUBLIC QualityControl O2::DataFormatsFT0)


install(TARGETS QcFT0
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


add_root_dictionary(QcFT0
        HEADERS include/FT0/BasicDigitQcTask.h
                include/FT0/ChannelsCheck.h
                include/FT0/DigitsCheck.h
                include/FT0/Utilities.h
                include/FT0/MergedTreeCheck.h
                include/FT0/TreeReaderPostProcessing.h
        LINKDEF include/FT0/LinkDef.h
        BASENAME QcFT0)

# ---- Executables ----

set(EXE_SRCS 
     src/runDataProducer.cxx )
set(EXE_NAMES 
     o2-qc-ft0-data-producer)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QualityControl QcFT0 O2::DataFormatsFT0)
endforeach()

install(
  TARGETS o2-qc-ft0-data-producer
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


# ---- Test(s) ----

set(TEST_SRCS test/testQcFT0.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcFT0 Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

