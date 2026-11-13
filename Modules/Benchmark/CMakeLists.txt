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
                    LINKDEF include/Benchmark/LinkDef.h)

install(DIRECTORY etc DESTINATION Modules/Benchmark)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Benchmark
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Test(s) ----

# uncomment to reenable the test which was empty
#add_executable(o2-qc-test-benchmark test/testQcBenchmark.cxx)
#target_link_libraries(o2-qc-test-benchmark PRIVATE O2QcBenchmark O2::Catch2)
#set_property(TARGET o2-qc-test-benchmark
#             PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
#add_test(NAME o2-qc-test-benchmark COMMAND o2-qc-test-benchmark)
#set_tests_properties(o2-qc-test-benchmark PROPERTIES TIMEOUT 20)

