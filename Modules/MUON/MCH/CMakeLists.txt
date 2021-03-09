set(MODULE_NAME "QcMuonChambers")

# ---- Files ----

set(SRCS
  src/Mapping.cxx
  src/Decoding.cxx
  src/GlobalHistogram.cxx
  src/PedestalsTask.cxx
  src/PhysicsTaskDigits.cxx
  src/PhysicsTaskPreclusters.cxx
  src/PedestalsCheck.cxx
  src/PhysicsCheck.cxx
  src/TH1MCHReductor.cxx
)

set(HEADERS
  include/MCH/Mapping.h
  include/MCH/Decoding.h
  include/MCH/GlobalHistogram.h
  include/MCH/PedestalsTask.h
  include/MCH/PhysicsTaskDigits.h
  include/MCH/PhysicsTaskPreclusters.h
  include/MCH/PedestalsCheck.h
  include/MCH/PhysicsCheck.h
  include/MCH/TH1MCHReductor.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS})

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MODULE_NAME} PUBLIC QualityControl O2::CommonDataFormat O2::GPUCommon 
        $<TARGET_NAME_IF_EXISTS:O2::MCHMappingFactory> O2::MCHMappingImpl4 O2::MCHMappingSegContour O2::MCHBase O2::DataFormatsMCH O2::MCHRawDecoder O2::MCHCalibration O2::MCHPreClustering)

target_compile_definitions(${MODULE_NAME} PRIVATE $<$<TARGET_EXISTS:O2::MCHMappingFactory>:MCH_HAS_MAPPING_FACTORY>)

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

add_root_dictionary(${MODULE_NAME}
                    HEADERS include/MCH/Mapping.h
                            include/MCH/Decoding.h
                            include/MCH/GlobalHistogram.h
                            include/MCH/PedestalsTask.h
                            include/MCH/PhysicsTaskDigits.h
                            include/MCH/PhysicsTaskPreclusters.h
                            include/MCH/PhysicsCheck.h
                            include/MCH/PedestalsCheck.h
                            include/MCH/TH1MCHReductor.h
                            include/MCH/sampa_header.h
                    LINKDEF include/MCH/LinkDef.h)

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
