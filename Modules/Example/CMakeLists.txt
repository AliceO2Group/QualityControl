set(MODULE_NAME "QcExample")

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
        src/BenchmarkTask.cxx
        src/ExampleTask.cxx
        src/FakeCheck.cxx
        )

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(HEADERS
        include/Example/BenchmarkTask.h
        include/Example/ExampleTask.h
        include/Example/FakeCheck.h
        )

set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME o2_qcmodules_example)
set(LINKDEF include/Example/LinkDef.h)

O2_GENERATE_LIBRARY()

set(TEST_SRCS
        test/testFactory.cxx
        test/testQcExample.cxx
        )

O2_GENERATE_TESTS(
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
        TEST_SRCS ${TEST_SRCS}
)
