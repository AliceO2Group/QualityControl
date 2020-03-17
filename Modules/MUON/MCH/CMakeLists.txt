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
  include/MCH/MuonChambersMapping.h
  include/MCH/MuonChambersDataDecoder.h
  include/MCH/RawDataProcessor.h
  include/MCH/PhysicsDataProcessor.h
  include/MCH/MCHCheckPedestals.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS})

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#target_link_libraries(${MODULE_NAME} PUBLIC QualityControl)
target_link_libraries(${MODULE_NAME} PUBLIC QualityControl O2::CommonDataFormat O2::GPUCommon O2::MCHMappingImpl3 O2::MCHMappingSegContour)

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

#generate_root_dict(MODULE_NAME ${MODULE_NAME} LINKDEF "include/MCH/LinkDef.h" DICT_CLASS "${MODULE_NAME}Dict")

add_root_dictionary(${MODULE_NAME}
                    HEADERS include/MCH/MuonChambersMapping.h
                            include/MCH/MuonChambersDataDecoder.h
                            include/MCH/RawDataProcessor.h
                            include/MCH/PhysicsDataProcessor.h
                            include/MCH/MCHCheckPedestals.h
                            include/MCH/sampa_header.h
                    LINKDEF include/MCH/LinkDef.h
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
