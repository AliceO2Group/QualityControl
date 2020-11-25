# ---- Library ----

add_library(QcTRD)

target_sources(QcTRD PRIVATE src/TRDDigitQcTask.cxx )

target_include_directories(
  QcTRD
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcTRD PUBLIC QualityControl)

install(TARGETS QcTRD
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(QcTRD
                    HEADERS
                            include/TRD/TRDDigitQcTask.h
                    LINKDEF include/TRD/LinkDef.h
                    BASENAME QcTRD)

# ---- Test(s) ----

set(TEST_SRCS test/testQcTRD.cxx)

foreach(test ${TEST_SRCS})

  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcTRD Boost::unit_test_framework)

  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()


# ---- Install ----

install(TARGETS QcTRD ${EXE_NAMES}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Install config files ----

install(FILES TRDDigitQcTask.json
              DESTINATION etc)
