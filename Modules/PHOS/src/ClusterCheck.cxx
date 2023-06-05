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
/// \file   ClusterCheck.cxx
/// \author Dmitri Peressounko
///

#include "PHOS/ClusterCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TList.h>
#include <iostream>
// O2
#include <DataFormatsQualityControl/FlagReasons.h>
#include <DataFormatsQualityControl/FlagReasonFactory.h>

using namespace std;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::phos
{

void ClusterCheck::configure()
{
  for (int mod = 1; mod < 5; mod++) {
    // mDeadThreshold
    if (auto param = mCustomParameters.find(Form("mDeadThreshold%d", mod));
        param != mCustomParameters.end()) {
      mDeadThreshold[mod] = stoi(param->second);
      ILOG(Debug, Support) << "configure() : Custom parameter "
                           << Form("mDeadThreshold%d", mod)
                           << mDeadThreshold[mod] << ENDM;
    }
    // mNoisyThreshold
    if (auto param = mCustomParameters.find(Form("mNoisyThreshold%d", mod));
        param != mCustomParameters.end()) {
      mNoisyThreshold[mod] = stoi(param->second);
      ILOG(Debug, Support) << "configure() : Custom parameter "
                           << Form("mNoisyThreshold%d", mod)
                           << mNoisyThreshold[mod] << ENDM;
    }
    // mMaxOccupancyCut
    if (auto param = mCustomParameters.find(Form("mMaxOccupancyCut%d", mod));
        param != mCustomParameters.end()) {
      mMaxOccupancyCut[mod] = stoi(param->second);
      ILOG(Debug, Support) << "configure() : Custom parameter "
                           << Form("mMaxOccupancyCut%d", mod)
                           << mMaxOccupancyCut[mod] << ENDM;
    }

    // mCluEnergyRangeL
    if (auto param = mCustomParameters.find(Form("mCluEnergyRangeL%d", mod));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mCluEnergyRangeL%d = ", mod)
                         << param->second << ENDM;
      mCluEnergyRangeL[mod] = stof(param->second);
    }
    // mCluEnergyRangeR
    if (auto param = mCustomParameters.find(Form("mCluEnergyRangeR%d", mod));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mCluEnergyRangeR%d = ", mod)
                         << param->second << ENDM;
      mCluEnergyRangeR[mod] = stof(param->second);
    }
    // mMinCluEnergyMean
    if (auto param = mCustomParameters.find(Form("mMinCluEnergyMean%d", mod));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinCluEnergyMean%d = ", mod)
                         << param->second << ENDM;
      mMinCluEnergyMean[mod] = stof(param->second);
    }
    // mMaxCluEnergyMean
    if (auto param = mCustomParameters.find(Form("mMaxCluEnergyMean%d", mod));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxCluEnergyMean%d = ", mod)
                         << param->second << ENDM;
      mMaxCluEnergyMean[mod] = stof(param->second);
    }
  }
}

Quality ClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Good;
  for (auto& [moName, mo] : *moMap) {
    //check occupancy
    if (mo->getName().find("ClusterOccupancyM") != std::string::npos) {
      // the problem here is that CcdbUrl must be setted via setCcdbUrl() method
      // otherwise it tries to open empty url
      // but how to access global qc config here?
      // for the timebeing don't include "ClusterOccupancyM" in list of objects to check
      if (!mBadMap) {
        mBadMap = UserCodeInterface::retrieveConditionAny<o2::phos::BadChannelsMap>("PHS/Calib/BadMap");
      }

      auto* h = dynamic_cast<TH2*>(mo->getObject());
      if (h->GetEntries() == 0) {
        continue;
      }
      //get module number:
      int m = mo->getName()[mo->getName().find_first_of("1234")] - '0';
      //Compare with current bad map: check for new bad regions
      float mean = 0;
      int entries = 0, nDead = 0, nNoisy = 0;
      for (int ix = 1; ix <= 64; ix++) {
        for (int iz = 1; iz <= 56; iz++) {
          float cont = h->GetBinContent(ix, iz);
          char relid[3] = { char(m), char(ix), char(iz) };
          short absId;
          o2::phos::Geometry::relToAbsNumbering(relid, absId);
          if (cont > 0) {
            mean += cont;
            entries++;
          } else {
            if (mBadMap->isChannelGood(absId)) { //new bad channel
              nDead++;
            }
          }
        }
      }
      if (entries) {
        mean /= entries;
      }
      //scan noisy channels
      for (int ix = 1; ix <= 64; ix++) {
        for (int iz = 1; iz <= 56; iz++) {
          float cont = h->GetBinContent(ix, iz);
          if (cont > mMaxOccupancyCut[m] * mean) {
            nNoisy++;
          }
        }
      }
      // check quality
      // dead channels
      if (nDead > mDeadThreshold[m]) {
        if (result.isBetterThan(Quality::Bad)) {
          result.set(Quality::Bad);
        }
        result.addReason(FlagReasonFactory::Unknown(), Form("too many dead channels M%d", m));
        TLatex* msg = new TLatex(0.2, 0.2, Form("#color[2]{Too many new dead channels: %d}", nDead));
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
      } else if (nNoisy > mNoisyThreshold[m]) { // noisy channels
        if (result.isBetterThan(Quality::Medium)) {
          result.set(Quality::Medium);
        }
        result.addReason(FlagReasonFactory::Unknown(), Form("too many noisy channels M%d", m));
        TLatex* msg = new TLatex(0.2, 0.2, Form("#color[2]{Too many noisy channels: %d}", nNoisy));
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
      } else {
        TPaveText* msgOk = new TPaveText(0.0, 0.0, 0.1, 0.1, "NDC");
        msgOk->SetName(Form("%s_msg", mo->GetName()));
        msgOk->Clear();
        msgOk->AddText("OK");
        msgOk->SetFillColor(kGreen);
        h->GetListOfFunctions()->Add(msgOk);
        h->SetFillColor(kGreen);
      }
    }
    // check energy spectrum
    if (mo->getName().find("SpectrumM") != std::string::npos) {
      LOG(info) << "I checking " << mo->getName();

      //get module number:
      int iMod = mo->getName()[mo->getName().find_first_of("1234")] - '0';
      bool isGoodMO = true;
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F*, skipping" << ENDM;
        continue;
      }
      TPaveText* msg = new TPaveText(0.6, 0.5, 1.0, 0.75, "NDC");
      h->GetListOfFunctions()->Add(msg);
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();

      auto nEvents = h->GetEntries();
      if (nEvents == 0) {
        if (result.isBetterThan(Quality::Null)) {
          result.set(Quality::Null);
        }
        result.addReason(FlagReasonFactory::Unknown(), Form("not enough statistics M%d", iMod));
        msg->AddText("Not enough data to check");
        msg->SetFillColor(kOrange);
        isGoodMO = false;
      } else {
        h->GetXaxis()->SetRangeUser(mCluEnergyRangeL[iMod], mCluEnergyRangeR[iMod]);
        auto mean = h->GetMean();
        if (mean < mMinCluEnergyMean[iMod]) {
          if (result.isBetterThan(Quality::Medium)) {
            result.set(Quality::Medium);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("too small mean energy M%d", iMod));
          msg->AddText(Form("Mean is too small: %f", mean));
          msg->AddText(Form("Min allowed mean: %f", mMinCluEnergyMean[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
          isGoodMO = false;
        } else if (mean > mMaxCluEnergyMean[iMod]) {
          if (result.isBetterThan(Quality::Medium)) {
            result.set(Quality::Medium);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("too big mean energy M%d", iMod));
          msg->AddText(Form("Mean is too big: %f", mean));
          msg->AddText(Form("Max allowed mean: %f", mMaxCluEnergyMean[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
          isGoodMO = false;
        }
      }
      if (isGoodMO) {
        msg->AddText("OK");
        msg->SetFillColor(kGreen);
      }
    }
  }
  return result;
} // namespace o2::quality_control_modules::phos

std::string ClusterCheck::getAcceptedType() { return "TH1"; }

void ClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("ClusterOccupancyM") != std::string::npos) {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Occupancy OK!!!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Too many dead channels");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call PHOS on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Devel) << "Quality::medium, setting to orange";
      msg->Clear();
      msg->AddText("Too many noisy channels");
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}
} // namespace o2::quality_control_modules::phos
