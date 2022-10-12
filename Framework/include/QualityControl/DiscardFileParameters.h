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
/// @file    DiscardFileParameters.h
/// @author  Barthelemy von Haller
///

#ifndef QC_CORE_DISCARDFILEPARAMETERS_H
#define QC_CORE_DISCARDFILEPARAMETERS_H

#include <string>

namespace o2::quality_control::core
{

struct DiscardFileParameters {
  bool debug = false;
  int fromLevel = 21 /* Discard Trace */;
  std::string discardFile;
  unsigned long rotateMaxBytes = 0;
  unsigned int rotateMaxFiles = 0;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_DISCARDFILEPARAMETERS_H
