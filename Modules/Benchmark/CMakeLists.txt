# ---- Library ----

add_library(O2QcBenchmark)

target_sources(O2QcBenchmark PRIVATE src/EmptyPPTask.cxx  src/AlwaysGoodCheck.cxx src/TH1FTask.cxx)

target_include_directories(
  O2QcBenchmark
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcBenchmark PUBLIC O2QualityControl)

install(TARGETS O2QcBenchmark
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcBenchmark
                    HEADERS
  include/Benchmark/EmptyPPTask.h
                    include/Benchmark/AlwaysGoodCheck.h
                    include/Benchmark/TH1FTask.h
                    LINKDEF include/Benchmark/LinkDef.h
                    BASENAME O2QcBenchmark)

install(DIRECTORY etc DESTINATION Modules/Benchmark)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Benchmark
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# ---- Test(s) ----

set(TEST_SRCS test/testQcBenchmark.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcBenchmark Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

