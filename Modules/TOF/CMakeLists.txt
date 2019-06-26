set(MODULE_NAME "QcTOF")

# ---- Files ----

set(SRCS
  src/TOFCheckRawsToT.cxx
  src/TOFCheckRawsTime.cxx
  src/TOFCheckRawsMulti.cxx
  src/TOFTask.cxx
)

set(HEADERS
  include/TOF/TOFCheckRawsToT.h
  include/TOF/TOFCheckRawsTime.h
  include/TOF/TOFCheckRawsMulti.h
  include/TOF/TOFTask.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS} ${MODULE_NAME}Dict.cxx)

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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

generate_root_dict(MODULE_NAME ${MODULE_NAME} LINKDEF "include/TOF/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

# ---- Tests ----

set(
  TEST_SRCS
)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
