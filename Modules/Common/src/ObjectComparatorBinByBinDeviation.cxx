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
/// \file   ObjectComparatorBinByBinDeviation.cxx
/// \author Andrea Ferrero
/// \brief  A class for comparing two histogram objects based on the average of the relative deviation between the bins
///

#include "Common/ObjectComparatorBinByBinDeviation.h"
#include "Common/Utils.h"
#include "QualityControl/QcInfoLogger.h"
//  ROOT
#include <TH1.h>

#include <fmt/core.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

void ObjectComparatorBinByBinDeviation::configure(const CustomParameters& customParameters, const std::string& plotName, const Activity activity)
{
  ObjectComparatorInterface::configure(customParameters, plotName, activity);

  // get configuration parameter associated to the key
  std::string parValue = getParameterForPlot(customParameters, std::string("maxAllowedBadBins"), plotName, activity);
  if (parValue.empty()) {
    return;
  }

  try {
    mMaxAllowedBadBins = std::stoi(parValue);
  } catch (std::exception& exception) {
    ILOG(Error, Support) << "Cannot convert maxAllowedBadBins for plot \"" << plotName << "\", string is " << parValue << ENDM;
    return;
  }
}

Quality ObjectComparatorBinByBinDeviation::compare(TObject* object, TObject* referenceObject, std::string& message)
{
  auto checkResult = checkInputObjects(object, referenceObject, message);
  if (!std::get<2>(checkResult)) {
    return Quality::Null;
  }

  auto* histogram = std::get<0>(checkResult);
  auto* referenceHistogram = std::get<1>(checkResult);

  int binRangeX[2] = { 1, histogram->GetXaxis()->GetNbins() };
  if (getXRange().has_value()) {
    binRangeX[0] = histogram->GetXaxis()->FindBin(getXRange()->first);
    binRangeX[1] = histogram->GetXaxis()->FindBin(getXRange()->second);
  }

  int binRangeY[2] = { 1, histogram->GetYaxis()->GetNbins() };
  if (getYRange().has_value()) {
    binRangeY[0] = histogram->GetYaxis()->FindBin(getYRange()->first);
    binRangeY[1] = histogram->GetYaxis()->FindBin(getYRange()->second);
  }

  int binRangeZ[2] = { 1, histogram->GetZaxis()->GetNbins() };

  // compute the average relative deviation between the bins
  double averageDeviation = 0;
  int numberOfBadBins = 0;
  for (int binX = binRangeX[0]; binX <= binRangeX[1]; binX++) {
    for (int binY = binRangeY[0]; binY <= binRangeY[1]; binY++) {
      for (int binZ = binRangeZ[0]; binZ <= binRangeZ[1]; binZ++) {
        int bin = histogram->GetBin(binX, binY, binZ);
        double val = histogram->GetBinContent(bin);
        double refVal = referenceHistogram->GetBinContent(bin);
        double deviation = (refVal == 0) ? 0 : std::abs((val - refVal) / refVal);
        if (deviation > getThreshold()) {
          numberOfBadBins += 1;
        }
      }
    }
  }

  // compare the average deviation with the maximum allowed value
  if (numberOfBadBins > mMaxAllowedBadBins) {
    message = fmt::format("bins above {:.2f}: {} > {}", getThreshold(), numberOfBadBins, mMaxAllowedBadBins);
    return Quality::Bad;
  }

  return Quality::Good;
}

} // namespace o2::quality_control_modules::common
