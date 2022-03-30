# ---- Library ----

add_library(O2QcPID)

target_sources(O2QcPID
                      # Tasks
              PRIVATE src/TaskFT0TOF.cxx
                      # PostProcessing
                      # Trending
                      # Trending config
                      # Utilities
                )

target_include_directories(
  O2QcPID
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
         )

target_link_libraries(O2QcPID PUBLIC O2QualityControl
                                    O2::TOFBase
                                    O2::DataFormatsTOF
                                    O2::TOFCompression
                                    O2::TOFReconstruction
                                    O2::DataFormatsGlobalTracking
                                    O2::DataFormatsFT0
                                    O2::FT0Base
                                    O2::FITCalibration)

install(TARGETS O2QcPID
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/PID
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- ROOT dictionary ----

add_root_dictionary(O2QcPID
                            # Tasks
                    HEADERS include/PID/TaskFT0TOF.h
                            # Checkers
                            # PostProcessing
                            # Trending
                            # Trending config
                            # Utilities
                    LINKDEF include/PID/LinkDef.h
                    BASENAME O2QcPID)

# ---- Test(s) ----

#add_executable(testQcPID test/testPID.cxx)
#target_link_libraries(testQcPID PRIVATE O2QcPID Boost::unit_test_framework)
#add_test(NAME testQcPID COMMAND testQcPID)
#set_property(TARGET ${test_name}
#  PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
#set_tests_properties(testQcPID PROPERTIES TIMEOUT 20)

# ---- Executables ----

#set(EXE_SRCS src/runPID.cxx)
#
#set(EXE_NAMES o2-qc-run-pid)
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

install(FILES pidft0tof.json
              pidtof.json
        DESTINATION etc)

get_property(dirs
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
