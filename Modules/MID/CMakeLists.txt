# ---- Library ----

add_library(QcMID)

target_sources(QcMID PRIVATE src/MIDCheck.cxx  src/RawDataQcTask.cxx )

target_include_directories(
  QcMID
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcMID PUBLIC QualityControl)

install(TARGETS QcMID
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(QcMID
                    HEADERS
  include/MID/MIDCheck.h
  include/MID/RawDataQcTask.h
                    LINKDEF include/MID/LinkDef.h
                    BASENAME QcMID)

# ---- Test(s) ----

add_executable(testQcMID test/testQcMID.cxx)
target_link_libraries(testQcMID
                      PRIVATE QcMID Boost::unit_test_framework)
add_test(NAME testQcMID COMMAND testQcMID)
set_tests_properties(testQcMID PROPERTIES TIMEOUT 60)
