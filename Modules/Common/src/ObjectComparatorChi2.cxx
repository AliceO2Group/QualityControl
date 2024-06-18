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
/// \brief  A class for comparing two histogram objects based on a chi2 test
///

#include "Common/ObjectComparatorChi2.h"
//  ROOT
#include <TH1.h>

#include <fmt/core.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

Quality ObjectComparatorChi2::compare(TObject* object, TObject* referenceObject, std::string& message)
{
  auto checkResult = checkInputObjects(object, referenceObject, message);
  if (!std::get<2>(checkResult)) {
    return Quality::Null;
  }

  auto* histogram = std::get<0>(checkResult);
  auto* referenceHistogram = std::get<1>(checkResult);

  // perform a chi2 compatibility test between the two histograms
  // it assumes that both histigrams represent counts, but the reference might
  // have been rescaled to match the integral of the current histogram
  double testProbability = histogram->Chi2Test(referenceHistogram, "UU NORM");

  // compare the chi2 probability with the minimum allowed value
  if (testProbability < getThreshold()) {
    message = fmt::format("chi2 test failed: {:.2f} < {:.2f}", testProbability, getThreshold());
    return Quality::Bad;
  }

  return Quality::Good;
}

} // namespace o2::quality_control_modules::common
