# ---- Library ---- 

add_library(O2QcEMCAL)

target_sources(O2QcEMCAL PRIVATE src/RawTask.cxx src/RawCheck.cxx src/DigitsQcTask.cxx src/DigitCheck.cxx src/DigitOccupancyReductor.cxx)

target_include_directories(
  O2QcEMCAL
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

target_link_libraries(O2QcEMCAL PUBLIC O2QualityControl O2::EMCALBase O2::EMCALReconstruction O2::CCDB O2::EMCALCalib)

add_root_dictionary(O2QcEMCAL
  HEADERS include/EMCAL/DigitsQcTask.h
  include/EMCAL/DigitCheck.h
  include/EMCAL/RawTask.h
  include/EMCAL/RawCheck.h
  include/EMCAL/DigitOccupancyReductor.h
  LINKDEF include/EMCAL/LinkDef.h
  BASENAME O2QcEMCAL)

install(TARGETS O2QcEMCAL
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install (
  DIRECTORY etc DESTINATION Modules/EMCAL
  )

# ---- Executables ----

add_executable(qcRunEmcalDigit src/runQCEMCALDigits.cxx)
target_link_libraries(qcRunEmcalDigit PUBLIC O2QualityControl O2QcEMCAL O2::DataFormatsEMCAL O2::EMCALBase O2::EMCALWorkflow O2::CCDB O2::EMCALCalib)
install(
  TARGETS qcRunEmcalDigit
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

# ---- Tests ----

set(
  TEST_SRCS
  )

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework O2::CCDB O2::EMCALCalib)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()
