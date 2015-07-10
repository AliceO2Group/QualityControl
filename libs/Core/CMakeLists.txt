# @author  Barthélémy von Haller

# Produce the final Version.h using template Version.h.in and substituting variables.
# We don't want to polute our source tree with it, thus putting it in binary tree.
configure_file("Version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.h" @ONLY)

set(HEADERS
  "${CMAKE_CURRENT_BINARY_DIR}/Version.h"
  QCTask.h
  MonitorObject.h
  Quality.h
  Exceptions.h
)

set(SOURCES
  Version.cxx
  QCTask.cxx
  MonitorObject.cxx
  Quality.cxx
)

# Define two variables in order not to repeat ourselves.
set(LIBRARY QCCore)
set(INCLUDE include/${LIBRARY})

# Define the library
add_library(${LIBRARY} SHARED ${HEADERS} ${SOURCES})

# Links the target(s) against libraries
target_link_libraries(${LIBRARY} ${InfoLogger_LIBRARIES})

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
