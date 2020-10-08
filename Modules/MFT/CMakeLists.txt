# ---- Library ----

add_library(QcMFT)

target_sources(QcMFT PRIVATE 
               src/BasicDigitQcCheck.cxx
               src/BasicDigitQcTask.cxx
               src/BasicClusterQcCheck.cxx
               src/BasicClusterQcTask.cxx
               src/BasicTrackQcCheck.cxx
               src/BasicTrackQcTask.cxx)

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
                    include/MFT/BasicClusterQcCheck.h
                    include/MFT/BasicClusterQcTask.h
                    include/MFT/BasicTrackQcCheck.h
                    include/MFT/BasicTrackQcTask.h  
                    LINKDEF include/MFT/LinkDef.h
                    BASENAME QcMFT)
                    
# ---- Executables ----

set(EXE_SRCS 
     src/runDigitsRootFileReaderMFT.cxx 
     src/runDigitsHotPixelRootFileReaderMFT.cxx 
     src/runClustersRootFileReaderMFT.cxx
     src/runTracksRootFileReaderMFT.cxx)
set(EXE_NAMES 
     o2-qc-run-digits-root-file-reader-mft
     o2-qc-run-digits-hot-pixel-root-file-reader-mft
     o2-qc-run-clusters-root-file-reader-mft
     o2-qc-run-tracks-root-file-reader-mft)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QualityControl QcMFT O2::ITSMFTBase)
endforeach()

install(
  TARGETS o2-qc-run-digits-root-file-reader-mft
          o2-qc-run-digits-hot-pixel-root-file-reader-mft
          o2-qc-run-clusters-root-file-reader-mft
          o2-qc-run-tracks-root-file-reader-mft
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
              basic-cluster-qc-task.json
              basic-track-qc-task.json
        DESTINATION etc)