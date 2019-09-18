set(MODULE_NAME "QcMuonChambers")

# ---- Files ----

set(SRCS
  src/MuonChambersMapping.cxx
  src/MuonChambersDataDecoder.cxx
  src/RawDataProcessor.cxx
  src/PhysicsDataProcessor.cxx
  src/MCHCheckPedestals.cxx
)

set(HEADERS
  include/MuonChambers/MuonChambersMapping.h
  include/MuonChambers/MuonChambersDataDecoder.h
  include/MuonChambers/RawDataProcessor.h
  include/MuonChambers/PhysicsDataProcessor.h
  include/MuonChambers/MCHCheckPedestals.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS})

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#target_link_libraries(${MODULE_NAME} PUBLIC QualityControl)
target_link_libraries(${MODULE_NAME} PUBLIC QualityControl O2::MCHMappingImpl3)

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

#generate_root_dict(MODULE_NAME ${MODULE_NAME} LINKDEF "include/MuonChambers/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

add_root_dictionary(${MODULE_NAME}
                    HEADERS include/MuonChambers/MuonChambersMapping.h
                            include/MuonChambers/MuonChambersDataDecoder.h
                            include/MuonChambers/RawDataProcessor.h
                            include/MuonChambers/PhysicsDataProcessor.h
                            include/MuonChambers/MCHCheckPedestals.h
                            include/MuonChambers/sampa_header.h
                    LINKDEF include/MuonChambers/LinkDef.h
                    BASENAME ${MODULE_NAME})

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
