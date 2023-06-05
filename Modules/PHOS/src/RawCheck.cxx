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
/// \file   RawCheck.cxx
/// \author Dmitri Peresunko
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>
#include <TLatex.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <fairlogger/Logger.h>
#include "PHOS/RawCheck.h"
#include "PHOS/TH1Fraction.h"

#include <DataFormatsQualityControl/FlagReasons.h>
#include <DataFormatsQualityControl/FlagReasonFactory.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::phos
{

void RawCheck::configure()
{
  auto param = mCustomParameters.find("mMinHGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMinHGPedestalValue = stoi(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMinHGPedestalValue "
                         << mMinHGPedestalValue << ENDM;
  }
  param = mCustomParameters.find("mMaxHGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMaxHGPedestalValue = stoi(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMaxHGPedestalValue "
                         << mMaxHGPedestalValue << ENDM;
  }
  param = mCustomParameters.find("mMinLGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMinLGPedestalValue = stoi(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMinLGPedestalValue "
                         << mMinLGPedestalValue << ENDM;
  }
  param = mCustomParameters.find("mMaxLGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMaxLGPedestalValue = stoi(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMaxLGPedestalValue "
                         << mMaxLGPedestalValue << ENDM;
  }
  // RMS
  param = mCustomParameters.find("mMinHGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMinHGPedestalRMS = stof(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMinHGPedestalRMS "
                         << mMinHGPedestalRMS << ENDM;
  }
  param = mCustomParameters.find("mMaxHGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMaxHGPedestalRMS = stof(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMaxHGPedestalRMS "
                         << mMaxHGPedestalRMS << ENDM;
  }
  param = mCustomParameters.find("mMinLGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMinLGPedestalRMS = stof(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMinLGPedestalRMS "
                         << mMinLGPedestalRMS << ENDM;
  }
  param = mCustomParameters.find("mMaxLGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMaxLGPedestalRMS = stof(param->second);
    ILOG(Debug, Support) << "configure() : Custom parameter mMaxLGPedestalRMS "
                         << mMaxLGPedestalRMS << ENDM;
  }

  for (int mod = 1; mod < 5; mod++) {
    // mToleratedBadPedestalValueChannelsM
    param = mCustomParameters.find(Form("mToleratedBadChannelsM%d", mod));
    if (param != mCustomParameters.end()) {
      mToleratedBadChannelsM[mod] = stoi(param->second);
      ILOG(Debug, Support) << "configure() : Custom parameter "
                           << Form("mToleratedBadChannelsM%d", mod)
                           << mToleratedBadChannelsM[mod] << ENDM;
    }
    // mToleratedDeviatedBranches
    param = mCustomParameters.find(Form("mToleratedDeviatedBranchesM%d", mod));
    if (param != mCustomParameters.end()) {
      mToleratedDeviatedBranches[mod] = stoi(param->second);
      ILOG(Debug, Support) << "configure() : Custom parameter "
                           << Form("mToleratedDeviatedBranchesM%d", mod)
                           << mToleratedDeviatedBranches[mod] << ENDM;
    }
    // mBranchOccupancyDeviationAllowed
    param = mCustomParameters.find(Form("mBranchOccupancyDeviationAllowedM%d", mod));
    if (param != mCustomParameters.end()) {
      mBranchOccupancyDeviationAllowed[mod] = stof(param->second);
      ILOG(Debug, Support) << "configure() : Custom parameter "
                           << Form("mBranchOccupancyDeviationAllowedM%d", mod)
                           << mBranchOccupancyDeviationAllowed[mod] << ENDM;
    }
  }
  // error occurance
  for (int i = 0; i < 5; i++) {
    if (auto param = mCustomParameters.find(Form("mErrorOccuranceThreshold%d", i));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mErrorOccuranceThreshold%d = ", i)
                         << param->second << "for the " << mErrorLabel[i] << ENDM;
      mErrorOccuranceThreshold[i] = stof(param->second);
    }
  }
  mBadMap[0] = 0;
}

Quality RawCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  mCheckResult = Quality::Good;
  for (auto& [moName, mo] : *moMap) {
    (void)moName; // trick the compiler about not used variable
    if (checkErrHistograms(mo.get())) {
      continue;
    }
    if (checkPedestalHistograms(mo.get())) {
      continue;
    }
    if (checkPhysicsHistograms(mo.get())) {
      continue;
    }
  }
  return mCheckResult;
}

std::string RawCheck::getAcceptedType() { return "TH1"; }

bool RawCheck::checkErrHistograms(MonitorObject* mo)
{
  // Return true if mo found and handled
  // else return false to search further

  if (mo->getName().find("ErrorTypePerDDL") != std::string::npos) { // HG mean summary 100, 0., 100.
    TH2S* h = dynamic_cast<TH2S*>(mo->getObject());
    if (h->Integral() == 0) {
      TPaveText* msg = new TPaveText(0., 0.8, 0.2, 0.9, "NDC");
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      msg->AddText("OK");
      msg->SetFillColor(kGreen);
      mCheckResult = Quality::Good;
      h->GetListOfFunctions()->Add(msg);
    } else {
      TPaveText* msg = new TPaveText(0.7, 0.6, 0.9, 0.9, "NDC");
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      mCheckResult = Quality::Bad;
      msg->AddText(Form("HW decoding errors"));
      msg->AddText(Form("Notify PHOS oncall"));
      msg->SetFillColor(kRed);
      h->SetFillColor(kRed);
      h->GetListOfFunctions()->Add(msg);
    }
    return true;
  }
  if (mo->getName().find("ErrorTypeOccurance") != std::string::npos) {
    auto* h = dynamic_cast<TH1Fraction*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1Fraction*, skipping" << ENDM;
    } else {
      h->SetOption("hist");
      h->SetDrawOption("hist");
      bool isGoodMO = true;
      for (int i = 1; i <= h->GetXaxis()->GetNbins(); i++) {
        if (h->GetBinContent(i) > mErrorOccuranceThreshold[i - 1]) {
          isGoodMO = false;
          if (mCheckResult.isBetterThan(Quality::Medium)) {
            mCheckResult.set(Quality::Medium);
          }
          mCheckResult.addReason(FlagReasonFactory::Unknown(), Form("too many %s errors", mErrorLabel[i - 1]));
          TLatex* msg = new TLatex(0.2, 0.2 + i * 0.125, Form("#color[2]{Too many %s errors}", mErrorLabel[i - 1]));
          msg->SetNDC();
          msg->SetTextSize(16);
          msg->SetTextFont(43);
          h->GetListOfFunctions()->Add(msg);
          msg->Draw();
          TPaveText* msgNotOk = new TPaveText(0.0, 0.0, 0.1, 0.1, "NDC");
          msgNotOk->SetName(Form("%s_msg", mo->GetName()));
          msgNotOk->Clear();
          msgNotOk->AddText("Not OK");
          msgNotOk->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msgNotOk);
          h->SetMarkerStyle(21);
          h->SetFillColor(kOrange);
        }
      }
      if (isGoodMO) {
        TPaveText* msgOk = new TPaveText(0.0, 0.0, 0.1, 0.1, "NDC");
        msgOk->SetName(Form("%s_msg", mo->GetName()));
        msgOk->Clear();
        msgOk->AddText("OK");
        msgOk->SetFillColor(kGreen);
        h->GetListOfFunctions()->Add(msgOk);
        h->SetFillColor(kGreen);
      }
    }
  }
  return false;
}
bool RawCheck::checkPhysicsHistograms(MonitorObject* mo)
{
  // get number of bad channels in each module. do it only once
  if (mo->getName().find("BadMapSummary") != std::string::npos) {
    if (mBadMap[0]) { // do it only once
      return true;
    }
    TH1F* h = dynamic_cast<TH1F*>(mo->getObject());
    for (int mod = 1; mod < 5; mod++) {
      mBadMap[mod] = h->GetBinContent(mod);
    }
    mBadMap[0] = 1;
    return true;
  }
  // Check occupancy histograms: if statistics is sufficient?
  // if yes - check number of dead channels wrt bad map
  if (mo->getName().find("CellHGOccupancyM") != std::string::npos) { // HG mean summary 100, 0., 100.
    int mod = atoi(&mo->getName()[mo->getName().find("CellHGOccupancyM") + 16]);
    TH2F* h = dynamic_cast<TH2F*>(mo->getObject());
    float nCounts = h->Integral();
    if ((mod == 1 && nCounts < 17920.) || (mod > 1 && nCounts < 35840.)) {
      TPaveText* msg = new TPaveText(0.0, 0.0, 0.3, 0.1, "NDC");
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      msg->AddText("Not enough stat");
      msg->SetFillColor(kGreen);
      h->GetListOfFunctions()->Add(msg);
      return true;
    }
    // calculate dead channels
    int nDead = 0;
    for (int ix = 1; ix <= 64; ix++) {
      for (int iz = 1; iz <= 56; iz++) {
        if (h->GetBinContent(ix, iz) == 0) {
          nDead++;
        }
      }
    }
    if (mod == 1) {
      nDead -= 1792;
    }
    // calculate deviated branches
    TH2F* hBranches = (TH2F*)h->Clone("hBranches");
    hBranches->Rebin2D(16, 28);
    double mean = 0;
    int nDeviatedBranches = 0;
    for (int ix = 1; ix <= 4; ix++) {
      mean += hBranches->GetBinContent(ix, 1);
      mean += hBranches->GetBinContent(ix, 2);
    }
    if (mod == 1) {
      mean /= 4.;
    } else {
      mean /= 8.;
    }
    for (int ix = (mod == 1) ? 3 : 1; ix <= 4; ix++) {
      for (int iz = 1; iz <= 2; iz++) {
        if (hBranches->GetBinContent(ix, iz) > mean * mBranchOccupancyDeviationAllowed[mod] ||
            hBranches->GetBinContent(ix, iz) < mean / mBranchOccupancyDeviationAllowed[mod])
          nDeviatedBranches++;
      }
    }
    // define quality
    if (mBadMap[0] && (nDead > mBadMap[mod] + mToleratedBadChannelsM[mod])) {
      if (mCheckResult.isBetterThan(Quality::Bad)) {
        mCheckResult.set(Quality::Bad);
      }
      mCheckResult.addReason(FlagReasonFactory::Unknown(), Form("too many dead channels M%d", mod));
      TLatex* msg = new TLatex(0.2, 0.2, Form("#color[2]{Too many dead channels: %d; but bad map has %d channels}", nDead, mToleratedBadChannelsM[mod]));
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
      TPaveText* msgNotOk = new TPaveText(0.0, 0.0, 0.1, 0.1, "NDC");
      msgNotOk->SetName(Form("%s_msg", mo->GetName()));
      msgNotOk->Clear();
      msgNotOk->AddText("Not OK");
      msgNotOk->SetFillColor(kRed);
      h->GetListOfFunctions()->Add(msgNotOk);
    } else if (nDeviatedBranches > mToleratedDeviatedBranches[mod]) {
      if (mCheckResult.isBetterThan(Quality::Medium)) {
        mCheckResult.set(Quality::Medium);
      }
      mCheckResult.addReason(FlagReasonFactory::Unknown(), Form("Branch oocupancy deviated M%d", mod));
      TLatex* msg = new TLatex(0.2, 0.4, Form("#color[2]{%d deviated branches; but %d allowed}", nDeviatedBranches, mToleratedDeviatedBranches[mod]));
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
      TPaveText* msgNotOk = new TPaveText(0.0, 0.0, 0.1, 0.1, "NDC");
      msgNotOk->SetName(Form("%s_msg", mo->GetName()));
      msgNotOk->Clear();
      msgNotOk->AddText("Not OK");
      msgNotOk->SetFillColor(kRed);
      h->GetListOfFunctions()->Add(msgNotOk);

    } else { // OK
      TPaveText* msg = new TPaveText(0.0, 0.0, 0.1, 0.1, "NDC");
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      msg->AddText("OK");
      msg->SetFillColor(kGreen);
      h->GetListOfFunctions()->Add(msg);
    }
    return true;
  }
  return false;
}

bool RawCheck::checkPedestalHistograms(MonitorObject* mo)
{
  // check pedestal historams
  // return true if object found/analyzed
  // Only 1D summary plots will be checked
  bool toCheck = false;
  float lowLimit = 0., upLimit = 0.;
  int mod = 0;
  if (!toCheck && mo->getName().find("PedHGMeanSum") != std::string::npos) { // HG mean summary 100, 0., 100.
    mod = atoi(&mo->getName()[mo->getName().find("PedHGMeanSum") + 12]);
    toCheck = true;
    lowLimit = mMinHGPedestalValue;
    upLimit = mMaxHGPedestalValue;
  }
  if (!toCheck && mo->getName().find("PedHGRMSSum") != std::string::npos) { // HG RMS summary 100, 0., 10.
    mod = atoi(&mo->getName()[mo->getName().find("PedHGRMSSum") + 11]);
    toCheck = true;
    lowLimit = mMinHGPedestalRMS;
    upLimit = mMaxHGPedestalRMS;
  }
  if (!toCheck && mo->getName().find("PedLGMeanSum") != std::string::npos) { // LG mean summary 100, 0., 100.
    mod = atoi(&mo->getName()[mo->getName().find("PedLGMeanSum") + 12]);
    toCheck = true;
    lowLimit = mMinLGPedestalValue;
    upLimit = mMaxLGPedestalValue;
  }
  if (!toCheck && mo->getName().find("PedLGRMSSum") != std::string::npos) { // HG mean summary 100, 0., 10.
    mod = atoi(&mo->getName()[mo->getName().find("PedLGRMSSum") + 11]);
    toCheck = true;
    lowLimit = mMinLGPedestalRMS;
    upLimit = mMaxLGPedestalRMS;
  }
  if (!toCheck) {
    return false;
  }
  int nTotal = (mod == 1) ? 1792 : 3584;
  TH1F* h = dynamic_cast<TH1F*>(mo->getObject());
  int startBin = h->GetXaxis()->FindBin(lowLimit);
  int lastBin = h->GetXaxis()->FindBin(upLimit);
  int nGood = h->Integral(startBin, lastBin);

  if (nGood > nTotal - mBadMap[mod] + mToleratedBadChannelsM[mod]) {
    TPaveText* msg = new TPaveText(0.7, 0.8, 0.9, 0.9, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    msg->AddText("OK");
    msg->SetFillColor(kGreen);
    mCheckResult = Quality::Good;
  } else {
    TPaveText* msg = new TPaveText(0.6, 0.6, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    mCheckResult = Quality::Bad;
    msg->AddText(Form("Too many bad channels"));
    msg->AddText(Form(" %d ", nTotal - nGood));
    msg->SetFillColor(kRed);
    h->SetFillColor(kRed);
  }
  return true;
}

} // namespace o2::quality_control_modules::phos
