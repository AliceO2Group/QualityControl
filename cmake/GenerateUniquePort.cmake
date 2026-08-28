# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

include_guard()


#
# o2_generate_unique_port(VAR_NAME) generates a random even TCP/UDP port number
# in the range 20000 - 29998 and puts it under the name specified in the first
# argument. Below the ephemeral range (32768+ on Linux, 49152+ on macOS), so
# kernel-assigned source ports cannot collide with it; even, so callers may use
# port+1 as a companion.

function(o2_generate_unique_port VAR_NAME)

  string(RANDOM LENGTH 3 ALPHABET 0123456789 OTHER_DIGITS)
  string(RANDOM LENGTH 1 ALPHABET 02468 LAST_DIGIT)
  string(CONCAT ${VAR_NAME} 2 ${OTHER_DIGITS} ${LAST_DIGIT})

  set(${VAR_NAME} ${${VAR_NAME}} PARENT_SCOPE)

endfunction()
