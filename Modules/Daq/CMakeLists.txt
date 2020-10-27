# ---- Library ----

add_library(QcDaq)

target_sources(QcDaq PRIVATE src/DaqTask.cxx)

target_include_directories(
  QcDaq
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QcDaq PUBLIC QualityControl PRIVATE ROOT::Gpad O2::DetectorsRaw)

add_root_dictionary(QcDaq
                    HEADERS include/Daq/DaqTask.h
                    LINKDEF include/Daq/LinkDef.h
                    BASENAME QcDaq)

install(TARGETS QcDaq
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Tests ----

set(TEST_SRCS test/testQcDaq.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcDaq Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()
