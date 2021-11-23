# ---- Library ----

add_library(O2QcTRD)

target_sources(O2QcTRD PRIVATE src/TrackletsCheck.cxx  src/TrackletsTask.cxx  src/PulseHeightCheck.cxx  src/Noise.cxx  src/PulseHeight.cxx  src/RawData.cxx  src/DigitsTask.cxx
                             src/DigitsCheck.cxx)

target_include_directories(
  O2QcTRD
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcTRD PUBLIC O2QualityControl)

install(TARGETS O2QcTRD
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcTRD
                    HEADERS
  include/TRD/TrackletsCheck.h
  include/TRD/TrackletsTask.h
  include/TRD/PulseHeightCheck.h
  include/TRD/Noise.h
  include/TRD/PulseHeight.h
  include/TRD/RawData.h
                            include/TRD/DigitsTask.h
                            include/TRD/DigitsCheck.h
                    LINKDEF include/TRD/LinkDef.h
                    BASENAME O2QcTRD)

# ---- Test(s) ----

set(TEST_SRCS test/testQcTRD.cxx)

foreach(test ${TEST_SRCS})

  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcTRD Boost::unit_test_framework)

  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()


# ---- Install ----

install(TARGETS O2QcTRD ${EXE_NAMES}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Install config files ----

install(FILES DigitsTask.json
              DESTINATION etc)
