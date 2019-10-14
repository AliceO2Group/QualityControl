# ---- Library ----

add_library(QcBenchmark)

target_sources(QcBenchmark PRIVATE src/BenchmarkCheck.cxx  src/BenchmarkTask.cxx )

target_include_directories(
  QcBenchmark
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcBenchmark PUBLIC QualityControl)

install(TARGETS QcBenchmark
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(QcBenchmark
                    HEADERS
  include/Benchmark/BenchmarkCheck.h
  include/Benchmark/BenchmarkTask.h
                    LINKDEF include/Benchmark/LinkDef.h
                    BASENAME QcBenchmark)

install(DIRECTORY etc DESTINATION Modules/Benchmark)

# ---- Test(s) ----

set(TEST_SRCS test/testQcBenchmark.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcBenchmark Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

