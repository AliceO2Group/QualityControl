# ---- Library ----

add_library(QcMFT)

target_sources(QcMFT PRIVATE src/BasicDigitQcCheck.cxx  src/BasicDigitQcTask.cxx )

target_include_directories(
  QcMFT
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcMFT PUBLIC QualityControl O2::DataFormatsITSMFT) 

install(TARGETS QcMFT
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(QcMFT
                    HEADERS
  include/MFT/BasicDigitQcCheck.h
  include/MFT/BasicDigitQcTask.h
                    LINKDEF include/MFT/LinkDef.h
                    BASENAME QcMFT)
                    
# ---- Executables ----

#set(EXE_SRCS src/runDigitsRootFileReaderMFT.cxx)
#set(EXE_NAMES o2-qc-run-digits-root-file-reader-mft)

add_executable(o2-qc-run-digits-root-file-reader-mft src/runDigitsRootFileReaderMFT.cxx)
target_link_libraries(o2-qc-run-digits-root-file-reader-mft PUBLIC QualityControl QcMFT O2::ITSMFTBase)
install(
  TARGETS o2-qc-run-digits-root-file-reader-mft
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)   

# ---- Test(s) ----

set(TEST_SRCS test/testQcMFT.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcMFT Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

# ---- Install config files and scripts ----

install(FILES basic-digit-qc-task.json
        DESTINATION etc)