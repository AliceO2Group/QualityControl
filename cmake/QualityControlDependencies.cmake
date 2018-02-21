find_package(Boost 1.58 COMPONENTS unit_test_framework program_options system log)
find_package(Git QUIET)
find_package(Configuration REQUIRED)
find_package(Monitoring REQUIRED)
find_package(FairRoot REQUIRED)
find_package(MySQL REQUIRED)
find_package(Common REQUIRED)
find_package(InfoLogger REQUIRED)
find_package(DataSampling REQUIRED)
find_package(AliceO2 REQUIRED)
find_package(ROOT 6.06.02 COMPONENTS RHTTP RMySQL Gui REQUIRED)
find_package(CURL REQUIRED)
find_package(ZeroMQ REQUIRED)
include_directories(${MS_GSL_INCLUDE_DIR})

# todo not sure why this is needed
if (BOOST_FOUND AND NOT Boost_FOUND)
    set(BOOST_FOUND 1)
    set(Boost_FOUND 1)
endif ()

link_directories(${FAIRROOT_LIBRARY_DIR})
set(FAIRROOT_LIBRARIES Base FairMQ BaseMQ Logger)

if (NOT MYSQL_FOUND)
    message(WARNING "MySQL not found, the corresponding classes won't be built.")
    #elseif(NOT ROOT_RMySQL_LIBRARY)
    #    message(WARNING "MySQL ROOT class not found, the corresponding classes won't be built.")
else ()
    add_definitions(-D_WITH_MYSQL)
#    set(ROOT_LIBRARIES "${ROOT_LIBRARIES} -lRMySQL")
endif ()


o2_define_bucket(
        NAME
        o2_qualitycontrol_bucket

        DEPENDENCIES
        ${Boost_LOG_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Common_LIBRARIES}
        ${Configuration_LIBRARIES}
        ${Monitoring_LIBRARIES}
        ${ROOT_LIBRARIES}
        ${Boost_LOG_DEBUG}
        ${InfoLogger_LIBRARIES}
        ${DataSampling_LIBRARIES}
        ${CURL_LIBRARIES}
        ${AliceO2_LIBRARIES}

        SYSTEMINCLUDE_DIRECTORIES
        ${Boost_INCLUDE_DIRS}
        ${ROOT_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIRS}
        ${Configuration_INCLUDE_DIRS}
        ${Monitoring_INCLUDE_DIRS}
        ${ZEROMQ_INCLUDE_DIR}
        ${Common_INCLUDE_DIRS}
        ${InfoLogger_INCLUDE_DIRS}
        ${DataSampling_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${AliceO2_INCLUDE_DIRS}
        ${MS_GSL_INCLUDE_DIR}
)

o2_define_bucket(
        NAME
        o2_qualitycontrol_with_fairroot_bucket

        DEPENDENCIES
        o2_qualitycontrol_bucket
        ${FAIRROOT_LIBRARIES}

        SYSTEMINCLUDE_DIRECTORIES
        ${FAIRROOT_INCLUDE_DIR}
        ${FAIRROOT_INCLUDE_DIR}/fairmq
)


o2_define_bucket(
        NAME
        o2_qcmodules_base

        DEPENDENCIES
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Monitoring_LIBRARIES}
        ${InfoLogger_LIBRARIES}
        ${Common_LIBRARIES}
        QualityControl

        SYSTEMINCLUDE_DIRECTORIES
        ${Boost_INCLUDE_DIRS}
        ${Monitoring_INCLUDE_DIRS}
        ${InfoLogger_INCLUDE_DIRS}
        ${Common_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/Framework/include # another module's include dir
)

o2_define_bucket(
        NAME
        o2_qcmodules_common

        DEPENDENCIES
        o2_qcmodules_base
)

o2_define_bucket(
        NAME
        o2_qcmodules_example

        DEPENDENCIES
        o2_qcmodules_base

        SYSTEMINCLUDE_DIRECTORIES
        ${Boost_INCLUDE_DIRS}
        ${Configuration_INCLUDE_DIRS}
)


o2_define_bucket(
        NAME
        o2_qc_tobject2json

        DEPENDENCIES
        o2_qualitycontrol_bucket
        ${ZeroMQ_LIBRARY_STATIC}

        SYSTEMINCLUDE_DIRECTORIES
        ${ZeroMQ_INCLUDE_DIR}
)
