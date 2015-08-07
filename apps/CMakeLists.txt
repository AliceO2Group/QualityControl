# @author  Barthélémy von Haller

set(HEADERS
)

set(SOURCES
  taskLauncher.cxx
)

include_directories(libs)

# Executable name then used to add an executable to the project.
# This is a make target, so you can do a "make qcTaskLauncher"
set(APPLICATION qcTaskLauncher)
add_executable(${APPLICATION} ${SOURCES})

# Links the target exe (qcTaskLauncher) against the QC libraries
target_link_libraries(${APPLICATION} QCCore QCPublisher QCChecker ${Boost_PROGRAM_OPTIONS_LIBRARY})

# Says how and where to install software
install(TARGETS ${APPLICATION} DESTINATION bin COMPONENT apps)
