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
/// \file   PhysicsCheck.cxx
/// \author Sergey Evdokimov
///

// QC
#include "CPV/PhysicsCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "PHOS/TH1Fraction.h"
#include "PHOS/TH2Fraction.h"
// O2
#include <DataFormatsQualityControl/FlagReasons.h>
#include <DataFormatsQualityControl/FlagReasonFactory.h>
// ROOT
#include <TF1.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>
#include <TLatex.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::quality_control::repository;
using TH1Fraction = o2::quality_control_modules::phos::TH1Fraction;
using TH2Fraction = o2::quality_control_modules::phos::TH2Fraction;

namespace o2::quality_control_modules::cpv
{

void PhysicsCheck::configure()
{
  ILOG(Info, Support) << "PhysicsCheck::configure() : I have been called with following custom parameters" << mCustomParameters << ENDM;

  for (int mod = 0; mod < 3; mod++) {
    // mAmplitudeRangeL
    if (auto param = mCustomParameters.find(Form("mAmplitudeRangeL%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mAmplitudeRangeL%d = ", mod + 2)
                         << param->second << ENDM;
      mAmplitudeRangeL[mod] = stof(param->second);
    }
    // mAmplitudeRangeR
    if (auto param = mCustomParameters.find(Form("mAmplitudeRangeR%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mAmplitudeRangeR%d = ", mod + 2)
                         << param->second << ENDM;
      mAmplitudeRangeR[mod] = stof(param->second);
    }
    // mMinEventsToChaeckAmplitude
    if (auto param = mCustomParameters.find(Form("mMinEventsToCheckAmplitude%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinEventsToCheckAmplitude%d = ", mod + 2)
                         << param->second << ENDM;
      mMinEventsToCheckAmplitude[mod] = stof(param->second);
    }

    // mMinAmplitudeMean
    if (auto param = mCustomParameters.find(Form("mMinAmplitudeMean%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinAmplitudeMean%d = ", mod + 2)
                         << param->second << ENDM;
      mMinAmplitudeMean[mod] = stof(param->second);
    }
    // mMaxAmplitudeMean
    if (auto param = mCustomParameters.find(Form("mMaxAmplitudeMean%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxAmplitudeMean%d = ", mod + 2)
                         << param->second << ENDM;
      mMaxAmplitudeMean[mod] = stof(param->second);
    }
    // mMinEventsToCheckClusters
    if (auto param = mCustomParameters.find(Form("mMinEventsToCheckClusters%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinEventsToCheckClusters%d = ", mod + 2)
                         << param->second << ENDM;
      mMinEventsToCheckClusters[mod] = stoi(param->second);
    }
    // mCluEnergyRangeL
    if (auto param = mCustomParameters.find(Form("mCluEnergyRangeL%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mCluEnergyRangeL%d = ", mod + 2)
                         << param->second << ENDM;
      mCluEnergyRangeL[mod] = stof(param->second);
    }
    // mCluEnergyRangeR
    if (auto param = mCustomParameters.find(Form("mCluEnergyRangeR%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mCluEnergyRangeR%d = ", mod + 2)
                         << param->second << ENDM;
      mCluEnergyRangeR[mod] = stof(param->second);
    }
    // mMinCluEnergyMean
    if (auto param = mCustomParameters.find(Form("mMinCluEnergyMean%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinCluEnergyMean%d = ", mod + 2)
                         << param->second << ENDM;
      mMinCluEnergyMean[mod] = stof(param->second);
    }
    // mMaxCluEnergyMean
    if (auto param = mCustomParameters.find(Form("mMaxCluEnergyMean%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxCluEnergyMean%d = ", mod + 2)
                         << param->second << ENDM;
      mMaxCluEnergyMean[mod] = stof(param->second);
    }

    // mMinClusterSize
    if (auto param = mCustomParameters.find(Form("mMinClusterSize%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinClusterSize%d = ", mod + 2)
                         << param->second << ENDM;
      mMinClusterSize[mod] = stof(param->second);
    }
    // mMaxClusterSize
    if (auto param = mCustomParameters.find(Form("mMaxClusterSize%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxClusterSize%d = ", mod + 2)
                         << param->second << ENDM;
      mMaxClusterSize[mod] = stof(param->second);
    }
    // mMinEventsToCheckDigitMap
    if (auto param = mCustomParameters.find(Form("mMinEventsToCheckDigitMap%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinEventsToCheckDigitMap%d = ", mod + 2)
                         << param->second << ENDM;
      mMinEventsToCheckDigitMap[mod] = stoi(param->second);
    }
    // mNCold3GassiplexAllowed
    if (auto param = mCustomParameters.find(Form("mNCold3GassiplexAllowed%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mNCold3GassiplexAllowed%d = ", mod + 2)
                         << param->second << ENDM;
      mNCold3GassiplexAllowed[mod] = stoi(param->second);
    }
    // mNHot3GassiplexAllowed
    if (auto param = mCustomParameters.find(Form("mNHot3GassiplexAllowed%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mNHot3GassiplexAllowed%d = ", mod + 2)
                         << param->second << ENDM;
      mNHot3GassiplexAllowed[mod] = stoi(param->second);
    }
    // mHot3GassiplexCriterium
    if (auto param = mCustomParameters.find(Form("mHot3GassiplexCriterium%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mHot3GassiplexCriterium%d = ", mod + 2)
                         << param->second << ENDM;
      mHot3GassiplexCriterium[mod] = stof(param->second);
    }
    // mCold3GassilexCriterium
    if (auto param = mCustomParameters.find(Form("mCold3GassiplexCriterium%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mCold3GassiplexCriterium%d = ", mod + 2)
                         << param->second << ENDM;
      mCold3GassiplexCriterium[mod] = stof(param->second);
    }
    // mHot3GassiplexOccurance
    if (auto param = mCustomParameters.find(Form("mHot3GassiplexOccurance%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mHot3GassiplexOccurance%d = ", mod + 2)
                         << param->second << ENDM;
      mHot3GassiplexOccurance[mod] = stof(param->second);
    }
    // mCold3GassilexOccurance
    if (auto param = mCustomParameters.find(Form("mCold3GassiplexOccurance%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mCold3GassiplexOccurance%d = ", mod + 2)
                         << param->second << ENDM;
      mCold3GassiplexOccurance[mod] = stof(param->second);
    }
    // mMinDigitsPerEvent
    if (auto param = mCustomParameters.find(Form("mMinDigitsPerEvent%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinDigitsPerEvent%d = ", mod + 2)
                         << param->second << ENDM;
      mMinDigitsPerEvent[mod] = stof(param->second);
    }
    // mMaxDigitsPerEvent
    if (auto param = mCustomParameters.find(Form("mMaxDigitsPerEvent%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxDigitsPerEvent%d = ", mod + 2)
                         << param->second << ENDM;
      mMaxDigitsPerEvent[mod] = stof(param->second);
    }
  }
  // error occurance
  for (int i = 0; i < 20; i++) {
    if (auto param = mCustomParameters.find(Form("mErrorOccuranceThreshold%d", i));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mErrorOccuranceThreshold%d = ", i)
                         << param->second << "for the " << mErrorLabel[i] << ENDM;
      mErrorOccuranceThreshold[i] = stof(param->second);
    }
  }

  ILOG(Info, Support) << "PhysicsCheck::configure() : configuring is done." << ENDM;
  mIsConfigured = true;
}

Quality PhysicsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  if (!mIsConfigured) {
    ILOG(Info, Support) << "PhysicsCheck::check() : I'm about to check already but configure() had not been called yet. So I call it now." << ENDM;
    configure();
  }

  // default value
  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;                          // trick the compiler about not used variable
    for (int iMod = 0; iMod < 3; iMod++) { // loop modules
      // ========================= CALIBDIGIT ENERGY =========================
      if (mo->getName() == Form("CalibDigitEnergyM%d", iMod + 2)) {
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
        if (nEvents < mMinEventsToCheckAmplitude[iMod]) {
          if (result.isBetterThan(Quality::Null)) {
            result.set(Quality::Null);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("not enough statistics M%d", iMod + 2));
          msg->AddText("Not enough data to check");
          msg->SetFillColor(kOrange);
          isGoodMO = false;
        } else {
          h->GetXaxis()->SetRangeUser(mAmplitudeRangeL[iMod], mAmplitudeRangeR[iMod]);
          auto mean = h->GetMean();
          if (mean < mMinAmplitudeMean[iMod]) {
            if (result.isBetterThan(Quality::Medium)) {
              result.set(Quality::Medium);
            }
            result.addReason(FlagReasonFactory::Unknown(), Form("too small mean M%d", iMod + 2));
            msg->AddText(Form("Mean is too small: %f", mean));
            msg->AddText(Form("Min allowed mean: %f", mMinAmplitudeMean[iMod]));
            msg->SetFillColor(kRed);
            h->SetFillColor(kRed);
            isGoodMO = false;
          } else if (mean > mMaxAmplitudeMean[iMod]) {
            if (result.isBetterThan(Quality::Medium)) {
              result.set(Quality::Medium);
            }
            result.addReason(FlagReasonFactory::Unknown(), Form("too big mean M%d", iMod + 2));
            msg->AddText(Form("Mean is too big: %f", mean));
            msg->AddText(Form("Max allowed mean: %f", mMaxAmplitudeMean[iMod]));
            msg->SetFillColor(kRed);
            h->SetFillColor(kRed);
            isGoodMO = false;
          }
        }
        if (isGoodMO) {
          msg->AddText("OK");
          msg->SetFillColor(kGreen);
        }
        break; // exit modules loop for this particular object
      }
      // ========================= CLUSTER ENERGY =========================
      if (mo->getName() == Form("ClusterTotEnergyM%d", iMod + 2)) {
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
        if (nEvents < mMinEventsToCheckClusters[iMod]) {
          if (result.isBetterThan(Quality::Null)) {
            result.set(Quality::Null);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("not enough statistics M%d", iMod + 2));
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
            result.addReason(FlagReasonFactory::Unknown(), Form("too small mean energy M%d", iMod + 2));
            msg->AddText(Form("Mean is too small: %f", mean));
            msg->AddText(Form("Min allowed mean: %f", mMinCluEnergyMean[iMod]));
            msg->SetFillColor(kRed);
            h->SetFillColor(kRed);
            isGoodMO = false;
          } else if (mean > mMaxCluEnergyMean[iMod]) {
            if (result.isBetterThan(Quality::Medium)) {
              result.set(Quality::Medium);
            }
            result.addReason(FlagReasonFactory::Unknown(), Form("too big mean energy M%d", iMod + 2));
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
        break; // exit modules loop for this particular object
      }

      // ====================== CLUSTER SIZE =============================
      if (mo->getName() == Form("NDigitsInClusterM%d", iMod + 2)) {

        auto* h = dynamic_cast<TH1F*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F*, skipping" << ENDM;
          continue;
        }
        TPaveText* msg = new TPaveText(0.6, 0.5, 1.0, 0.75, "NDC");
        h->GetListOfFunctions()->Add(msg);
        msg->SetName(Form("%s_msg", mo->GetName()));
        msg->Clear();
        if (h->GetEntries() < mMinEventsToCheckClusters[iMod]) {
          result.addReason(FlagReasonFactory::Unknown(), Form("not enough statistics M%d", iMod));
          msg->AddText("Not enough data to check");
          msg->SetFillColor(kOrange);
          break;
        }
        auto meanClusterSize = h->GetMean();
        if (meanClusterSize < mMinClusterSize[iMod]) {
          if (result.isBetterThan(Quality::Medium)) {
            result.set(Quality::Medium);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("too small mean size M%d", iMod + 2));
          msg->AddText(Form("Mean is too small: %f", meanClusterSize));
          msg->AddText(Form("Min allowed mean: %f", mMinClusterSize[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
        } else if (meanClusterSize > mMaxClusterSize[iMod]) {
          if (result.isBetterThan(Quality::Medium)) {
            result.set(Quality::Medium);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("too big mean size M%d", iMod + 2));
          msg->AddText(Form("Mean is too big: %f", meanClusterSize));
          msg->AddText(Form("Max allowed mean: %f", mMaxClusterSize[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
        } else {
          msg->AddText("OK");
          msg->SetFillColor(kGreen);
        }
        break; // exit modules loop for this particular object
      }

      // ======================= DIGIT MAP =========================
      if (mo->getName() == Form("DigitMapM%d", iMod + 2)) {

        auto* h = dynamic_cast<TH2F*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH2F*, skipping" << ENDM;
          break;
        }
        if (h->GetEntries() < mMinEventsToCheckDigitMap[iMod]) {
          break;
        }
        bool isObjectGood = true;
        // check Cold and hot 3gassiplex cards
        int nHot3Gassiplexes = 0, nCold3Gassiplexes = 0;
        TH2* h3GassiplexMap = (TH2*)h->Clone("h3GassiplexMap");
        h3GassiplexMap->Rebin2D(8, 6);
        float meanOcc = 0;
        for (int iX = 1; iX <= h3GassiplexMap->GetNbinsX(); iX++) {
          for (int iY = 1; iY <= h3GassiplexMap->GetNbinsY(); iY++) {
            meanOcc += h3GassiplexMap->GetBinContent(iX, iY);
          }
        }
        meanOcc /= 160.;
        for (int iX = 1; iX <= h3GassiplexMap->GetNbinsX(); iX++) {
          for (int iY = 1; iY <= h3GassiplexMap->GetNbinsY(); iY++) {
            if (h3GassiplexMap->GetBinContent(iX, iY) > meanOcc * mHot3GassiplexCriterium[iMod]) {
              nHot3Gassiplexes++;
            }
            if (h3GassiplexMap->GetBinContent(iX, iY) < meanOcc * mCold3GassiplexCriterium[iMod]) {
              nCold3Gassiplexes++;
            }
          }
        }
        if (nHot3Gassiplexes > mNHot3GassiplexAllowed[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("many hot cards M%d", iMod + 2));
          TPaveText* msg = new TPaveText(0.0, 0.0, 0.2, 0.1, "NDC");
          msg->SetName(Form("%s_msgHot3G", mo->GetName()));
          msg->Clear();
          msg->AddText(Form("hot 3G cards (%d/%d)", nHot3Gassiplexes, mNHot3GassiplexAllowed[iMod]));
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
          isObjectGood = false;
        }
        if (nCold3Gassiplexes > mNCold3GassiplexAllowed[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("many cold cards M%d", iMod + 2));
          TPaveText* msg = new TPaveText(0.0, 0.9, 0.2, 1.0, "NDC");
          msg->AddText(Form("cold 3G cards (%d/%d)", nCold3Gassiplexes, mNCold3GassiplexAllowed[iMod]));
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
          isObjectGood = false;
        }
        delete h3GassiplexMap;

        // show OK message if everything is OK
        if (isObjectGood) {
          TPaveText* msgOk = new TPaveText(0.9, 0.9, 1.0, 1.0, "NDC");
          msgOk->SetName(Form("%s_msg", mo->GetName()));
          msgOk->Clear();
          msgOk->AddText("OK");
          msgOk->SetFillColor(kGreen);
          h->GetListOfFunctions()->Add(msgOk);
        }
        break; // exit modules loop for this particular object
      }
      // ======================= DIGIT OCCURANCE =========================
      if (mo->getName() == Form("DigitOccuranceM%d", iMod + 2)) {

        auto* h = dynamic_cast<TH2D*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH2D*, skipping" << ENDM;
          break;
        }
        auto* hFraction = dynamic_cast<TH2Fraction*>(mo->getObject());
        if (hFraction->getEventCounter() < mMinEventsToCheckDigitMap[iMod]) {
          break;
        }
        bool isObjectGood = true;
        // check Cold and hot 3gassiplex cards
        int nHot3Gassiplexes = 0, nCold3Gassiplexes = 0;
        TH2* h3GassiplexOccurance = (TH2*)h->Clone("h3GassiplexOccurance");
        h3GassiplexOccurance->Rebin2D(8, 6);
        h3GassiplexOccurance->Scale(1. / 48.);
        float meanOcc = 0;
        for (int iX = 1; iX <= h3GassiplexOccurance->GetNbinsX(); iX++) {
          for (int iY = 1; iY <= h3GassiplexOccurance->GetNbinsY(); iY++) {
            if (h3GassiplexOccurance->GetBinContent(iX, iY) > mHot3GassiplexOccurance[iMod]) {
              nHot3Gassiplexes++;
            }
            if (h3GassiplexOccurance->GetBinContent(iX, iY) < mCold3GassiplexOccurance[iMod]) {
              nCold3Gassiplexes++;
            }
          }
        }
        if (nHot3Gassiplexes > mNHot3GassiplexAllowed[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("digit occurance: many hot cards M%d", iMod + 2));
          TPaveText* msg = new TPaveText(0.0, 0.0, 0.2, 0.1, "NDC");
          msg->SetName(Form("%s_msgHot3G", mo->GetName()));
          msg->Clear();
          msg->AddText(Form("hot 3G cards (%d/%d)", nHot3Gassiplexes, mNHot3GassiplexAllowed[iMod]));
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
          isObjectGood = false;
        }
        if (nCold3Gassiplexes > mNCold3GassiplexAllowed[iMod]) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("digit occurance: many cold cards M%d", iMod + 2));
          TPaveText* msg = new TPaveText(0.0, 0.9, 0.2, 1.0, "NDC");
          msg->AddText(Form("cold 3G cards (%d/%d)", nCold3Gassiplexes, mNCold3GassiplexAllowed[iMod]));
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
          isObjectGood = false;
        }
        delete h3GassiplexOccurance;

        // show OK message if everything is OK
        if (isObjectGood) {
          TPaveText* msgOk = new TPaveText(0.9, 0.9, 1.0, 1.0, "NDC");
          msgOk->SetName(Form("%s_msg", mo->GetName()));
          msgOk->Clear();
          msgOk->AddText("OK");
          msgOk->SetFillColor(kGreen);
          h->GetListOfFunctions()->Add(msgOk);
        }
        break; // exit modules loop for this particular object
      }
      // ======================= PER EVENT COUNTS ===========================
      if (mo->getName() == Form("DigitsInEventM%d", iMod + 2)) {

        auto* h = dynamic_cast<TH1F*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F*, skipping" << ENDM;
          break;
        }
        auto mean = h->GetMean();
        if (mean > mMaxDigitsPerEvent[iMod]) {
          if (result.isBetterThan(Quality::Medium)) {
            result.set(Quality::Medium);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("too many digits per event M%d", iMod + 2));
          TPaveText* msg = new TPaveText(0.6, 0.6, 1.0, 0.8, "NDC");
          msg->AddText(Form("Mean is too big: %f", mean));
          msg->AddText(Form("Max allowed mean: %f", mMaxDigitsPerEvent[iMod]));
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
        } else if (mean < mMinDigitsPerEvent[iMod]) {
          if (result.isBetterThan(Quality::Medium)) {
            result.set(Quality::Medium);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("too few digits per event M%d", iMod + 2));
          TPaveText* msg = new TPaveText(0.6, 0.6, 1.0, 0.8, "NDC");
          msg->AddText(Form("Mean is too small: %f", mean));
          msg->AddText(Form("Min allowed mean: %f", mMinDigitsPerEvent[iMod]));
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
        } else {
          TPaveText* msgOk = new TPaveText(0.0, 0.0, 0.1, 0.1, "NDC");
          msgOk->SetName(Form("%s_msg", mo->GetName()));
          msgOk->Clear();
          msgOk->AddText("OK");
          msgOk->SetFillColor(kGreen);
          h->GetListOfFunctions()->Add(msgOk);
        }
      }
    } // iMod cycle
    // ======================= HW ERRORS CHECK ============================
    if (mo->getName() == "ErrorTypeOccurance") {
      auto* h = dynamic_cast<TH1Fraction*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1Fraction*, skipping" << ENDM;
        break;
      }
      bool isGoodMO = true;
      for (int i = 1; i <= h->GetXaxis()->GetNbins(); i++) {
        if (h->GetBinContent(i) > mErrorOccuranceThreshold[i - 1]) {
          isGoodMO = false;
          if (result.isBetterThan(Quality::Medium)) {
            result.set(Quality::Medium);
          }
          result.addReason(FlagReasonFactory::Unknown(), Form("too many %s errors", mErrorLabel[i - 1]));
          TLatex* msg = new TLatex(0.12 + (0.2 * (i % 2)), 0.2 + (i / 2) * 0.06, Form("#color[2]{Too many %s errors}", mErrorLabel[i - 1]));
          msg->SetNDC();
          msg->SetTextSize(16);
          msg->SetTextFont(43);
          h->GetListOfFunctions()->Add(msg);
          msg->Draw();
          h->SetFillColor(kOrange);
          h->SetOption("hist");
          h->SetDrawOption("hist");
          h->SetMarkerStyle(21);
          h->SetLineWidth(2);
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
        h->SetOption("hist");
        h->SetDrawOption("hist");
        h->SetMarkerStyle(21);
        h->SetLineWidth(2);
      }
    }

  } // moMap cycle

  return result;
} // namespace o2::quality_control_modules::cpv

void PhysicsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  return;                                // do noting for the time being. Maybe in the future we will do something sofisticated
  for (int iMod = 0; iMod < 3; iMod++) { // loop over modules
    if (mo->getName() == Form("PhysicsValueM%d", iMod + 2)) {
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

int PhysicsCheck::getRunNumberFromMO(std::shared_ptr<MonitorObject> mo)
{
  int runNumber{ 0 };
  auto metaData = mo->getMetadataMap();
  auto foundRN = metaData.find(metadata_keys::runNumber);
  if (foundRN != metaData.end()) {
    runNumber = std::stoi(foundRN->second);
  }
  if (runNumber == 0) {
    runNumber = mo->getActivity().mId;
  }
  if (runNumber == 0) {
    auto foundRNFT = metaData.find("RunNumberFromTask");
    if (foundRNFT != metaData.end()) {
      runNumber = std::stoi(foundRNFT->second);
    }
  }
  return runNumber;
}

} // namespace o2::quality_control_modules::cpv
