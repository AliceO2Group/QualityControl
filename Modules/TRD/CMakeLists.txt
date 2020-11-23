# ---- Library ----

add_library(QcTRD)

target_sources(QcTRD PRIVATE src/TRDDigitQcTask.cxx)

target_include_directories(
  QcTRD
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
					${CMAKE_CURRENT_SOURCE_DIR}/src
					${O2_ROOT}/include/GPU)

target_link_libraries(QcTRD
											PUBLIC
												QualityControl)


add_root_dictionary(QcTRD
                    HEADERS include/TRD/TRDDigitQcTask.h
                    LINKDEF include/TRD/LinkDef.h
                    BASENAME QcTRD)

# ---- Test(s) ----

set(TEST_SRCS test/testQcTRD.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PUBLIC QcTRD
                        PRIVATE QcTRD Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  #set_property(TARGET ${test_name}

  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()


# ---- Executables ----

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QcTRD ROOT::Tree)
endforeach()

# ---- Install ----

install(TARGETS QcTRD ${EXE_NAMES}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Install config files ----

install(FILES TRDDigitQcTask.json
              DESTINATION etc)
