# ---- Library ----

add_library(QcITSRawTask src/ITSRawTask.cxx)

target_sources(QcITSRawTask PRIVATE)

target_include_directories(
  QcITSRawTask
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${O2_ROOT}/include/GPU)

target_link_libraries(QcITSRawTask PUBLIC QualityControl O2::ITSBase O2::ITSMFTBase O2::ITSMFTReconstruction)

install(TARGETS QcITSRawTask
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- ROOT dictionary ----

add_root_dictionary(QcITSRawTask
                    HEADERS include/ITS/ITSRawTask.h
                    LINKDEF include/ITS/LinkDef.h
                    BASENAME QcITSRawTask)

# ---- Test(s) ----

add_executable(testQcITS test/testITS.cxx)
target_link_libraries(testQcITS PRIVATE QcITSRawTask Boost::unit_test_framework O2::ITSMFTReconstruction O2::ITSBase O2::DetectorsBase QualityControl)
add_test(NAME testQcITS COMMAND testQcITS)
set_tests_properties(testQcITS PROPERTIES TIMEOUT 60)
install(
  TARGETS testQcITS
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Executables ----

set(EXE_SRCS src/runITS.cxx)

set(EXE_NAMES o2-qc-run-its)

# These were the original names before the convention changed. We will get rid
# of them but for the time being we want to create symlinks to avoid confusion.
set(EXE_OLD_NAMES qcRunITS)

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  list(GET EXE_OLD_NAMES ${i} oldname)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QualityControl CURL::libcurl O2::ITSQCDataReaderWorkflow O2::DetectorsBase)
  install_symlink(${name} ${CMAKE_INSTALL_FULL_BINDIR}/${oldname})
endforeach()

install(
  TARGETS ${EXE_NAMES}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Extra scripts ----

install(FILES its.json DESTINATION etc)

get_property(dirs
             DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
