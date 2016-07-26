# - Find mysqlclient
# Find the native MySQL includes and library
#
#  MYSQL_INCLUDE_DIRS - where to find mysql.h, etc.
#  MYSQL_LIBRARIES    - List of libraries when using MySQL.
#  MYSQL_FOUND        - True if MySQL found.

#  MYSQL_PATH_SUFFIXES  - by default it includes "mysql". Set it if you want to force extra suffixes to be used.

if(NOT MYSQL_PATH_SUFFIXES)
    set(MYSQL_PATH_SUFFIXES "" "mysql") 
endif()

FIND_PATH(MYSQL_INCLUDE_DIRS mysql.h
  /usr/local/include/mysql
  /usr/include/mysql
)

SET(MYSQL_NAMES mysqlclient mysqlclient_r)
FIND_LIBRARY(MYSQL_LIBRARY
  NAMES ${MYSQL_NAMES}
  PATHS /usr/lib /usr/lib64 /usr/local/lib 
  PATH_SUFFIXES ${MYSQL_PATH_SUFFIXES}
)

IF (MYSQL_INCLUDE_DIRS AND MYSQL_LIBRARY)
  SET(MYSQL_FOUND TRUE)
  SET( MYSQL_LIBRARIES ${MYSQL_LIBRARY} )
ELSE (MYSQL_INCLUDE_DIRS AND MYSQL_LIBRARY)
  SET(MYSQL_FOUND FALSE)
  UNSET(MYSQL_LIBRARIES CACHE)
  UNSET(MYSQL_INCLUDE_DIRS CACHE)
ENDIF (MYSQL_INCLUDE_DIRS AND MYSQL_LIBRARY)

IF (MYSQL_FOUND)
    MESSAGE(STATUS "Found MySQL: ${MYSQL_LIBRARY}")
ELSE (MYSQL_FOUND)
  IF (MYSQL_FIND_REQUIRED)
    MESSAGE(STATUS "Looked for MySQL libraries named ${MYSQL_NAMES}.")
    MESSAGE(FATAL_ERROR "Could NOT find MySQL library")
  ENDIF (MYSQL_FIND_REQUIRED)
ENDIF (MYSQL_FOUND)

MARK_AS_ADVANCED(
  MYSQL_LIBRARY
  MYSQL_INCLUDE_DIRS
)
