# ---- Library ----

add_library(QcTOF)

target_sources(QcTOF 
              PRIVATE src/TOFTask.cxx
                      src/TOFTaskCompressed.cxx
                      src/TOFCheckDiagnostic.cxx
                      src/TOFCheckRawsMulti.cxx
                      src/TOFCheckRawsTime.cxx 
                      src/TOFCheckRawsToT.cxx)

target_include_directories(
  QcTOF
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
         )

target_link_libraries(QcTOF PUBLIC QualityControl
                                    O2::TOFBase
                                    O2::TOFWorkflow
                                    O2::DataFormatsTOF
                                    O2::TOFCompression)

install(TARGETS QcTOF
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- ROOT dictionary ----

add_root_dictionary(QcTOF
                    HEADERS include/TOF/TOFTask.h
                            include/TOF/TOFTaskCompressed.h
                            include/TOF/TOFCheckDiagnostic.h
                            include/TOF/TOFCheckRawsMulti.h
                            include/TOF/TOFCheckRawsTime.h
                            include/TOF/TOFCheckRawsToT.h
                    LINKDEF include/TOF/LinkDef.h
                    BASENAME QcTOF)

# ---- Test(s) ----

add_executable(testQcTOF test/testTOF.cxx)
target_link_libraries(testQcTOF PRIVATE QcTOF Boost::unit_test_framework)
add_test(NAME testQcTOF COMMAND testQcTOF)
set_property(TARGET ${test_name}
  PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set_tests_properties(testQcTOF PROPERTIES TIMEOUT 20)

# ---- Executables ----

set(EXE_SRCS src/runTOF.cxx)

set(EXE_NAMES o2-qc-run-tof)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QualityControl CURL::libcurl)
endforeach()

# ---- Extra scripts ----

install(FILES tof.json tofcompressed.json DESTINATION etc)

get_property(dirs
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
