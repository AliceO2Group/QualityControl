# ---- Library ----

add_library(QcTPC)

target_sources(QcTPC PRIVATE src/PID.cxx)

target_include_directories(
  QcTPC
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${O2_ROOT}/include/GPU)

target_link_libraries(QcTPC PUBLIC QualityControl O2::TPCQC)

install(TARGETS QcTPC
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(QcTPC
                    HEADERS include/TPC/PID.h
                    LINKDEF include/TPC/LinkDef.h
                    BASENAME QcTPC)

# ---- Test(s) ----

set(TEST_SRCS test/testQcTPC.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PUBLIC QcTPC
                        PRIVATE Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
