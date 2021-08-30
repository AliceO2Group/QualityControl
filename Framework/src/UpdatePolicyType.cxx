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
/// \file   UpdatePolicyType.cxx
/// \author Piotr Konopka
///

#include "QualityControl/UpdatePolicyType.h"

#include <unordered_map>

namespace o2::quality_control::checker
{

UpdatePolicyType UpdatePolicyTypeUtils::FromString(const std::string& str)
{
  static std::unordered_map<std::string, UpdatePolicyType> const updatePolicyTypeFromString = {
    { "OnAny", UpdatePolicyType::OnAny },
    { "OnAnyNonZero", UpdatePolicyType::OnAnyNonZero },
    { "OnAll", UpdatePolicyType::OnAll },
    { "OnEachSeparately", UpdatePolicyType::OnEachSeparately },
    { "OnGlobalAny", UpdatePolicyType::OnGlobalAny }
  };

  return updatePolicyTypeFromString.at(str);
}

std::string UpdatePolicyTypeUtils::ToString(UpdatePolicyType policyType)
{
  static std::unordered_map<UpdatePolicyType, std::string> const stringFromUpdatePolicyType = {
    { UpdatePolicyType::OnAny, "OnAny" },
    { UpdatePolicyType::OnAnyNonZero, "OnAnyNonZero" },
    { UpdatePolicyType::OnAll, "OnAll" },
    { UpdatePolicyType::OnEachSeparately, "OnEachSeparately" },
    { UpdatePolicyType::OnGlobalAny, "OnGlobalAny" }
  };
  return stringFromUpdatePolicyType.at(policyType);
}

} // namespace o2::quality_control::checker