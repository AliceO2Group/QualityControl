# ---- Library ----

add_library(O2QcCPV)

target_sources(O2QcCPV PRIVATE src/PhysicsTask.cxx  src/PedestalCheck.cxx  src/PedestalTask.cxx )

target_include_directories(
  O2QcCPV
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcCPV PUBLIC O2QualityControl O2::CPVBase ROOT::Spectrum)

install(TARGETS O2QcCPV
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcCPV
                    HEADERS
  include/CPV/PhysicsTask.h
  include/CPV/PedestalCheck.h
  include/CPV/PedestalTask.h
                    LINKDEF include/CPV/LinkDef.h
                    BASENAME O2QcCPV)

# ---- Test(s) ----

set(TEST_SRCS test/testQcCPV.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcCPV Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

