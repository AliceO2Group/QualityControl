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
/// \file   Criticality.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_CRITICALITY_H
#define QUALITYCONTROL_CRITICALITY_H

namespace o2::quality_control::core
{

/// \brief Defines how a Control system should react to task failures for a concrete Actor
enum class Criticality {
  // If a critical task goes to ERROR or crashes, it brings the computing node to ERROR.
  // If a node is critical (e.g. an FLP or a QC node workflow), that implies stopping a data-taking run or grid job
  // If a node is non-critical (e.g. an EPN), this implies dropping that node from data taking or grid job.
  // A critical task can only depend on outputs of other critical tasks, otherwise it's a DPL workflow error.
  Critical,
  // When an expendable (non-critical) task goes to ERROR or crashes, it does NOT bring the computing node to ERROR.
  Expendable,
  // A resilient task brings down the computing node upon ERROR or crash, but it can survive a failure
  // of an upstream expendable task.
  Resilient,
  // The decision on criticality is delegated to user, but we take care of critical/resilient distinction.
  UserDefined
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_CRITICALITY_H