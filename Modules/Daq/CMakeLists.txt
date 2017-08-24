set(MODULE_NAME "QcDaq")

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
        src/DaqTask.cxx
        src/EverIncreasingGraph.cxx
        )

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(HEADERS
        include/Daq/DaqTask.h
        include/Daq/EverIncreasingGraph.h
        )

set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME o2_qcmodules_base)
set(LINKDEF include/Daq/LinkDef.h)

O2_GENERATE_LIBRARY()

set(TEST_SRCS
        test/testQcDaq.cxx
        )

O2_GENERATE_TESTS(
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
        TEST_SRCS ${TEST_SRCS}
)
