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
///

#include "Common/ObjectComparatorDeviation.h"
// #include <DataFormatsQualityControl/FlagReasons.h>
//  ROOT
#include <TClass.h>
#include <TH1.h>

#include <fmt/core.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

//_________________________________________________________________________________________
//
// Compare an histogram with a reference one, using the selected comparison method
// Returns the quality flag given by the comparison function

static Quality compareTH1(TH1* hist, TH1* refHist, double threshold, bool normalize, std::string& message)
{
  if (!hist || !refHist) {
    return Quality::Null;
  }

  if (refHist->GetEntries() < 1) {
    message = "empty reference plot";
    return Quality::Null;
  }

  if (hist->GetNcells() < 3 || hist->GetNcells() != refHist->GetNcells()) {
    message = "wrong number of bins";
    return Quality::Null;
  }

  double scale = 1;
  if (normalize) {
    // the reference histogram is scaled to match the integral of the current histogram
    double integral = hist->Integral();
    double refIntegral = refHist->Integral();
    if (integral != 0 && refIntegral != 0) {
      scale = integral / refIntegral;
    }
  }

  double delta = 0;
  for (int bin = 1; bin < (hist->GetNcells() - 1); bin++) {
    double val = hist->GetBinContent(bin);
    double refVal = scale * refHist->GetBinContent(bin);
    delta += (refVal == 0) ? 0 : std::abs((val - refVal) / refVal);
  }
  delta /= hist->GetNcells() - 2;

  if (delta > threshold) {
    message = fmt::format("large deviation {:.2f} > {:.2f}", delta, threshold);
    return Quality::Bad;
  }
  return Quality::Good;
}

//_________________________________________________________________________________________

void ObjectComparatorDeviation::configure(const CustomParameters& customParameters, const Activity activity)
{
  auto parOpt = customParameters.atOptional("objCompDevThreshold", activity);
  if (!parOpt.has_value()) {
    parOpt = customParameters.atOptional("objCompDevThreshold");
  }
  if (parOpt.has_value()) {
    mThreshold = std::stof(parOpt.value());
  }

  parOpt = customParameters.atOptional("objCompDevNormalize", activity);
  if (!parOpt.has_value()) {
    parOpt = customParameters.atOptional("objCompDevNormalize");
  }
  if (parOpt.has_value()) {
    mNormalize = (std::stoi(parOpt.value()) == 1);
  }
}

//_________________________________________________________________________________________

Quality ObjectComparatorDeviation::compare(TObject* obj, TObject* objRef, std::string& message)
{
  if (!obj || !objRef) {
    message = "missing objects";
    return Quality::Null;
  }

  if (obj->IsA() != objRef->IsA()) {
    message = "incompatible objects";
    return Quality::Null;
  }

  // compare the objects if they inherit from TH1
  if (obj->InheritsFrom("TH1")) {
    return compareTH1(dynamic_cast<TH1*>(obj), dynamic_cast<TH1*>(objRef), mThreshold, mNormalize, message);
  }

  message = "objects are not TH1";
  return Quality::Null;
}

} // namespace o2::quality_control_modules::common
