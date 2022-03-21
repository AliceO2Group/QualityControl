# ---- Library ----

add_library(O2QcHMPID)

target_sources(O2QcHMPID PRIVATE src/HmpidTask.cxx
                                 src/HmpidTaskDigits.cxx
                                 src/HmpidTaskClusters.cxx)

target_include_directories(
  O2QcHMPID
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcHMPID PUBLIC O2QualityControl O2::HMPIDReconstruction
                                                        O2::DataFormatsHMP)

get_target_property(O2_INCLUDE_DIRS O2::CommonDataFormat INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(ROOT_INCLUDE_DIRS ROOT::Core INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(HMPID_INCLUDE_DIRS O2::HMPIDReconstruction INTERFACE_INCLUDE_DIRECTORIES)

set(CMAKE_REQUIRED_INCLUDES ${O2_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} ${HMPID_INCLUDE_DIRS})

install(TARGETS O2QcHMPID
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(O2QcHMPID
                    HEADERS 
                    include/HMPID/HmpidTask.h
                    include/HMPID/HmpidTaskDigits.h
                    include/HMPID/HmpidTaskClusters.h
                    LINKDEF include/HMPID/LinkDef.h
                    BASENAME O2QcHMPID)

# ---- Test(s) ----

set(TEST_SRCS test/testQcHMPID.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE O2QcHMPID Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

