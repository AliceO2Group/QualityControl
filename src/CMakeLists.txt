# @author  Barthélémy von Haller
set(LIB_HEADERS_CORE
        "${CMAKE_CURRENT_BINARY_DIR}/../include/QualityControl/Version.h" # Generated header
        ../include/QualityControl/TaskInterface.h
        ../include/QualityControl/MonitorObject.h
        ../include/QualityControl/Quality.h
        ../include/QualityControl/TaskControl.h
        ../include/QualityControl/ObjectsManager.h
        ../include/QualityControl/QcInfoLogger.h
        ../include/QualityControl/PublisherInterface.h
        ../include/QualityControl/MockPublisher.h
        ../include/QualityControl/TaskFactory.h
        ../include/QualityControl/Checker.h
        ../include/QualityControl/CheckInterface.h
        ../include/QualityControl/DatabaseInterface.h
        ../include/QualityControl/DatabaseFactory.h
        ../include/QualityControl/SpyMainFrame.h
        ../include/QualityControl/SpyDevice.h
        ../include/QualityControl/ClientDataProvider.h
        ../include/QualityControl/TaskConfig.h
        )

set(LIB_SOURCES_CORE
        QCCoreDict.cxx # generated
        TaskInterface.cxx
        MonitorObject.cxx
        Quality.cxx
        TaskControl.cxx
        ObjectsManager.cxx
        PublisherInterface.cxx
        MockPublisher.cxx
        TaskFactory.cxx
        Checker.cxx
        CheckInterface.cxx
        DatabaseFactory.cxx
        SpyMainFrame.cxx
        SpyDevice.cxx
        ClientDataProvider.cxx
        )

if (FAIRROOT_FOUND)
    set(LIB_HEADERS_CORE ${LIB_HEADERS_CORE} ../include/QualityControl/AlfaPublisher.h)
    set(LIB_SOURCES_CORE ${LIB_SOURCES_CORE} AlfaPublisher.cxx)
    set(APP_TEST_RECEIVER_SOURCES AlfaReceiverForTests.cxx)
endif (FAIRROOT_FOUND)

if(MYSQL_FOUND AND ROOT_mysql_FOUND)
    set(LIB_HEADERS_CORE ${LIB_HEADERS_CORE} ../include/QualityControl/MySqlDatabase.h)
    set(LIB_SOURCES_CORE ${LIB_SOURCES_CORE} MySqlDatabase.cxx)
endif()

set(APP_SOURCES_TASK_LAUNCHER
        qcTaskLauncher.cxx
)
        
set(APP_SOURCES_CHECKER_LAUNCHER
        qcCheckerLauncher.cxx
)

set(APP_SOURCES_SPY
        qcSpy.cxx
)

set(APP_SOURCES_CONSUMER
        Consumer.cxx
)

set(LIBRARY_NAME_CORE QCCore)

# DICTIONARIES
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROOTSYS}/share/root/cmake/modules) # todo how to do it in a cleaner way ?
#include(RootNewMacros) # todo should we do it in the top cmakelists ?
list(APPEND DICTINC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/MonitorObject.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/Quality.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/CheckInterface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/SpyMainFrame.h
        )
set(CMAKE_ROOTTEST_DICT 1)
ROOT_GENERATE_DICTIONARY("${LIBRARY_NAME_CORE}Dict" ${DICTINC} LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/../include/QualityControl/LinkDef.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME_CORE}Dict_rdict.pcm DESTINATION lib)

# LIBRARY QCCore
add_library(${LIBRARY_NAME_CORE} SHARED ${LIB_SOURCES_CORE} )
# TODO use Configuration_LIBRARIES instead of Configuration_LIBRARIES_STATIC once the shared lib is fixed
target_link_libraries(${LIBRARY_NAME_CORE} InfoLogger-static Configuration-static DataSampling DataCollector 
                        ${ROOT_LIBRARIES} ${FAIRROOT_LIBRARIES})
set_target_properties(${LIBRARY_NAME_CORE} PROPERTIES
        VERSION ${VERSION}  # ${VERSION} was defined in the main CMakeLists.
        FRAMEWORK FALSE
        PUBLIC_HEADER "${LIB_HEADERS_CORE}" # specify the public headers
        )


# BINARIES

# Task Launcher
set(APPLICATION_TASK_LAUNCHER qcTaskLauncher)
add_executable(${APPLICATION_TASK_LAUNCHER} ${APP_SOURCES_TASK_LAUNCHER})
target_link_libraries(${APPLICATION_TASK_LAUNCHER} QCCore ${Boost_PROGRAM_OPTIONS_LIBRARY})
#${Boost_LOG}) # BOOST_LOG needed because we use FairMQParser...
install(TARGETS ${APPLICATION_TASK_LAUNCHER} DESTINATION bin COMPONENT apps)

# Checker Launcher
set(APPLICATION_CHECKER_LAUNCHER qcCheckerLauncher)
add_executable(${APPLICATION_CHECKER_LAUNCHER} ${APP_SOURCES_CHECKER_LAUNCHER})
target_link_libraries(${APPLICATION_CHECKER_LAUNCHER} 
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
install(TARGETS ${APPLICATION_CHECKER_LAUNCHER} DESTINATION bin COMPONENT apps)

# APP_SOURCES_SPY
set(APPLICATION_QC_SPY qcSpy)
add_executable(${APPLICATION_QC_SPY} ${APP_SOURCES_SPY})
target_link_libraries(${APPLICATION_QC_SPY} 
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
install(TARGETS ${APPLICATION_QC_SPY} DESTINATION bin COMPONENT apps)

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

# qcConsumer
set(APPLICATION_CONSUMER qcConsumer)
add_executable(${APPLICATION_CONSUMER} ${APP_SOURCES_CONSUMER})
target_link_libraries(${APPLICATION_CONSUMER} 
            ${ROOT_LIBRARIES}
            ${Boost_PROGRAM_OPTIONS_LIBRARY}
            QCCore
            )
install(TARGETS ${APPLICATION_CONSUMER} DESTINATION bin COMPONENT apps)

# Says how and where to install software
install(TARGETS ${LIBRARY_NAME_CORE} ${APPLICATION_TASK_LAUNCHER} ${APPLICATION_CHECKER_LAUNCHER} ${APPLICATION_NAME_RECEIVER} ${APPLICATION_CONSUMER} ${APPLICATION_QC_SPY}
  EXPORT ${PROJECT_NAME}Targets            # for downstream dependencies
  ARCHIVE DESTINATION lib COMPONENT libs   # static lib
  RUNTIME DESTINATION bin COMPONENT libs   # binaries
  LIBRARY DESTINATION lib COMPONENT libs   # shared lib
  FRAMEWORK DESTINATION bin COMPONENT libs # for mac
  PUBLIC_HEADER DESTINATION include/${PROJECT_NAME} COMPONENT devel   # headers for mac (note the different component -> different package)
  INCLUDES DESTINATION include             # headers
)
