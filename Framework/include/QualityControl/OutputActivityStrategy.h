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

#ifndef QUALITYCONTROL_OUTPUTOBJECTVALIDITY_H
#define QUALITYCONTROL_OUTPUTOBJECTVALIDITY_H

namespace o2::quality_control::core
{

/// This enum lets user select how Activity (incl. validity) of output objects should be calculated
enum class OutputActivityStrategy {
  // Output activity contains all activities of input objects
  Integrated,
  // Output activity contains only the activity of the last input objects
  Last
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_OUTPUTOBJECTVALIDITY_H