# ---- Library ----

add_library(O2QcMFT)

target_sources(O2QcMFT PRIVATE
               src/QcMFTDigitCheck.cxx
               src/QcMFTDigitTask.cxx
               src/QcMFTClusterCheck.cxx
               src/QcMFTClusterTask.cxx
               src/QcMFTTrackCheck.cxx
               src/QcMFTTrackTask.cxx
               src/QcMFTAsyncTask.cxx
               src/QcMFTReadoutCheck.cxx
               src/QcMFTReadoutTask.cxx)

target_include_directories(
  O2QcMFT
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcMFT PUBLIC O2QualityControl O2::DataFormatsITSMFT O2::ITSMFTReconstruction)

install(TARGETS O2QcMFT
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcMFT
                    HEADERS
                    include/MFT/QcMFTDigitCheck.h
                    include/MFT/QcMFTDigitTask.h
                    include/MFT/QcMFTClusterCheck.h
                    include/MFT/QcMFTClusterTask.h
                    include/MFT/QcMFTTrackCheck.h
                    include/MFT/QcMFTTrackTask.h 
                    include/MFT/QcMFTAsyncTask.h
                    include/MFT/QcMFTReadoutCheck.h
                    include/MFT/QcMFTReadoutTask.h
                    include/MFT/QcMFTUtilTables.h
                    LINKDEF include/MFT/LinkDef.h
                    BASENAME O2QcMFT)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/MFT
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")
                    
# ---- Executables ----

set(EXE_SRCS 
     src/runMFTDigitsRootFileReader.cxx 
     src/runMFTDigitsHotPixelRootFileReader.cxx 
     src/runMFTClustersRootFileReader.cxx
     src/runMFTTracksRootFileReader.cxx)
set(EXE_NAMES 
     o2-qc-mft-digits-root-file-reader
     o2-qc-mft-digits-hot-pixel-root-file-reader
     o2-qc-mft-clusters-root-file-reader
     o2-qc-mft-tracks-root-file-reader)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE O2QualityControl O2QcMFT O2::ITSMFTBase)
endforeach()

install(
  TARGETS o2-qc-mft-digits-root-file-reader
          o2-qc-mft-digits-hot-pixel-root-file-reader
          o2-qc-mft-clusters-root-file-reader
          o2-qc-mft-tracks-root-file-reader
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

install(FILES qc-mft-digit.json
              qc-mft-cluster.json
              qc-mft-track.json
              qc-mft-readout.json
              qc-mft-async.json
              qc-mft-async_direct.json
        DESTINATION etc)
