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
/// \file   UserCodeCardinality.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_USERCODECARDINALITY_H
#define QUALITYCONTROL_USERCODECARDINALITY_H

namespace o2::quality_control::core
{

// Indicates whether an Actor runs none, one or multiple user tasks/checks/aggregators/...
enum class UserCodeCardinality {
  None = 0,
  One = 1,
  Many = 2
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_USERCODECARDINALITY_H