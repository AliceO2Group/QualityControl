set(MODULE_NAME "QcCommon")

O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
  src/NonEmpty.cxx
  src/MeanIsAbove.cxx
)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

set(HEADERS
  include/Common/NonEmpty.h
  include/Common/MeanIsAbove.h
  )

set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME o2_qcmodules_common)
set(LINKDEF include/Common/LinkDef.h)

O2_GENERATE_LIBRARY()

set(TEST_SRCS
  test/testMeanIsAbove.cxx
  test/testNonEmpty.cxx
)

O2_GENERATE_TESTS(
 MODULE_LIBRARY_NAME ${LIBRARY_NAME}
 BUCKET_NAME ${BUCKET_NAME}
 TEST_SRCS ${TEST_SRCS}
)
