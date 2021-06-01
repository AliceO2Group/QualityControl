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
# o2_generate_unique_port(VAR_NAME) generates a random TCP/UDP port number in
# the range 30000 - 59999 and puts it under the name specified in the first argument.

function(o2_generate_unique_port VAR_NAME)

  string(RANDOM LENGTH 1 ALPHABET 345 FIRST_DIGIT)
  string(RANDOM LENGTH 4 ALPHABET 0123456789 OTHER_DIGITS)
  string(CONCAT ${VAR_NAME} ${FIRST_DIGIT} ${OTHER_DIGITS})

  set(${VAR_NAME} ${${VAR_NAME}} PARENT_SCOPE)

endfunction()