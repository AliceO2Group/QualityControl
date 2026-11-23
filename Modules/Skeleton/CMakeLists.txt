# ---- Library ----

add_library(O2QcSkeleton)

target_sources(O2QcSkeleton PRIVATE src/SkeletonTask.cxx src/SkeletonCheck.cxx src/SkeletonAggregator.cxx src/SkeletonPostProcessing.cxx src/SkeletonLateTask.cxx)

target_include_directories(
  O2QcSkeleton
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(O2QcSkeleton PUBLIC O2QualityControl)

install(TARGETS O2QcSkeleton
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Skeleton
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QualityControl")

# ---- ROOT dictionary ----

add_root_dictionary(O2QcSkeleton
                    HEADERS include/Skeleton/SkeletonTask.h
                            include/Skeleton/SkeletonCheck.h
                            include/Skeleton/SkeletonAggregator.h
                            include/Skeleton/SkeletonPostProcessing.h
                            include/Skeleton/SkeletonLateTask.h
                    LINKDEF "include/Skeleton/LinkDef.h")

# ---- Tests ----

add_executable(o2-qc-test-skeleton test/testQcSkeleton.cxx)
target_link_libraries(o2-qc-test-skeleton PRIVATE O2QcSkeleton O2::Catch2)
set_property(TARGET o2-qc-test-skeleton
             PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
add_test(NAME o2-qc-test-skeleton COMMAND o2-qc-test-skeleton)
set_tests_properties(o2-qc-test-skeleton PROPERTIES TIMEOUT 20)
