# @author  Barthélémy von Haller

set(HEADERS
  Checker.h
  Check.h
)

set(SOURCES
  Checker.cxx
  Check.cxx
)

# Define two variables in order not to repeat ourselves.
set(LIBRARY QCChecker)
set(INCLUDE include/${LIBRARY})

# Define the library
add_library(${LIBRARY} SHARED ${HEADERS} ${SOURCES})

# Set the build version. It will be used in the name of the lib, with corresponding
# symlinks created. SOVERSION could also be specified for api version. 
set_target_properties(${LIBRARY} PROPERTIES
  VERSION ${VERSION}  # ${VERSION} was defined in the main CMakeLists.
    FRAMEWORK TRUE PUBLIC_HEADER "${HEADERS}"
)

# Specify how/where to install this library
install(TARGETS ${LIBRARY}
  ARCHIVE DESTINATION lib COMPONENT dev   # static lib
  RUNTIME DESTINATION bin COMPONENT libs  # binaries
  LIBRARY DESTINATION lib COMPONENT libs  # shared lib
  FRAMEWORK DESTINATION lib COMPONENT libs # for Mac
  PUBLIC_HEADER DESTINATION ${INCLUDE} COMPONENT dev   # headers
)
