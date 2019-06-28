set(MODULE_NAME "QcExample")

# ---- Files ----

set(
  SRCS
  src/BenchmarkTask.cxx
  src/ExampleTask.cxx
  src/FakeCheck.cxx
)

set(
  HEADERS
  include/Example/BenchmarkTask.h
  include/Example/ExampleTask.h
  include/Example/FakeCheck.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS} ${MODULE_NAME}Dict.cxx)

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MODULE_NAME} PUBLIC QualityControl)

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

generate_root_dict(MODULE_NAME ${MODULE_NAME} LINKDEF "include/Example/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

# ---- Tests ----

set(TEST_SRCS test/testFactory.cxx test/testQcExample.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
