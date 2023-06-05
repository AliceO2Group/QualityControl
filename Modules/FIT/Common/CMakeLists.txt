set(MODULE_NAME "O2QcFITCommon")

# ---- Files ----

set(SRCS
  src/HelperFIT.cxx
  src/HelperHist.cxx
  src/DigitSync.cxx
)

set(HEADERS
  include/FITCommon/HelperFIT.h
  include/FITCommon/HelperHist.h
  include/FITCommon/DigitSync.h
)

# ---- Library ----

add_library(${MODULE_NAME} SHARED ${SRCS})

target_include_directories(
  ${MODULE_NAME}
  PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MODULE_NAME} PUBLIC O2QualityControl ROOT::Core ROOT::Hist O2::CommonDataFormat O2::DataFormatsFIT O2QcCommon)

set(CMAKE_REQUIRED_INCLUDES ${O2_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS})

install(
        TARGETS ${MODULE_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
