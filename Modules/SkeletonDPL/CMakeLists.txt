set(MODULE_NAME "QcSkeletonDpl")

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
        src/SkeletonTaskDPL.cxx
#        src/_SkeletonCheck.cxx
        )

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(HEADERS
        include/SkeletonDPL/SkeletonTaskDPL.h
#        include/SkeletonDPL/_SkeletonCheck.h
        )

set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME o2_qcmodules_dpl)
set(LINKDEF include/SkeletonDPL/LinkDef.h)

O2_GENERATE_LIBRARY()

set(TEST_SRCS
        test/testQcSkeletonDPL.cxx
        )

O2_GENERATE_TESTS(
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
        TEST_SRCS ${TEST_SRCS}
)
