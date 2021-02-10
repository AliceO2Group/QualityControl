# ---- Library ----

add_library(O2QcHMPID)

target_sources(O2QcHMPID PRIVATE src/HmpidEquipment.cxx)
target_sources(O2QcHMPID PRIVATE src/HmpidDecoder.cxx)
target_sources(O2QcHMPID PRIVATE src/HmpidDecodeRawMem.cxx)
target_sources(O2QcHMPID PRIVATE src/HmpidTask.cxx)

target_include_directories(
  O2QcHMPID
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcHMPID PUBLIC O2QualityControl)

install(TARGETS O2QcHMPID
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcHMPID
                    HEADERS
  include/HMPID/HmpidEquipment.h
  include/HMPID/HmpidDecoder.h
  include/HMPID/HmpidDecodeRawMem.h
  include/HMPID/HmpidTask.h
                    LINKDEF include/HMPID/LinkDef.h
                    BASENAME O2QcHMPID)

# ---- Test(s) ----

set(TEST_SRCS test/testQcHMPID.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcHMPID Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

