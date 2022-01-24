# ---- Library ----

add_library(O2QcTOF)

target_sources(O2QcTOF
                      # Tasks
              PRIVATE src/TaskDigits.cxx
                      src/TaskCosmics.cxx
                      src/TaskRaw.cxx
                      src/TOFMatchedTracks.cxx
                      # Checkers
                      src/CheckDiagnostics.cxx
                      src/CheckDRMDiagnostics.cxx
                      src/CheckCompressedData.cxx
                      src/CheckRawMultiplicity.cxx
                      src/CheckRawTime.cxx
                      src/CheckRawToT.cxx
                      src/CheckHitMap.cxx
                      # PostProcessing
                      src/PostProcessDiagnosticPerCrate.cxx
                      # Trending
                      src/TrendingHits.cxx
                      # Trending config
                      src/TrendingConfigTOF.cxx
                      # Utilities
                )

target_include_directories(
  O2QcTOF
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
         )

target_link_libraries(O2QcTOF PUBLIC O2QualityControl
                                    O2::TOFBase
                                    O2::DataFormatsTOF
                                    O2::TOFCompression
                                    O2::TOFReconstruction
                                    O2::DataFormatsGlobalTracking)

install(TARGETS O2QcTOF
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- ROOT dictionary ----

add_root_dictionary(O2QcTOF
                            # Tasks
                    HEADERS include/TOF/TaskDigits.h
                            include/TOF/TaskCosmics.h
                            include/TOF/TaskRaw.h
                            include/TOF/TOFMatchedTracks.h
                            # Checkers
                            include/TOF/CheckCompressedData.h
                            include/TOF/CheckDiagnostics.h
                            include/TOF/CheckDRMDiagnostics.h
                            include/TOF/CheckRawMultiplicity.h
                            include/TOF/CheckRawTime.h
                            include/TOF/CheckRawToT.h
                            include/TOF/CheckHitMap.h
                            # PostProcessing
                            include/TOF/PostProcessDiagnosticPerCrate.h
                            # Trending
                            include/TOF/TrendingHits.h
                            # Trending config
                            include/TOF/TrendingConfigTOF.h
                            # Utilities
                            include/Base/Counter.h
                            include/Base/MessagePad.h
                    LINKDEF include/TOF/LinkDef.h
                    BASENAME O2QcTOF)

# ---- Test(s) ----

add_executable(testQcTOF test/testTOF.cxx)
target_link_libraries(testQcTOF PRIVATE O2QcTOF Boost::unit_test_framework)
add_test(NAME testQcTOF COMMAND testQcTOF)
set_property(TARGET ${test_name}
  PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set_tests_properties(testQcTOF PROPERTIES TIMEOUT 20)

# ---- Executables ----

#set(EXE_SRCS src/runTOF.cxx)
#
#set(EXE_NAMES o2-qc-run-tof)
#
#list(LENGTH EXE_SRCS count)
#math(EXPR count "${count}-1")
#foreach(i RANGE ${count})
#  list(GET EXE_SRCS ${i} src)
#  list(GET EXE_NAMES ${i} name)
#  add_executable(${name} ${src})
#  target_link_libraries(${name} PRIVATE O2QualityControl CURL::libcurl)
#endforeach()

# ---- Extra scripts ----

install(FILES tofdigits.json
              tofdigits_multinode.json
              toffull.json
              toffull_multinode.json
              tofpostprocessdiagnosticpercrate.json
              tofraw.json
              tofraw_multinode.json
              tofcosmics.json
              toftrending.json
              tofMatchedTracks_ITSTPCTOF.json
              tofMatchedTracks_ITSTPCTOF_TPCTOF.json
              tofMatchedTracks_ITSTPCTOF_TPCTOF_MC.json
              tofMatchedTracks_ITSTPCTOF_TPCTOF_direct.json
              tofMatchedTracks_ITSTPCTOF_TPCTOF_direct_MC.json
        DESTINATION etc)

get_property(dirs
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
