# see first if we have a modern O2 available
find_package(O2 CONFIG)
if(O2_FOUND)
  message(
    STATUS
      "Good news. We are using the modern cmake version of our O2 dependency")
  if(NOT COMMAND add_root_dictionary)
    message(
      FATAL_ERROR "Bad news though: missing add_root_dictionary function !")
  endif()
  set(AliceO2_FOUND True)
  return()
endif()

include(AddRootDictionary.cmake)

# FIXME: temporary hand-crafter imported targets from O2 this piece of code will
# disappear as soon as O2 gets its new cmake system in place (and thus will
# define correctly those targets)

# * O2::Framework
# * O2::CCDB

message(
  WARNING "Created ourselves the O2 targets. That should be only temporary")

set(AliceO2_FOUND True)
