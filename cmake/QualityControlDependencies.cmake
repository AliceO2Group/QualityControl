find_package(Boost 1.58 COMPONENTS unit_test_framework program_options system log)
find_package(Git QUIET)
find_package(Configuration REQUIRED)
find_package(FairRoot)
find_package(MySQL)

# todo not sure why this is needed
if(BOOST_FOUND AND NOT Boost_FOUND)
    set(BOOST_FOUND 1)
    set(Boost_FOUND 1)
endif()

if(FAIRROOT_FOUND)
    # this should go away when fairrot provides a proper Find script or proper config scripts
    # See : http://www.cmake.org/cmake/help/v3.0/command/link_directories.html
    link_directories(${FAIRROOT_LIBRARY_DIR})
    find_package(ROOT 6.06.02 COMPONENTS RHTTP RMySQL Gui)
    set(FAIRROOT_LIBRARIES Base FairMQ BaseMQ)
else(FAIRROOT_FOUND)
    message(WARNING "FairRoot not found, corresponding classes will not be compiled.")
        # todo not sure that it is needed (when using alibuild it hsould not)
    SET(ROOT_DIR "/opt/root-6/cmake/")
    find_package(ROOT CONFIG COMPONENTS Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore QUIET)
    set(FAIRROOT_LIBRARIES "")
endif(FAIRROOT_FOUND)

if(NOT MYSQL_FOUND)
    message(WARNING "MySQL not found, the corresponding classes won't be built.")
elseif(NOT ROOT_RMySQL_LIBRARY)
    message(WARNING "MySQL ROOT class not found, the corresponding classes won't be built.")
else()
    add_definitions(-D_WITH_MYSQL)
endif()

o2_define_bucket(
        NAME
        o2_qualitycontrol_bucket

        DEPENDENCIES
        ${Boost_LOG_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        Common
        InfoLogger
        ${Configuration_LIBRARIES}
        DataSampling
        Monitoring
        DataFormat
        ${ROOT_LIBRARIES}
        ${Boost_LOG_DEBUG}

        SYSTEMINCLUDE_DIRECTORIES
        ${Boost_INCLUDE_DIRS}
        ${ROOT_INCLUDE_DIR}
        ${MYSQL_INCLUDE_DIRS}
        ${Configuration_INCLUDE_DIRS}
        ${ZEROMQ_INCLUDE_DIR}
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