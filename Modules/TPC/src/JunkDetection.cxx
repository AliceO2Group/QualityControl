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
/// \file   JunkDetection.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "Framework/ProcessingContext.h"
#include <Framework/InputRecord.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/JunkDetection.h"
#include "Common/Utils.h"

// root includes
#include "TCanvas.h"
#include "TObjArray.h"
#include "TH2F.h"
#include "TPaveText.h"

namespace o2::quality_control_modules::tpc
{

void JunkDetection::initialize(o2::framework::InitContext&)
{
  ILOG(Debug, Devel) << "initialize JunkDetection QC task" << ENDM;

  mIsMergeable = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "mergeableOutput");

  if (!mIsMergeable) {
    mJDCanv = std::make_unique<TCanvas>("c_junk_detection", "Junk Detection", 1000, 1000);
  }
  mJDHistos.emplace_back(new TH2F("h_removed_Strategy_A", "Removed Strategy (A)", 1, 0, 1, 1, 0, 1)); // dummy for the objectsManager
  mJDHistos.emplace_back(new TH2F("h_removed_Strategy_B", "Removed Strategy (B)", 1, 0, 1, 1, 0, 1)); // dummy for the objectsManager

  if (!mIsMergeable) {
    getObjectsManager()->startPublishing(mJDCanv.get());
  }
  for (const auto& hist : mJDHistos) {
    getObjectsManager()->startPublishing(hist);
  }
}

void JunkDetection::startOfActivity(const Activity&)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  if (!mIsMergeable) {
    mJDCanv->Clear();
  }
}

void JunkDetection::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void JunkDetection::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto jdHistos = ctx.inputs().get<TObjArray*>("JunkDetection");

  *mJDHistos[0] = *(TH2F*)jdHistos->At(4);
  *mJDHistos[1] = *(TH2F*)jdHistos->At(5);

  mJDHistos[0]->SetName("h_removed_Strategy_A");
  mJDHistos[1]->SetName("h_removed_Strategy_B");

  if (!mIsMergeable) {
    makeCanvas(jdHistos.get(), mJDCanv.get());
  }
}

void JunkDetection::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void JunkDetection::endOfActivity(const Activity&)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;

  if (!mIsMergeable) {
    getObjectsManager()->stopPublishing(mJDCanv.get());
  }
  for (auto& hist : mJDHistos) {
    getObjectsManager()->stopPublishing(hist);
    delete hist;
    hist = nullptr;
  }
}

void JunkDetection::reset()
{
  ILOG(Debug, Devel) << "Resetting the histogramss" << ENDM;

  if (!mIsMergeable) {
    mJDCanv->Clear();
  }
}

TCanvas* JunkDetection::makeCanvas(const TObjArray* data, TCanvas* outputCanvas)
{
  auto c = outputCanvas;
  if (!c) {
    c = new TCanvas("c_junk_detection", "Junk Detection", 1000, 1000);
  }

  c->Clear();

  auto strA = (TH2F*)data->At(4);
  auto strB = (TH2F*)data->At(5);

  double statsA[7];
  double statsB[7];

  strA->GetStats(statsA);
  strB->GetStats(statsB);

  auto junkDetectionMsg = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
  junkDetectionMsg->SetFillColor(0);
  junkDetectionMsg->SetBorderSize(0);
  junkDetectionMsg->AddText("Removal Strategy A");
  junkDetectionMsg->AddText(fmt::format("Number of Clusters before Removal: {}", statsA[2]).data());
  junkDetectionMsg->AddText(fmt::format("Removed Fraction: {:.2f}%", statsA[4]).data());
  junkDetectionMsg->AddLine(.0, .5, 1., .5);
  junkDetectionMsg->AddText("Removal Strategy B");
  junkDetectionMsg->AddText(fmt::format("Number of Clusters before Removal: {}", statsB[2]).data());
  junkDetectionMsg->AddText(fmt::format("Removed Fraction: {:.2f}%", statsB[4]).data());

  c->cd();
  junkDetectionMsg->SetBit(TObject::kCanDelete);
  junkDetectionMsg->Draw();

  return c;
}

} // namespace o2::quality_control_modules::tpc
