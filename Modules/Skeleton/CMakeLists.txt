set(MODULE_NAME "QcSkeleton")

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
        src/SkeletonTask.cxx
        src/SkeletonCheck.cxx
        )

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(HEADERS
        include/Skeleton/SkeletonTask.h
        include/Skeleton/SkeletonCheck.h
        )

set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME o2_qcmodules_base)
set(LINKDEF include/Skeleton/LinkDef.h)

O2_GENERATE_LIBRARY()

set(TEST_SRCS
        test/testQcSkeleton.cxx
        )

O2_GENERATE_TESTS(
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
        TEST_SRCS ${TEST_SRCS}
)
