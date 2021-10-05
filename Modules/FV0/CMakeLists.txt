# ---- Library ----

add_library(O2QcFV0)

target_sources(O2QcFV0 PRIVATE  src/DigitQcTask.cxx
                                src/CFDEffCheck.cxx
                                src/BasicPPTask.cxx
                                src/OutOfBunchCollTask.cxx
                                src/OutOfBunchCollCheck.cxx
                                #src/TriggerQcTask.cxx
                                )

target_include_directories(
  O2QcFV0
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcFV0 PUBLIC O2QualityControl O2::DataFormatsFV0)

install(TARGETS O2QcFV0
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcFV0
                    HEADERS
  include/FV0/DigitQcTask.h
  include/FV0/Helper.h
  include/FV0/CFDEffCheck.h
  include/FV0/BasicPPTask.h
  include/FV0/OutOfBunchCollTask.h
  include/FV0/OutOfBunchCollCheck.h
  #include/FV0/TriggerQcTask.h
                    LINKDEF include/FV0/LinkDef.h
                    BASENAME O2QcFV0)

# ---- Test(s) ----

set(TEST_SRCS test/testQcFV0.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcFV0 Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()
