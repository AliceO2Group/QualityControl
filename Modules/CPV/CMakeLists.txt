# ---- Library ----

add_library(QcCPV)

target_sources(QcCPV PRIVATE src/PedestalCheck.cxx  src/PedestalTask.cxx )

target_include_directories(
  QcCPV
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcCPV PUBLIC QualityControl O2::CPVBase ROOT::Spectrum)

install(TARGETS QcCPV
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(QcCPV
                    HEADERS
  include/CPV/PedestalCheck.h
  include/CPV/PedestalTask.h
                    LINKDEF include/CPV/LinkDef.h
                    BASENAME QcCPV)

# ---- Test(s) ----

set(TEST_SRCS test/testQcCPV.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcCPV Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

