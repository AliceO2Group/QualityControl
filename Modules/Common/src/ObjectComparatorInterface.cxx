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
/// \file   ObjectComparatorInterface.cxx
/// \author Andrea Ferrero
/// \brief  An interface for comparing two TObject
///

#include "Common/ObjectComparatorInterface.h"
#include "Common/Utils.h"
#include "QualityControl/QcInfoLogger.h"
#include <CommonUtils/StringUtils.h>
//  ROOT
#include <TH1.h>

#include <fmt/core.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

std::string ObjectComparatorInterface::getParameterForPlot(const CustomParameters& customParameters, const std::string& parKey, const std::string& plotName, const Activity& activity)
{
  // get configuration parameter associated to the key
  std::string parValue;
  if (!plotName.empty()) {
    // first try to retrieve the plot-specific value, if existing
    auto parKeyForPlot = parKey + ":" + plotName;
    parValue = getFromExtendedConfig<std::string>(activity, customParameters, parKeyForPlot);
  }

  if (parValue.empty()) {
    // get the default value if the plot-specific one is not found
    parValue = getFromExtendedConfig<std::string>(activity, customParameters, parKey);
  }

  return parValue;
}

void ObjectComparatorInterface::configure(const CustomParameters& customParameters, const std::string& plotName, const Activity activity)
{
  // get configuration parameter associated to the key
  std::string parValue = getParameterForPlot(customParameters, std::string("threshold"), plotName, activity);

  if (parValue.empty()) {
    ILOG(Error, Support) << "Reference comparator threshold not found for plot \"" << plotName << "\"" << ENDM;
    return;
  }

  try {
    mThreshold = std::stod(parValue);
  } catch (std::exception& exception) {
    ILOG(Error, Support) << "Cannot convert reference comparator threshold for plot \"" << plotName << "\", string is " << parValue << ENDM;
    return;
  }

  // get optional axis ranges
  std::array<std::string, 2> axisNames{ "X", "Y" };
  for (int index = 0; index < 2; index++) {
    // get configuration parameter associated to the key
    std::string parValue = getParameterForPlot(customParameters, std::string("range") + axisNames[index], plotName, activity);

    if (parValue.empty()) {
      continue;
    }

    // extract min and max values of the range
    auto rangeMinMax = o2::utils::Str::tokenize(parValue, ',', false, true);
    if (rangeMinMax.size() != 2) {
      ILOG(Error, Support) << "Cannot parse values for " << axisNames[index] << " axis range of plot \"" << plotName << "\", string is " << parValue << ENDM;
      return;
    }

    try {
      mRanges[index] = std::make_pair<double, double>(std::stod(rangeMinMax[0]), std::stod(rangeMinMax[1]));
    } catch (std::exception& exception) {
      ILOG(Error, Support) << "Cannot convert values from string to double for " << axisNames[index] << " axis range of plot \"" << plotName << "\", string is " << parValue << ENDM;
    }
  }
}

std::tuple<TH1*, TH1*, bool> ObjectComparatorInterface::checkInputObjects(TObject* object, TObject* referenceObject, std::string& message)
{
  TH1* histogram = nullptr;
  TH1* referenceHistogram = nullptr;

  if (!object || !referenceObject) {
    message = "missing objects";
    return std::make_tuple(histogram, referenceHistogram, false);
  }

  // only consider objects that inherit from TH1
  histogram = dynamic_cast<TH1*>(object);
  referenceHistogram = dynamic_cast<TH1*>(referenceObject);

  if (!histogram || !referenceHistogram) {
    message = "objects are not TH1";
    return std::make_tuple(histogram, referenceHistogram, false);
  }

  // the object and the reference must correspond to the same ROOT class
  if (histogram->IsA() != referenceHistogram->IsA()) {
    message = "incompatible objects";
    return std::make_tuple(histogram, referenceHistogram, false);
  }

  if (referenceHistogram->GetEntries() < 1) {
    message = "empty reference plot";
    return std::make_tuple(histogram, referenceHistogram, false);
  }

  if (histogram->GetNcells() < 3 || histogram->GetNcells() != referenceHistogram->GetNcells()) {
    message = "incompatible number of bins";
    return std::make_tuple(histogram, referenceHistogram, false);
  }

  return std::make_tuple(histogram, referenceHistogram, true);
}

} // namespace o2::quality_control_modules::common
