set(MODULE_NAME "QcCommon")

# ---- Dependencies ----

find_package(
  ROOT 6.06.02
  COMPONENTS
    RHTTP
    RMySQL
    Gui
    REQUIRED
)
find_package(Boost 1.58 COMPONENTS unit_test_framework)

# ---- Files ----

set(SRCS src/NonEmpty.cxx src/MeanIsAbove.cxx)

set(HEADERS include/Common/NonEmpty.h include/Common/MeanIsAbove.h)

# Produce the final Version.h using template Version.h.in and substituting variables. We don't want to polute our source
# tree with it, thus putting it in binary tree.
configure_file("include/Common/Version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/Common/Version.h" @ONLY)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS} ${MODULE_NAME}Dict.cxx)

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MODULE_NAME} PUBLIC QualityControl PRIVATE ROOT::Graf)

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

generate_root_dict(MODULE_NAME ${MODULE_NAME} LINKDEF "include/Common/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

# ---- Tests ----

set(TEST_SRCS test/testMeanIsAbove.cxx test/testNonEmpty.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(
    REGEX
    REPLACE
      ".cxx"
      ""
      test_name
      ${test_name}
  )

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} PRIVATE ${MODULE_NAME} Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()
