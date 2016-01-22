# @author  Barthélémy von Haller
set(LIB_HEADERS_CORE
        "${CMAKE_CURRENT_BINARY_DIR}/../include/QualityControl/Version.h" # Generated header
        ../include/QualityControl/TaskInterface.h
        ../include/QualityControl/MonitorObject.h
        ../include/QualityControl/Quality.h
        ../include/QualityControl/ExampleTask.h
        ../include/QualityControl/Exceptions.h
        ../include/QualityControl/TaskControl.h
        ../include/QualityControl/ObjectsManager.h
        ../include/QualityControl/QcInfoLogger.h
        ../include/QualityControl/PublisherInterface.h
        ../include/QualityControl/MockPublisher.h
        ../include/QualityControl/TaskFactory.h
        ../include/QualityControl/Checker.h
        ../include/QualityControl/CheckInterface.h
        ../include/QualityControl/NonEmpty.h
        ../include/QualityControl/Checker.h
        ../include/QualityControl/CheckInterface.h
        ../include/QualityControl/NonEmpty.h
        )

set(LIB_SOURCES_CORE
        QCCoreDict.cxx # generated
        TaskInterface.cxx
        MonitorObject.cxx
        Quality.cxx
        TaskControl.cxx
        ObjectsManager.cxx
        ExampleTask.cxx
        PublisherInterface.cxx
        MockPublisher.cxx
        TaskFactory.cxx
        Checker.cxx
        CheckInterface.cxx
        NonEmpty.cxx
        Checker.cxx
        CheckInterface.cxx
        NonEmpty.cxx
        )

if (FAIRROOT_FOUND)
    set(LIB_HEADERS_CORE ${LIB_HEADERS_CORE} ../include/QualityControl/AlfaPublisher.h)
    set(LIB_SOURCES_CORE ${LIB_SOURCES_CORE} AlfaPublisher.cxx)
    set(APP_TEST_RECEIVER_SOURCES AlfaReceiverForTests.cxx)
endif (FAIRROOT_FOUND)

set(APP_HEADERS_LAUNCHER
        signalUtilities.h
        )

set(APP_SOURCES_LAUNCHER
        taskLauncher.cxx
        )


# DICTIONARIES
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROOTSYS}/share/root/cmake/modules) # todo how to do it in a cleaner way ?
#include(RootNewMacros) # todo should we do it in the top cmakelists ?
include(ROOTMacros) # todo we should use the standard rootnewmacros but it forces to install some files we don't want
list(APPEND DICTINC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/MonitorObject.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/Quality.h)
set(CMAKE_INSTALL_LIBDIR lib) # todo remove when ROOT fixes the macro below
ROOT_GENERATE_DICTIONARY("QCCoreDict" ${DICTINC} LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/LinkDef.h)

# LIBRARIES

# QCCore
set(LIBRARY_NAME_CORE QCCore)
add_library(${LIBRARY_NAME_CORE} SHARED ${LIB_SOURCES_CORE} )
# TODO use Configuration_LIBRARIES instead of Configuration_LIBRARIES_STATIC once the shared lib is fixed
target_link_libraries(${LIBRARY_NAME_CORE} InfoLogger-static Configuration-static DataSampling ${ROOT_LIBRARIES} ${FAIRROOT_LIBRARIES})
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
#${Boost_LOG}) # BOOST_LOG needed because we use FairMQParser...
install(TARGETS ${APPLICATION_LAUNCHER} DESTINATION bin COMPONENT apps)

# alfaTestReceiver
set(APPLICATION_NAME_RECEIVER "")
if (FAIRROOT_FOUND)
    set(APPLICATION_NAME_RECEIVER alfaTestReceiver)
    add_executable(${APPLICATION_NAME_RECEIVER} ${APP_TEST_RECEIVER_SOURCES})
    target_link_libraries(${APPLICATION_NAME_RECEIVER}
            ${Boost_LOG_LIBRARY}
            ${Boost_THREAD_LIBRARY}
            ${Boost_SYSTEM_LIBRARY}
            ${FAIRROOT_LIBRARIES}
            ${ROOT_LIBRARIES}
            ${Boost_PROGRAM_OPTIONS_LIBRARY}
            ${FAIRROOT_LIBRARIES}
            pthread
            QCCore
            )
endif (FAIRROOT_FOUND)

# Says how and where to install software
install(TARGETS ${LIBRARY_NAME_CORE} ${APPLICATION_LAUNCHER} ${APPLICATION_NAME_RECEIVER}
        EXPORT ${PROJECT_NAME}Targets            # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs   # static lib
        RUNTIME DESTINATION bin COMPONENT libs   # binaries
        LIBRARY DESTINATION lib COMPONENT libs   # shared lib
        PUBLIC_HEADER DESTINATION include/${PROJECT_NAME} COMPONENT devel   # headers (note the different component -> different package)
        FRAMEWORK DESTINATION bin COMPONENT libs # for mac
        )

