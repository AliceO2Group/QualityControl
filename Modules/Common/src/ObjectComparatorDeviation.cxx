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
/// \file   ObjectComparatorDeviation.cxx
/// \author Andrea Ferrero
/// \brief  A class for comparing two histogram objects based on the average of the relative deviation between the bins
///

#include "Common/ObjectComparatorDeviation.h"
//  ROOT
#include <TH1.h>

#include <fmt/core.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

Quality ObjectComparatorDeviation::compare(TObject* object, TObject* referenceObject, std::string& message)
{
  auto checkResult = checkInputObjects(object, referenceObject, message);
  if (!std::get<2>(checkResult)) {
    return Quality::Null;
  }

  auto* histogram = std::get<0>(checkResult);
  auto* referenceHistogram = std::get<1>(checkResult);

  // compute the average relative deviation between the bins
  double averageDeviation = 0;
  int numberOfBins = histogram->GetNcells() - 2;
  for (int bin = 1; bin <= numberOfBins; bin++) {
    double val = histogram->GetBinContent(bin);
    double refVal = referenceHistogram->GetBinContent(bin);
    averageDeviation += (refVal == 0) ? 0 : std::abs((val - refVal) / refVal);
  }
  averageDeviation /= numberOfBins;

  // compare the average deviation with the maximum allowed value
  if (averageDeviation > getThreshold()) {
    message = fmt::format("large deviation: {:.2f} > {:.2f}", averageDeviation, getThreshold());
    return Quality::Bad;
  }

  return Quality::Good;
}

} // namespace o2::quality_control_modules::common
