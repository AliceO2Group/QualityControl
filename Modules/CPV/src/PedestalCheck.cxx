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
/// \file   PedestalCheck.cxx
/// \author Sergey Evdokimov
///

// QC
#include "CPV/PedestalCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ObjectMetadataKeys.h"
// O2
#include <DataFormatsQualityControl/FlagReasons.h>
#include <DataFormatsQualityControl/FlagReasonFactory.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::quality_control::repository;

namespace o2::quality_control_modules::cpv
{

void PedestalCheck::configure()
{
  ILOG(Info, Support) << "PedestalCheck::configure() : I have been called with following custom parameters" << mCustomParameters << ENDM;

  for (int mod = 0; mod < 3; mod++) {
    // mMinGoodPedestalValueM
    if (auto param = mCustomParameters.find(Form("mMinGoodPedestalValueM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinGoodPedestalValueM%d = ", mod + 2)
                         << param->second << ENDM;
      mMinGoodPedestalValueM[mod] = stoi(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mMinGoodPedestalValueM%d", mod + 2)
                         << " = "
                         << mMinGoodPedestalValueM[mod] << ENDM;
    // mMaxGoodPedestalSigmaM
    if (auto param = mCustomParameters.find(Form("mMaxGoodPedestalSigmaM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxGoodPedestalSigmaM%d = ", mod + 2)
                         << param->second << ENDM;
      mMaxGoodPedestalSigmaM[mod] = stof(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mMaxGoodPedestalSigmaM%d", mod + 2)
                         << " = "
                         << mMaxGoodPedestalSigmaM[mod] << ENDM;
    // mMinGoodPedestalEfficiencyM
    if (auto param = mCustomParameters.find(Form("mMinGoodPedestalEfficiencyM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinGoodPedestalEfficiencyM%d = ", mod + 2)
                         << param->second << ENDM;
      mMinGoodPedestalEfficiencyM[mod] = stof(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mMinGoodPedestalEfficiencyM%d", mod + 2)
                         << " = "
                         << mMinGoodPedestalEfficiencyM[mod] << ENDM;
    // mMaxGoodPedestalEfficiencyM
    if (auto param = mCustomParameters.find(Form("mMaxGoodPedestalEfficiencyM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxGoodPedestalEfficiencyM%d = ", mod + 2)
                         << param->second << ENDM;
      mMaxGoodPedestalEfficiencyM[mod] = stof(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mMaxGoodPedestalEfficiencyM%d", mod + 2)
                         << " = "
                         << mMaxGoodPedestalEfficiencyM[mod] << ENDM;
    // mToleratedBadPedestalValueChannelsM
    if (auto param = mCustomParameters.find(Form("mToleratedBadPedestalValueChannelsM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mToleratedBadPedestalValueChannelsM%d = ", mod + 2)
                         << param->second << ENDM;
      mToleratedBadPedestalValueChannelsM[mod] = stoi(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mToleratedBadPedestalValueChannelsM%d", mod + 2)
                         << " = "
                         << mToleratedBadPedestalValueChannelsM[mod] << ENDM;
    // mToleratedBadPedestalSigmaChannelsM
    if (auto param = mCustomParameters.find(Form("mToleratedBadPedestalSigmaChannelsM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mToleratedBadPedestalSigmaChannelsM%d = ", mod + 2)
                         << param->second << ENDM;
      mToleratedBadPedestalSigmaChannelsM[mod] = stoi(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mToleratedBadPedestalSigmaChannelsM%d", mod + 2)
                         << " = "
                         << mToleratedBadPedestalSigmaChannelsM[mod] << ENDM;

    // mToleratedBadChannelsM
    if (auto param = mCustomParameters.find(Form("mToleratedBadChannelsM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mToleratedBadChannelsM%d = ", mod + 2)
                         << param->second << ENDM;
      mToleratedBadChannelsM[mod] = stoi(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mToleratedBadChannelsM%d", mod + 2)
                         << " = "
                         << mToleratedBadChannelsM[mod] << ENDM;
    // mToleratedBadPedestalEfficiencyChannelsM
    if (auto param = mCustomParameters.find(Form("mToleratedBadPedestalEfficiencyChannelsM%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mToleratedBadPedestalEfficiencyChannelsM%d = ", mod + 2)
                         << param->second << ENDM;
      mToleratedBadPedestalEfficiencyChannelsM[mod] = stoi(param->second);
    }
    ILOG(Debug, Support) << "configure() : I use "
                         << Form("mToleratedBadPedestalEfficiencyChannelsM%d", mod + 2)
                         << " = "
                         << mToleratedBadPedestalEfficiencyChannelsM[mod] << ENDM;
  }
  ILOG(Info, Support) << "PedestalCheck::configure() : configuring is done." << ENDM;
  mIsConfigured = true;
}

Quality PedestalCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  if (!mIsConfigured) {
    ILOG(Info, Support) << "PedestalCheck::check() : I'm about to check already but configure() had not been called yet. So I call it now." << ENDM;
    configure();
  }

  // default
  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;                          // trick the compiler about not used variable
    for (int iMod = 0; iMod < 3; iMod++) { // loop modules
      if (mo->getName() == Form("PedestalValueM%d", iMod + 2)) {
        bool isGoodMO = true;

        auto* h = dynamic_cast<TH1F*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F*, skipping" << ENDM;
          continue;
        }
        TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
        h->GetListOfFunctions()->Add(msg);
        msg->SetName(Form("%s_msg", mo->GetName()));
        msg->Clear();
        // msg->AddText(Form("Run %d", getRunNumberFromMO(mo)));
        // count number of too small pedestals + too big pedestals
        int nOfBadPedestalValues = h->Integral(0, mMinGoodPedestalValueM[iMod]) + h->GetBinContent(h->GetNbinsX() + 1); // underflow + small pedestals + overflow
        if (nOfBadPedestalValues > mToleratedBadPedestalValueChannelsM[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result = Quality::Bad;
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("bad ped values M%d", iMod));
          msg->AddText(Form("Too many bad ped values: %d", nOfBadPedestalValues));
          msg->AddText(Form("Tolerated bad ped values: %d", mToleratedBadPedestalValueChannelsM[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
          isGoodMO = false;
        }
        // count number of bad pedestals (double peaked and so)
        int nOfBadPedestals = 7680 - h->GetEntries();
        if (nOfBadPedestals > mToleratedBadChannelsM[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("bad pedestals M%d", iMod));
          msg->AddText(Form("Too many bad channels: %d", nOfBadPedestals));
          msg->AddText(Form("Tolerated bad channels: %d", mToleratedBadChannelsM[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
          isGoodMO = false;
        }
        if (isGoodMO) {
          msg->AddText("OK");
          msg->SetFillColor(kGreen);
        }
        break; // exit modules loop for this particular object
      }

      if (mo->getName() == Form("PedestalSigmaM%d", iMod + 2)) {

        auto* h = dynamic_cast<TH1F*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F*, skipping" << ENDM;
          continue;
        }
        TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
        h->GetListOfFunctions()->Add(msg);
        msg->SetName(Form("%s_msg", mo->GetName()));
        msg->Clear();
        // msg->AddText(Form("Run %d", getRunNumberFromMO(mo)));
        // count number of too small pedestals + too big pedestals
        float binWidth = h->GetBinWidth(1);
        int nOfBadPedestalSigmas = h->Integral(mMaxGoodPedestalSigmaM[iMod] / binWidth + 1,
                                               h->GetNbinsX() + 1); // big sigmas + overflow

        if (nOfBadPedestalSigmas > mToleratedBadPedestalSigmaChannelsM[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("bad ped sigmas M%d", iMod));
          msg->AddText(Form("Too many bad ped sigmas: %d", nOfBadPedestalSigmas));
          msg->AddText(Form("Tolerated bad ped sigmas: %d", mToleratedBadPedestalSigmaChannelsM[iMod]));

          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
        } else {
          msg->AddText("OK");
          msg->SetFillColor(kGreen);
        }
        break; // exit modules loop for this particular object
      }

      if (mo->getName() == Form("PedestalEfficiencyM%d", iMod + 2)) {

        auto* h = dynamic_cast<TH1F*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F*, skipping" << ENDM;
          continue;
        }
        result = Quality::Good; // default
        TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
        h->GetListOfFunctions()->Add(msg);
        msg->SetName(Form("%s_msg", mo->GetName()));
        msg->Clear();
        // msg->AddText(Form("Run %d", getRunNumberFromMO(mo)));
        // count number of too small pedestals + too big pedestals
        float binWidth = h->GetBinWidth(1);
        int nOfBadPedestalEfficiencies = 7680 -
                                         h->Integral(mMinGoodPedestalEfficiencyM[iMod] / binWidth + 1,
                                                     mMaxGoodPedestalEfficiencyM[iMod] / binWidth); // big sigmas + overflow

        if (nOfBadPedestalEfficiencies > mToleratedBadPedestalEfficiencyChannelsM[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("bad ped efficiencies M%d", iMod));
          msg->AddText(Form("Too many bad ped efficiencies: %d", nOfBadPedestalEfficiencies));
          msg->AddText(Form("Tolerated bad ped efficiencies: %d",
                            mToleratedBadPedestalEfficiencyChannelsM[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
        } else {
          msg->AddText("OK");
          msg->SetFillColor(kGreen);
        }
        break; // exit modules loop for this particular object
      }

    } // iMod cycle
  }   // moMap cycle

  return result;
}

// std::string PedestalCheck::getAcceptedType() { return "TObject"; }
std::string PedestalCheck::getAcceptedType() { return "TH1"; }

void PedestalCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  return;                                // do noting for the time being. Maybe in the future we will do something sofisticated
  for (int iMod = 0; iMod < 3; iMod++) { // loop over modules
    if (mo->getName() == Form("PedestalValueM%d", iMod + 2)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F*, skipping" << ENDM;
        continue;
      }

      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
        //
      } else if (checkResult == Quality::Bad) {
        ILOG(Info, Support) << "beautify() : Quality::Bad, setting to red for "
                            << mo->GetName() << ENDM;
        h->SetFillColor(kRed);
        //
      } else if (checkResult == Quality::Medium) {
        ILOG(Error, Support) << "beautify() : unexpected quality for " << mo->GetName() << ENDM;
        h->SetFillColor(kOrange);
      }
      return; // exit when object is processed
    }
  }
}

int PedestalCheck::getRunNumberFromMO(std::shared_ptr<MonitorObject> mo)
{
  int runNumber{ 0 };
  auto metaData = mo->getMetadataMap();
  ILOG(Info, Support) << "PedestalCheck::check() : I have following metadata:" << ENDM;
  for (auto [key, value] : metaData) {
    ILOG(Info, Support) << "key = " << key << "; value = " << value << ENDM;
  }
  auto foundRN = metaData.find(metadata_keys::runNumber);
  if (foundRN != metaData.end()) {
    runNumber = std::stoi(foundRN->second);
    ILOG(Info, Support) << "PedestalCheck::check() : I found in metadata RunNumber = " << foundRN->second << ENDM;
  }
  if (runNumber == 0) {
    ILOG(Info, Support) << "PedestalCheck::check() : I haven't found RunNumber in metadata, using from Activity." << ENDM;
    runNumber = mo->getActivity().mId;
    ILOG(Info, Support) << "PedestalCheck::check() : RunNumber = " << runNumber << ENDM;
  }
  if (runNumber == 0) {
    auto foundRNFT = metaData.find("RunNumberFromTask");
    if (foundRNFT != metaData.end()) {
      runNumber = std::stoi(foundRNFT->second);
      ILOG(Info, Support) << "PedestalCheck::check() : I found in metadata RunNumberFromTask = " << foundRNFT->second << ENDM;
    }
  }
  return runNumber;
}

} // namespace o2::quality_control_modules::cpv
