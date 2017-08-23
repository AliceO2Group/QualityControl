find_package(Boost 1.58 COMPONENTS unit_test_framework program_options system log)
find_package(Git QUIET)
find_package(Configuration REQUIRED)
find_package(Monitoring REQUIRED)
find_package(FairRoot)
find_package(MySQL)
find_package(Common REQUIRED)
find_package(InfoLogger REQUIRED)

# todo not sure why this is needed
if (BOOST_FOUND AND NOT Boost_FOUND)
    set(BOOST_FOUND 1)
    set(Boost_FOUND 1)
endif ()

link_directories(${FAIRROOT_LIBRARY_DIR})
find_package(ROOT 6.06.02 COMPONENTS RHTTP RMySQL Gui)
set(FAIRROOT_LIBRARIES Base FairMQ BaseMQ)

if (NOT ROOT_FOUND)
    return()
endif ()

if (NOT MYSQL_FOUND)
    message(WARNING "MySQL not found, the corresponding classes won't be built.")
    #elseif(NOT ROOT_RMySQL_LIBRARY)
    #    message(WARNING "MySQL ROOT class not found, the corresponding classes won't be built.")
else ()
    add_definitions(-D_WITH_MYSQL)
    set(ROOT_LIBRARIES "${ROOT_LIBRARIES} -lRMySQL")
endif ()

o2_define_bucket(
        NAME
        o2_qualitycontrol_bucket

        DEPENDENCIES
        ${Boost_LOG_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Common_LIBRARIES}
        ${Configuration_LIBRARIES}
        DataSampling
        ${Monitoring_LIBRARIES}
        ${ROOT_LIBRARIES}
        ${Boost_LOG_DEBUG}
        ${InfoLogger_LIBRARIES}

        SYSTEMINCLUDE_DIRECTORIES
        ${Boost_INCLUDE_DIRS}
        ${ROOT_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIRS}
        ${Configuration_INCLUDE_DIRS}
        ${Monitoring_INCLUDE_DIRS}
        ${ZEROMQ_INCLUDE_DIR}
        ${Common_INCLUDE_DIRS}
        ${InfoLogger_INCLUDE_DIRS}
)

o2_define_bucket(
        NAME
        o2_qualitycontrol_with_fairroot_bucket

        DEPENDENCIES
        o2_qualitycontrol_bucket
        ${FAIRROOT_LIBRARIES}

        SYSTEMINCLUDE_DIRECTORIES
        ${FAIRROOT_INCLUDE_DIR}
)
