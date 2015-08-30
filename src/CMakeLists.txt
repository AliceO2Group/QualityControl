# @author  Barthélémy von Haller
set(LIB_HEADERS_CORE
  "${CMAKE_CURRENT_BINARY_DIR}/../include/QualityControl/Version.h" # Generated header
  ../include/QualityControl/TaskInterface.h
  ../include/QualityControl/MonitorObject.h
  ../include/QualityControl/Quality.h
  ../include/QualityControl/ExampleTask.h
  ../include/QualityControl/Exceptions.h
  ../include/QualityControl/TaskControl.h
  ../include/QualityControl/Publisher.h
  ../include/QualityControl/QcInfoLogger.h
  ../include/QualityControl/PublisherBackendInterface.h
  ../include/QualityControl/MockPublisherBackend.h
  ../include/QualityControl/TaskFactory.h
  ../include/QualityControl/Checker.h
  ../include/QualityControl/CheckInterface.h
  ../include/QualityControl/NonEmpty.h
  ../include/QualityControl/Checker.h
  ../include/QualityControl/CheckInterface.h
  ../include/QualityControl/NonEmpty.h
)

set(LIB_SOURCES_CORE
  TaskInterface.cxx
  MonitorObject.cxx
  Quality.cxx
  TaskControl.cxx
  Publisher.cxx
  ExampleTask.cxx
  PublisherBackendInterface.cxx
  MockPublisherBackend.cxx
  TaskFactory.cxx
  Checker.cxx
  CheckInterface.cxx
  NonEmpty.cxx
  Checker.cxx
  CheckInterface.cxx
  NonEmpty.cxx
)

if(FAIRROOT_FOUND)
  list(append LIB_HEADERS_CORE AlfaPublisherBackend.h)
  list(append LIB_SOURCES_CORE AlfaPublisherBackend.cxx)
endif(FAIRROOT_FOUND)

set(APP_HEADERS_LAUNCHER
  signalUtilities.h
)

set(APP_SOURCES_LAUNCHER
  taskLauncher.cxx
)

# LIBRARIES

# QCCore
set(LIBRARY_NAME_CORE QCCore)
add_library(${LIBRARY_NAME_CORE} SHARED ${LIB_SOURCES_CORE})
# TODO use Configuration_LIBRARIES instead of Configuration_LIBRARIES_STATIC once the shared lib is fixed
target_link_libraries(${LIBRARY_NAME_CORE} ${InfoLogger_LIBRARIES} ${Configuration_LIBRARIES_STATIC} DataSampling ${ROOT_LIBRARIES} ${FAIRROOT_LIBRARIES})
set_target_properties(${LIBRARY_NAME_CORE} PROPERTIES
  VERSION ${VERSION}  # ${VERSION} was defined in the main CMakeLists.
  FRAMEWORK FALSE
  PUBLIC_HEADER "${LIB_HEADERS_CORE}" # specify the public headers
)



# BINARIES

# Launcher
set(APPLICATION_LAUNCHER qcTaskLauncher)
add_executable(${APPLICATION_LAUNCHER} ${APP_SOURCES_LAUNCHER})
target_link_libraries(${APPLICATION_LAUNCHER} QCCore ${Boost_PROGRAM_OPTIONS_LIBRARY})
install(TARGETS ${APPLICATION_LAUNCHER} DESTINATION bin COMPONENT apps)



# Says how and where to install software
install(TARGETS ${LIBRARY_NAME_CORE} ${APPLICATION_LAUNCHER}
  EXPORT ${PROJECT_NAME}Targets            # for downstream dependencies
  ARCHIVE DESTINATION lib COMPONENT libs   # static lib
  RUNTIME DESTINATION bin COMPONENT libs   # binaries
  LIBRARY DESTINATION lib COMPONENT libs   # shared lib
  PUBLIC_HEADER DESTINATION include/${PROJECT_NAME} COMPONENT devel   # headers (note the different component -> different package)
  FRAMEWORK DESTINATION bin COMPONENT libs # for mac
)