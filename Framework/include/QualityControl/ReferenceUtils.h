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
/// \file   ReferenceComparatorUtils.h
/// \author Andrea Ferrero and Barthelemy von Haller
///

#ifndef QUALITYCONTROL_ReferenceUtils_H
#define QUALITYCONTROL_ReferenceUtils_H

#include <memory>
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Activity.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RepoPathUtils.h"

#include <TH1.h>
#include <TCanvas.h>

namespace o2::quality_control::checker
{

//_________________________________________________________________________________________
//
// Get the reference plot for a given MonitorObject path

static std::shared_ptr<quality_control::core::MonitorObject> getReferencePlot(quality_control::repository::DatabaseInterface* qcdb, std::string& fullPath,
                                                                              core::Activity referenceActivity)
{
  auto [success, path, name] = o2::quality_control::core::RepoPathUtils::splitObjectPath(fullPath);
  if (!success) {
    return nullptr;
  }
  return qcdb->retrieveMO(path, name, repository::DatabaseInterface::Timestamp::Latest, referenceActivity);
}

//_________________________________________________________________________________________
//
// Get the current and reference histograms from the container canvas.
// The two histograms are returned as a std::pair

static std::pair<TH1*, TH1*> getPlotsFromCanvas(TCanvas* canvas, std::string& message)
{
  // Get the pad containing the current histogram, as well as the reference one in the case of 1-D plots
  TPad* padHist = dynamic_cast<TPad*>(canvas->GetPrimitive(TString::Format("%s_PadHist", canvas->GetName())));
  if (!padHist) {
    message = "missing PadHist";
    return { nullptr, nullptr };
  }
  // Get the pad containing the reference histogram.
  // This pad is only present for 2-D histograms.
  // 1-D histograms are drawn superimposed in the same pad
  TPad* padHistRef = (TPad*)canvas->GetPrimitive(TString::Format("%s_PadHistRef", canvas->GetName()));

  // Get the current histogram
  TH1* hist = dynamic_cast<TH1*>(padHist->GetPrimitive(TString::Format("%s_hist", canvas->GetName())));
  if (!hist) {
    message = "missing histogram";
    return { nullptr, nullptr };
  }

  // Get the reference histogram, trying both pads
  TH1* histRef = nullptr;
  if (padHistRef) {
    histRef = dynamic_cast<TH1*>(padHistRef->GetPrimitive(TString::Format("%s_hist_ref", canvas->GetName())));
  } else {
    histRef = dynamic_cast<TH1*>(padHist->GetPrimitive(TString::Format("%s_hist_ref", canvas->GetName())));
  }

  if (!histRef) {
    message = "missing reference histogram";
    return { nullptr, nullptr };
  }

  // return a pair with the two histograms
  return { hist, histRef };
}

//_________________________________________________________________________________________
//
// Get the ratio histogram from the container canvas

static TH1* getRatioPlotFromCanvas(TCanvas* canvas)
{
  // Get the pad containing the current histogram, as well as the reference one in the case of 1-D plots
  TPad* padHistRatio = dynamic_cast<TPad*>(canvas->GetPrimitive(TString::Format("%s_PadHistRatio", canvas->GetName())));
  if (!padHistRatio) {
    return nullptr;
  }

  // Get the current histogram
  TH1* histRatio = dynamic_cast<TH1*>(padHistRatio->GetPrimitive(TString::Format("%s_hist_ratio", canvas->GetName())));
  if (!histRatio) {
    return nullptr;
  }

  // return a pair with the two histograms
  return histRatio;
}

} // namespace o2::quality_control::checker

#endif // QUALITYCONTROL_ReferenceUtils_H
