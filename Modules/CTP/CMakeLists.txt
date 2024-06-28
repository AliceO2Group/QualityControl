# ---- Library ----

add_library(O2QcCTP
            src/TrendingConfigCTP.cxx
            src/TH1ctpReductor.cxx )

target_sources(O2QcCTP PRIVATE
  src/RawDataReaderCheck.cxx
  src/CountersQcTask.cxx
  src/RawDataQcTask.cxx
  src/TH1ctpReductor.cxx
  src/CTPTrendingTask.cxx )

target_include_directories(
  O2QcCTP
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcCTP PUBLIC O2QualityControl O2QcCommon O2::DataFormatsCTP O2::CTPReconstruction O2QcCommon)

install(TARGETS O2QcCTP
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcCTP
                    HEADERS include/CTP/CountersQcTask.h
                            include/CTP/RawDataQcTask.h
                            include/CTP/RawDataReaderCheck.h
                            include/CTP/TH1ctpReductor.h
                            include/CTP/CTPTrendingTask.h
                            include/CTP/TrendingConfigCTP.h
                    LINKDEF include/CTP/LinkDef.h)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/CTP
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Test(s) ----

#set(TEST_SRCS test/testQcCTP.cxx) # uncomment to reenable the test which was empty

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcCTP Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

# ---- Extra scripts ----
