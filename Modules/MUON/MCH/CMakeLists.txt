set(MODULE_NAME "O2QcMuonChambers")

# ---- Files ----

set(SRCS
  src/GlobalHistogram.cxx
  src/PedestalsTask.cxx
  src/PhysicsTaskDigits.cxx
  src/PhysicsTaskPreclusters.cxx
  src/DecodingErrorsTask.cxx
  src/PedestalsCheck.cxx
  src/DecodingErrorsCheck.cxx
  src/PhysicsCheck.cxx
  src/PhysicsOccupancyCheck.cxx
  src/PhysicsPreclustersCheck.cxx
  src/TH1MCHReductor.cxx
)

set(HEADERS
  include/MCH/GlobalHistogram.h
  include/MCH/PedestalsTask.h
  include/MCH/PhysicsTaskDigits.h
  include/MCH/PhysicsTaskPreclusters.h
  include/MCH/DecodingErrorsTask.h
  include/MCH/PedestalsCheck.h
  include/MCH/DecodingErrorsCheck.h
  include/MCH/PhysicsCheck.h
  include/MCH/PhysicsOccupancyCheck.h
  include/MCH/PhysicsPreclustersCheck.h
  include/MCH/TH1MCHReductor.h
  include/MCH/MergeableTH1OccupancyPerDE.h
  include/MCH/MergeableTH1OccupancyPerDECycle.h
  include/MCH/MergeableTH1PseudoEfficiencyPerDE.h
  include/MCH/MergeableTH1PseudoEfficiencyPerDECycle.h
  include/MCH/MergeableTH1MPVPerDECycle.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS})

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MODULE_NAME} PUBLIC O2QualityControl O2QcMUONCommon O2::CommonDataFormat O2::GPUCommon
        $<TARGET_NAME_IF_EXISTS:O2::MCHMappingFactory> O2::MCHMappingImpl4 O2::MCHMappingSegContour O2::MCHBase O2::DataFormatsMCH O2::MCHRawDecoder O2::MCHCalibration O2::MCHPreClustering)

target_compile_definitions(${MODULE_NAME} PRIVATE $<$<TARGET_EXISTS:O2::MCHMappingFactory>:MCH_HAS_MAPPING_FACTORY>)

# Digit.h is moving from MCHBase to DataFormatsMCH : let's handle both
# gracefully for the moment...
get_target_property(O2_INCLUDE_DIRS O2::CommonDataFormat INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(ROOT_INCLUDE_DIRS ROOT::Core INTERFACE_INCLUDE_DIRECTORIES)

set(CMAKE_REQUIRED_INCLUDES ${O2_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS})
check_include_file_cxx("DataFormatsMCH/Digit.h" HAVE_DIGIT_IN_DATAFORMATS)

if(HAVE_DIGIT_IN_DATAFORMATS)
    target_compile_definitions(${MODULE_NAME} PRIVATE HAVE_DIGIT_IN_DATAFORMATS)
endif()

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- ROOT dictionary ----

add_root_dictionary(${MODULE_NAME}
                    HEADERS include/MCH/GlobalHistogram.h
                            include/MCH/PedestalsTask.h
                            include/MCH/PhysicsTaskDigits.h
                            include/MCH/PhysicsTaskPreclusters.h
                            include/MCH/PhysicsCheck.h
                            include/MCH/PhysicsOccupancyCheck.h
                            include/MCH/PhysicsPreclustersCheck.h
                            include/MCH/DecodingErrorsTask.h
                            include/MCH/DecodingErrorsCheck.h
                            include/MCH/PedestalsCheck.h
                            include/MCH/TH1MCHReductor.h
                            include/MCH/sampa_header.h
                            include/MCH/MergeableTH1OccupancyPerDE.h
                            include/MCH/MergeableTH1OccupancyPerDECycle.h
                            include/MCH/MergeableTH1PseudoEfficiencyPerDE.h
                            include/MCH/MergeableTH1PseudoEfficiencyPerDECycle.h
                            include/MCH/MergeableTH1MPVPerDECycle.h
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
