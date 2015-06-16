# @author  Barthélémy von Haller

set(HEADERS
)

set(SOURCES
  dataDump.cpp
)

# Executable name then used to add an executable to the project.
# This is a make target, so you can do a "make dataDump"
set(APPLICATION dataDump)
add_executable(${APPLICATION} ${HEADERS} ${SOURCES})

# Links the target exe (dataDump) against the library (libhello)
target_link_libraries(${APPLICATION} hello ${Boost_PROGRAM_OPTIONS_LIBRARY})

# Says how and where to install software
install(TARGETS ${APPLICATION} DESTINATION bin COMPONENT apps)
