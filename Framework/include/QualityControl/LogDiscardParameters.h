// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// @file    LogDiscardParameters.h
/// @author  Barthelemy von Haller
///

#ifndef QC_CORE_DISCARDPARAMETERS_H
#define QC_CORE_DISCARDPARAMETERS_H

#include <string>

namespace o2::quality_control::core
{

struct LogDiscardParameters {
  bool debug = true;                // Discard debug messages
  int fromLevel = 21;               // Discard from this level up, default: Trace
  std::string file;                 // Discard to this file (if set) the messages whose level are equal or above `fromLevel`
  unsigned long rotateMaxBytes = 0; // Rotate the file to which we discard when it reaches this size
  unsigned int rotateMaxFiles = 0;  // Number of files we rotate over
  bool debugInDiscardFile = false;  // Discarded debug messages also go to the discard file
};

} // namespace o2::quality_control::core

#endif // QC_CORE_DISCARDPARAMETERS_H
