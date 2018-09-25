CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)

# Set cmake policy by version: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.12)
endif()

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(Framework)
#add_subdirectory(Modules)
#add_subdirectory(doc)
