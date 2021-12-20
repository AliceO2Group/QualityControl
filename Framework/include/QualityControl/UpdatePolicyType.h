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

#ifndef QUALITYCONTROL_UPDATEPOLICYTYPE_H
#define QUALITYCONTROL_UPDATEPOLICYTYPE_H

///
/// \file   UpdatePolicyType.h
/// \author Piotr Konopka
///

#include <string>

namespace o2::quality_control::checker
{

enum class UpdatePolicyType {
  OnAny,
  OnAnyNonZero,
  OnAll,
  OnEachSeparately,
  OnGlobalAny
};

struct UpdatePolicyTypeUtils {
  static UpdatePolicyType FromString(const std::string&);
  static std::string ToString(UpdatePolicyType);
};

} // namespace o2::quality_control::checker
#endif //QUALITYCONTROL_UPDATEPOLICYTYPE_H
