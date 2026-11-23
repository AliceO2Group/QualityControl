# ---- Library ----

add_library(O2QcDaq)

target_sources(O2QcDaq PRIVATE src/DaqTask.cxx)

target_include_directories(
  O2QcDaq
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(O2QcDaq PUBLIC O2QualityControl PRIVATE 
                      ROOT::Gpad 
                      O2::DetectorsRaw 
                      O2QcCommon)

add_root_dictionary(O2QcDaq
                    HEADERS include/Daq/DaqTask.h
                    LINKDEF include/Daq/LinkDef.h)

install(TARGETS O2QcDaq
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Daq
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- Tests ----

add_executable(o2-qc-test-daq test/testQcDaq.cxx)
target_link_libraries(o2-qc-test-daq PRIVATE O2QcDaq O2::Catch2)
set_property(TARGET o2-qc-test-daq
             PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
add_test(NAME o2-qc-test-daq COMMAND o2-qc-test-daq)
set_tests_properties(o2-qc-test-daq PROPERTIES TIMEOUT 20)
