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
/// \file   SkeletonCheck.cxx
/// \author My Name
///

#include "Skeleton/SkeletonAggregator.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/QCInputsAdapters.h"

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::skeleton
{

void SkeletonAggregator::configure()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  // This method is called whenever CustomParameters are set.

  // Example of retrieving a custom parameter
  std::string parameter = mCustomParameters.atOrDefaultValue("myOwnKey", "fallback value");
}

std::map<std::string, Quality> SkeletonAggregator::aggregate(const o2::quality_control::core::QCInputs& data)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  std::map<std::string, Quality> result;

  ILOG(Info, Devel) << "Entered SkeletonAggregator::aggregate" << ENDM;
  ILOG(Info, Devel) << "   received a data of size : " << data.size() << ENDM;

  // we return the worse quality of all the objects we receive
  Quality current = Quality::Good;
  for (const auto& qo : iterateQualityObjects(data)) {
    ILOG(Info, Devel) << "Object: " << qo << ENDM;
    if (qo.getQuality().isWorseThan(current)) {
      current = qo.getQuality();
    }
  }

  ILOG(Info, Devel) << "   result: " << current << ENDM;
  result["newQuality"] = current;

  // add one more
  Quality plus = Quality::Medium;
  result["another"] = plus;

  return result;
}
} // namespace o2::quality_control_modules::skeleton
