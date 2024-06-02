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

Quality ObjectComparatorDeviation::compare(TObject* obj, TObject* objRef, std::string& message)
{
  if (!obj || !objRef) {
    message = "missing objects";
    return Quality::Null;
  }

  // only consider objects that inherit from TH1
  auto* hist = dynamic_cast<TH1*>(obj);
  auto* histRef = dynamic_cast<TH1*>(objRef);

  if (!hist || !histRef) {
    message = "objects are not TH1";
    return Quality::Null;
  }

  // the object and the reference must correspond to the same ROOT class
  if (hist->IsA() != histRef->IsA()) {
    message = "incompatible objects";
    return Quality::Null;
  }

  if (histRef->GetEntries() < 1) {
    message = "empty reference plot";
    return Quality::Null;
  }

  if (hist->GetNcells() < 3 || hist->GetNcells() != histRef->GetNcells()) {
    message = "incompatible number of bins";
    return Quality::Null;
  }

  // compute the average relative deviation between the bins
  double averageDeviation = 0;
  int numberOfBins = hist->GetNcells() - 2;
  for (int bin = 1; bin <= numberOfBins; bin++) {
    double val = hist->GetBinContent(bin);
    double refVal = histRef->GetBinContent(bin);
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
