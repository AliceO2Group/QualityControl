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
/// \file   ObjectComparatorChi2.cxx
/// \author Andrea Ferrero
///

#include "Common/ObjectComparatorChi2.h"
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

  std::string chi2TestOption = "UU P";
  if (normalize) {
    // the reference histogram is scaled to match the integral of the current histogram
    double integral = hist->Integral();
    double refIntegral = refHist->Integral();
    if (integral != 0 && refIntegral != 0) {
      refHist->Scale(integral / refIntegral);
      chi2TestOption = "UW P";
    }
  }

  double chi2Prob = hist->Chi2Test(refHist, chi2TestOption.c_str());

  if (chi2Prob < threshold) {
    message = fmt::format("chi2 test failed {:.2f} < {:.2f}", chi2Prob, threshold);
    return Quality::Bad;
  }
  return Quality::Good;
}

//_________________________________________________________________________________________

void ObjectComparatorChi2::configure(const CustomParameters& customParameters, const Activity activity)
{
  auto parOpt = customParameters.atOptional("objCompChi2Threshold", activity);
  if (!parOpt.has_value()) {
    parOpt = customParameters.atOptional("objCompChi2Threshold");
  }
  if (parOpt.has_value()) {
    mThreshold = std::stof(parOpt.value());
  }

  parOpt = customParameters.atOptional("objCompChi2Normalize", activity);
  if (!parOpt.has_value()) {
    parOpt = customParameters.atOptional("objCompChi2Normalize");
  }
  if (parOpt.has_value()) {
    mNormalize = (std::stoi(parOpt.value()) == 1);
  }
}

//_________________________________________________________________________________________

Quality ObjectComparatorChi2::compare(TObject* obj, TObject* objRef, std::string& message)
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
