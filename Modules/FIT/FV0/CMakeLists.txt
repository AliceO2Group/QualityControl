# ---- Library ----

add_library(O2QcFV0)

target_sources(O2QcFV0 PRIVATE src/TH1ReductorLaser.cxx
                               src/DigitQcTaskLaser.cxx
                               src/DigitQcTask.cxx
                               src/GenericCheck.cxx
                               src/CFDEffCheck.cxx
                               src/PostProcTask.cxx
                               src/OutOfBunchCollCheck.cxx
                               src/TriggersSwVsTcmCheck.cxx
                               src/OutOfBunchCollFeeModulesCheck.cxx)

target_include_directories(
  O2QcFV0
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcFV0 PUBLIC O2QualityControl
                                     O2::DataFormatsFV0
                                     O2::FV0Base
                                     O2::DataFormatsParameters
                                     O2QcCommon)

install(TARGETS O2QcFV0
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcFV0
  HEADERS include/FV0/TH1ReductorLaser.h
          include/FV0/DigitQcTaskLaser.h
          include/FV0/DigitQcTask.h
          include/FV0/PostProcTask.h
          include/FV0/CFDEffCheck.h
          include/FV0/OutOfBunchCollCheck.h
          include/FV0/GenericCheck.h
          include/FV0/TriggersSwVsTcmCheck.h
          include/FV0/OutOfBunchCollFeeModulesCheck.h
  LINKDEF include/FV0/LinkDef.h)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/FV0
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Test(s) ----

#set(TEST_SRCS test/testQcFV0.cxx) # uncomment to reenable the test which was empty

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
