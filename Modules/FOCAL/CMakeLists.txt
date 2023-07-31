# ---- Library ----

add_library(O2QcFOCAL)

target_sources(O2QcFOCAL PRIVATE src/PedestalCalibTask.cxx  src/TestbeamRawTask.cxx)

target_include_directories(
  O2QcFOCAL
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcFOCAL PUBLIC O2QualityControl O2::DetectorsRaw O2::Headers O2::ITSMFTReconstruction O2::DataFormatsFOCAL O2::FOCALReconstruction O2::FOCALCalib)

install(TARGETS O2QcFOCAL
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcFOCAL
  HEADERS
  include/FOCAL/TestbeamRawTask.h
      include/FOCAL/PedestalCalibTask.h
        LINKDEF include/FOCAL/LinkDef.h)

install(
  DIRECTORY etc DESTINATION Modules/FOCAL
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/FOCAL
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Test(s) ----
#set(TEST_SRCS test/testQcFOCAL.cxx) # uncomment to reenable the test which was empty

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
    PRIVATE O2QcFOCAL Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()
