CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)

### CMP0025   Compiler id for Apple Clang is now AppleClang.
### CMP0042   MACOSX_RPATH is enabled by default.

FOREACH (p
        CMP0025 # CMake 3.0
        CMP0042 # CMake 3.0
        )
    IF (POLICY ${p})
        cmake_policy(SET ${p} NEW)
    ENDIF ()
endforeach ()

# Download the .clang-format from CodingGuidelines.
file(DOWNLOAD https://raw.githubusercontent.com/AliceO2Group/CodingGuidelines/master/.clang-format
     ${CMAKE_SOURCE_DIR}/.clang-format
     INACTIVITY_TIMEOUT 2
     TIMEOUT 2 STATUS status SHOW_PROGRESS)
if(NOT "${status}" EQUAL "0")
  MESSAGE(WARNING "Problem downloading .clang-format.")
ENDIF()

enable_testing()

add_subdirectory(Framework)
add_subdirectory(Modules)
add_subdirectory(doc)
