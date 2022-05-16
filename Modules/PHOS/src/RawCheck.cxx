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

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <fairlogger/Logger.h>
#include "PHOS/RawCheck.h"

using namespace std;

namespace o2::quality_control_modules::phos
{

void RawCheck::configure()
{
  auto param = mCustomParameters.find("mMinHGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMinHGPedestalValue = stoi(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMinHGPedestalValue "
                        << mMinHGPedestalValue << ENDM;
  }
  param = mCustomParameters.find("mMaxHGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMaxHGPedestalValue = stoi(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMaxHGPedestalValue "
                        << mMaxHGPedestalValue << ENDM;
  }
  param = mCustomParameters.find("mMinLGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMinLGPedestalValue = stoi(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMinLGPedestalValue "
                        << mMinLGPedestalValue << ENDM;
  }
  param = mCustomParameters.find("mMaxLGPedestalValue");
  if (param != mCustomParameters.end()) {
    mMaxLGPedestalValue = stoi(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMaxLGPedestalValue "
                        << mMaxLGPedestalValue << ENDM;
  }
  // RMS
  param = mCustomParameters.find("mMinHGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMinHGPedestalRMS = stof(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMinHGPedestalRMS "
                        << mMinHGPedestalRMS << ENDM;
  }
  param = mCustomParameters.find("mMaxHGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMaxHGPedestalRMS = stof(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMaxHGPedestalRMS "
                        << mMaxHGPedestalRMS << ENDM;
  }
  param = mCustomParameters.find("mMinLGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMinLGPedestalRMS = stof(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMinLGPedestalRMS "
                        << mMinLGPedestalRMS << ENDM;
  }
  param = mCustomParameters.find("mMaxLGPedestalRMS");
  if (param != mCustomParameters.end()) {
    mMaxLGPedestalRMS = stof(param->second);
    ILOG(Info, Support) << "configure() : Custom parameter mMaxLGPedestalRMS "
                        << mMaxLGPedestalRMS << ENDM;
  }

  // mToleratedBadPedestalValueChannelsM
  for (int mod = 1; mod < 5; mod++) {
    param = mCustomParameters.find(Form("mToleratedBadChannelsM%d", mod));
    if (param != mCustomParameters.end()) {
      mToleratedBadChannelsM[mod] = stoi(param->second);
      ILOG(Info, Support) << "configure() : Custom parameter "
                          << Form("mToleratedBadChannelsM%d", mod)
                          << mToleratedBadChannelsM[mod] << ENDM;
    }
  }
}

Quality RawCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  mCheckResult = Quality::Null;
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

void RawCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
}

bool RawCheck::checkErrHistograms(MonitorObject* mo)
{
  // Return true if mo found and handled
  // else return false to search further

  if (mo->getName().find("BadMapSummary") != std::string::npos) {                   // HG mean summary 100, 0., 100.
    if (mBadMap[1] != 0 || mBadMap[2] != 0 || mBadMap[3] != 0 || mBadMap[4] != 0) { // already set
      return true;
    }
    TH1F* h = dynamic_cast<TH1F*>(mo->getObject());
    for (int mod = 1; mod < 5; mod++) {
      mBadMap[mod] = h->GetBinContent(mod);
    }
    return true;
  }
  if (mo->getName().find("ErrorTypePerDDL") != std::string::npos) { // HG mean summary 100, 0., 100.
    TH2F* h = dynamic_cast<TH2F*>(mo->getObject());
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
  return false;
}
bool RawCheck::checkPhysicsHistograms(MonitorObject* mo)
{
  // Check occupancy histograms: if statistics is sufficient?
  // if yes - check number of dead channels wrt bad map
  if (mo->getName().find("CellOccupancyM") != std::string::npos) { // HG mean summary 100, 0., 100.
    int mod = atoi(&mo->getName()[mo->getName().find("CellOccupancyM") + 14]);
    TH2F* h = dynamic_cast<TH2F*>(mo->getObject());
    float nCounts = h->Integral();
    if ((mod == 1 && nCounts < 17920.) || (mod > 1 && nCounts < 35840.)) {
      mCheckResult = Quality::Null;
      return true;
    }
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
    if (nDead > mBadMap[mod] + mToleratedBadChannelsM[mod]) {
      mCheckResult = Quality::Bad;
      TPaveText* msg = new TPaveText(0.6, 0.6, 0.9, 0.75, "NDC");
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      mCheckResult = Quality::Bad;
      msg->AddText(Form("Too many new bad channels"));
      msg->AddText(Form(" %d in bad map: %d ", nDead, mBadMap[mod]));
      msg->SetFillColor(kRed);
      h->GetListOfFunctions()->Add(msg);
      h->SetFillColor(kRed);
    } else { // OK
      TPaveText* msg = new TPaveText(0.6, 0.8, 0.9, 0.9, "NDC");
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      msg->AddText("OK");
      msg->SetFillColor(kGreen);
      h->GetListOfFunctions()->Add(msg);
      mCheckResult = Quality::Good;
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
