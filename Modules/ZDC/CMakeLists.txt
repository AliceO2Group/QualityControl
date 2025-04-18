# ---- Library ----

add_library(O2QcZDC)

target_sources(O2QcZDC PRIVATE src/ZDCRecBeautifyPlots.cxx  src/ZDCRecDataCheck.cxx  src/ZDCRecDataTask.cxx  src/ZDCRecDataPostProcessing.cxx src/PostProcessingConfigZDC.cxx  src/ZDCRawDataCheck.cxx  src/ZDCRawDataTask.cxx )

target_include_directories(
  O2QcZDC
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcZDC PUBLIC O2QualityControl O2::DataFormatsZDC)

install(TARGETS O2QcZDC
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcZDC
                    HEADERS
  include/ZDC/ZDCRecBeautifyPlots.h
  include/ZDC/ZDCRecDataCheck.h
  include/ZDC/ZDCRecDataTask.h
  include/ZDC/ZDCRecDataPostProcessing.h
  include/ZDC/ZDCRawDataCheck.h
  include/ZDC/ZDCRawDataTask.h
                    LINKDEF include/ZDC/LinkDef.h)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ZDC
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Test(s) ----

#set(TEST_SRCS test/testQcZDC.cxx) # uncomment to reenable the test which was empty

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcZDC Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

