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
/// \file   ReferenceComparatorCheck.cxx
/// \author Andrea Ferrero
///

#include "Common/ReferenceComparatorCheck.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ActivityHelpers.h"
// ROOT
#include <TH1.h>
#include <TLine.h>

//#include <DataFormatsQualityControl/FlagReasons.h>

//using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

template <>
std::string ReferenceComparatorCheck::getParameter(std::string parName, const std::string defaultValue, const o2::quality_control::core::Activity& activity)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

template <>
std::string ReferenceComparatorCheck::getParameter(std::string parName, const std::string defaultValue)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}


void ReferenceComparatorCheck::configure()
{
}


void ReferenceComparatorCheck::startOfActivity(const Activity& activity)
{
  auto compMethodStr = getParameter<std::string>("method", "Deviation", activity);
  if (compMethodStr == "Deviation") {
    mPlotCompMethod = MOCompDeviation;
  }
  if (compMethodStr == "Chi2") {
    mPlotCompMethod = MOCompChi2;
  }
  mCompThreshold = getParameter<double>("threshold", 0.1, activity);
}


void ReferenceComparatorCheck::endOfActivity(const Activity& activity)
{
}

//_________________________________________________________________________________________
//
// Get the THxyRatio histogram from the MO, and compare the current histogram (numerator) with
// the reference histogram (denominator), using the comparison method passed as parameter

static Quality compare(std::shared_ptr<MonitorObject> mo, MOCompMethod method, double threshold)
{
  if (!mo) {
    return Quality::Null;
  }

  auto object = mo->getObject();
  if (!object) {
    return Quality::Null;
  }

  // check if the MO contains an histogram derived from THxRatio
  if (auto histRatio = dynamic_cast<TH1FRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  } else if (auto histRatio = dynamic_cast<TH1DRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  } else if (auto histRatio = dynamic_cast<TH2FRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  } else if (auto histRatio = dynamic_cast<TH2DRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  }

  return Quality::Null;
}

//_________________________________________________________________________________________
//
// Loop over all the input MOs and compare each of them with the corresponding MO from the reference run
// The final quality is the combination of the individual values stored in the mQualityFlags map

Quality ReferenceComparatorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [key, mo] : *moMap) {

    auto moName = mo->getName();
    mQualityFlags[moName] = compare(mo, mPlotCompMethod, 0.1);
  }

  for (auto& [moName, q] : mQualityFlags) {
    if (q == Quality::Null) {
      continue;
    }
    if (result == Quality::Null || q.isWorseThan(result)) {
      result = q;
    }
  }
  return result;
}

void ReferenceComparatorCheck::reset()
{
  mQualityFlags.clear();
}

std::string ReferenceComparatorCheck::getAcceptedType() { return "TH1"; }

void ReferenceComparatorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TH1* hist = dynamic_cast<TH1*>(mo->getObject());
  if (!hist) {
    return;
  }

  hist->SetMinimum(0);
  hist->SetMaximum(2);

  auto moName = mo->getName();
  auto quality = mQualityFlags[moName];
  if (hist->InheritsFrom("TH1") && !hist->InheritsFrom("TH2")) {
    // this is a 1-D histogram, we can draw lines and change the color
    if (quality == Quality::Bad) {
      hist->SetLineColor(kRed);
      hist->SetMarkerColor(kRed);
    }
    if (mPlotCompMethod == MOCompDeviation) {
      double xmin = hist->GetXaxis()->GetXmin();
      double xmax = hist->GetXaxis()->GetXmax();
      TLine* l1 = new TLine(xmin, 1.0 - mCompThreshold, xmax, 1.0 - mCompThreshold);
      l1->SetLineColor(kBlue);
      l1->SetLineStyle(kDotted);
      hist->GetListOfFunctions()->Add(l1);

      TLine* l2 = new TLine(xmin, 1.0 + mCompThreshold, xmax, 1.0 + mCompThreshold);
      l2->SetLineColor(kBlue);
      l2->SetLineStyle(kDotted);
      hist->GetListOfFunctions()->Add(l2);
    }
  }
}

} // namespace o2::quality_control_modules::skeleton
