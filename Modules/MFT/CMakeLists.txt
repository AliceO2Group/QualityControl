# ---- Library ----

add_library(O2QcMFT)

target_sources(O2QcMFT PRIVATE
               src/BasicDigitQcCheck.cxx
               src/BasicDigitQcTask.cxx
               src/BasicClusterQcCheck.cxx
               src/BasicClusterQcTask.cxx
               src/BasicTrackQcCheck.cxx
               src/BasicTrackQcTask.cxx
               src/BasicReadoutHeaderQcCheck.cxx
               src/BasicReadoutHeaderQcTask.cxx)

target_include_directories(
  O2QcMFT
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcMFT PUBLIC O2QualityControl O2::DataFormatsITSMFT)

install(TARGETS O2QcMFT
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcMFT
                    HEADERS
                    include/MFT/BasicDigitQcCheck.h
                    include/MFT/BasicDigitQcTask.h
                    include/MFT/BasicClusterQcCheck.h
                    include/MFT/BasicClusterQcTask.h
                    include/MFT/BasicTrackQcCheck.h
                    include/MFT/BasicTrackQcTask.h 
                    include/MFT/BasicReadoutHeaderQcCheck.h
                    include/MFT/BasicReadoutHeaderQcTask.h
                    LINKDEF include/MFT/LinkDef.h
                    BASENAME O2QcMFT)
                    
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
  target_link_libraries(${name} PRIVATE O2QualityControl O2QcMFT O2::ITSMFTBase)
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
                        PRIVATE O2QcMFT Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

# ---- Install config files and scripts ----

install(FILES mft-digit-qc-task-FLP-0-TaskLevel-0.json
              mft-digit-qc-task-FLP-0-TaskLevel-1.json
              mft-digit-qc-task-FLP-0-TaskLevel-2.json
              mft-digit-qc-task-FLP-0-TaskLevel-3.json
              mft-digit-qc-task-FLP-0-TaskLevel-4.json
              mft-digit-qc-task-FLP-1-TaskLevel-0.json
              mft-digit-qc-task-FLP-1-TaskLevel-1.json
              mft-digit-qc-task-FLP-1-TaskLevel-2.json
              mft-digit-qc-task-FLP-1-TaskLevel-3.json
              mft-digit-qc-task-FLP-1-TaskLevel-4.json
              mft-digit-qc-task-FLP-2-TaskLevel-0.json
              mft-digit-qc-task-FLP-2-TaskLevel-1.json
              mft-digit-qc-task-FLP-2-TaskLevel-2.json
              mft-digit-qc-task-FLP-2-TaskLevel-3.json
              mft-digit-qc-task-FLP-2-TaskLevel-4.json
              mft-digit-qc-task-FLP-3-TaskLevel-0.json
              mft-digit-qc-task-FLP-3-TaskLevel-1.json
              mft-digit-qc-task-FLP-3-TaskLevel-2.json
              mft-digit-qc-task-FLP-3-TaskLevel-3.json
              mft-digit-qc-task-FLP-3-TaskLevel-4.json
              mft-digit-qc-task-FLP-4-TaskLevel-0.json
              mft-digit-qc-task-FLP-4-TaskLevel-1.json
              mft-digit-qc-task-FLP-4-TaskLevel-2.json
              mft-digit-qc-task-FLP-4-TaskLevel-3.json
              mft-digit-qc-task-FLP-4-TaskLevel-4.json
              basic-cluster-qc-task.json
              basic-track-qc-task.json
              mft-basic-readout-header-qc-task.json
        DESTINATION etc)