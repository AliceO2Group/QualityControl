# @author  Barthélémy von Haller

# Compile C++ tests (here only one, but it will grab whatever cpp is in the test directory)
file(GLOB_RECURSE TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

# Iterate over all tests found. For each, declare an executable and add it
# to the tests list.
set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
    string(REGEX REPLACE "[./]" "_" NAME ${FILE})
    source_group(\\ FILES ${FILE})
    add_executable(${NAME} ${FILE})
    target_link_libraries(${NAME} hello ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    get_target_property(EXECUTABLE ${NAME} LOCATION)
    STRING(REGEX REPLACE "\\$\\(.*\\)" "\${CTEST_CONFIGURATION_TYPE}"
      EXECUTABLE "${EXECUTABLE}")
    add_test(${NAME} ${EXECUTABLE})                              # <-- this is how to add tests to CMake
    set_target_properties(${NAME} PROPERTIES FOLDER "Tests")
    list(APPEND ALL_TESTS ${NAME})
endforeach(FILE ${TEST_FILES})

# Add extra test
add_test(opt_exists /usr/bin/test -d /opt)

include(CTest)
