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

#include "CPV/PhysicsCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ObjectMetadataKeys.h"
// ROOT
#include <TF1.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;
using namespace o2::quality_control::repository;

namespace o2::quality_control_modules::cpv
{

void PhysicsCheck::configure()
{
  ILOG(Info, Support) << "PhysicsCheck::configure() : I have been called with following custom parameters" << ENDM;
  for (auto [key, value] : mCustomParameters) {
    ILOG(Info, Support) << key << ": " << value << ENDM;
  }

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
    // mMinEventsToFit
    if (auto param = mCustomParameters.find(Form("mMinEventsToFit%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinEventsToFit%d = ", mod + 2)
                         << param->second << ENDM;
      mMinEventsToFit[mod] = stof(param->second);
    }

    // mMinAmplification
    if (auto param = mCustomParameters.find(Form("mMinAmplification%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinAmplification%d = ", mod + 2)
                         << param->second << ENDM;
      mMinAmplification[mod] = stof(param->second);
    }
    // mMaxAmplification
    if (auto param = mCustomParameters.find(Form("mMaxAmplification%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMaxAmplification%d = ", mod + 2)
                         << param->second << ENDM;
      mMaxAmplification[mod] = stof(param->second);
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

    // mMinEventsToCheckTrip
    if (auto param = mCustomParameters.find(Form("mMinEventsToCheckDigitMap%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mMinEventsToCheckDigitMap%d = ", mod + 2)
                         << param->second << ENDM;
      mMinEventsToCheckDigitMap[mod] = stoi(param->second);
    }
    // mStripPopulationDeviationAllowed
    if (auto param = mCustomParameters.find(Form("mStripPopulationDeviationAllowed%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mStripPopulationDeviationAllowed%d = ", mod + 2)
                         << param->second << ENDM;
      mStripPopulationDeviationAllowed[mod] = stof(param->second);
    }
    // mNBadStripsPerQuarterAllowed
    if (auto param = mCustomParameters.find(Form("mNBadStripsPerQuarterAllowed%d", mod + 2));
        param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "configure() : Custom parameter "
                         << Form("mNBadStripsPerQuarterAllowed%d = ", mod + 2)
                         << param->second << ENDM;
      mNBadStripsPerQuarterAllowed[mod] = stoi(param->second);
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

  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;                          // trick the compiler about not used variable
    for (int iMod = 0; iMod < 3; iMod++) { // loop modules
      if (mo->getName() == Form("CalibDigitEnergyM%d", iMod + 2)) {
        bool isGoodMO = true;

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
        float binWidth = h->GetBinWidth(1);
        float amplification;
        auto nEventsToFit = h->Integral(mAmplitudeRangeL[iMod] / binWidth + 1, mAmplitudeRangeR[iMod] / binWidth + 1);
        if (nEventsToFit < mMinEventsToFit[iMod]) {
          result = Quality::Medium;
          msg->AddText("Not enough data in fit range");
          msg->SetFillColor(kOrange);
          isGoodMO = false;
        } else {
          // fit with Landau function
          TF1* fLandau = new TF1("fLandau", "landau", mAmplitudeRangeL[iMod], mAmplitudeRangeR[iMod]);
          fLandau->SetParLimits(0, 0., 1.E100);
          fLandau->SetParLimits(1, 0., 1.E3);
          fLandau->SetParLimits(2, 0., 1.E3);
          fLandau->SetParameters(1000., 100., 60.);
          h->Fit(fLandau, "Q0N", "", mAmplitudeRangeL[iMod], mAmplitudeRangeR[iMod]);
          amplification = fLandau->GetParameter(1);
          if (amplification < mMinAmplification[iMod]) {
            result = Quality::Bad;
            msg->AddText(Form("Amplification is too small: %f", amplification));
            msg->AddText(Form("Lowest allowed amplification: %f", mMinAmplification[iMod]));
            msg->SetFillColor(kRed);
            h->SetFillColor(kRed);
            isGoodMO = false;
          } else if (amplification > mMaxAmplification[iMod]) {
            result = Quality::Bad;
            msg->AddText(Form("Amplification is too big: %f", amplification));
            msg->AddText(Form("Largest allowed amplification: %f", mMaxAmplification[iMod]));
            msg->SetFillColor(kRed);
            h->SetFillColor(kRed);
            isGoodMO = false;
          }
        }
        if (isGoodMO) {
          msg->AddText(Form("amplification %4.1f: OK", amplification));
          msg->SetFillColor(kGreen);
        }
        break; // exit modules loop for this particular object
      }

      if (mo->getName() == Form("NDigitsInClusterM%d", iMod + 2)) {

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
        msg->SetMargin(0.005);
        auto meanClusterSize = h->GetMean();
        if (meanClusterSize < mMinClusterSize[iMod]) {
          result = Quality::Bad;
          msg->AddText(Form("Too small mean cluster size: %f", meanClusterSize));
          msg->AddText(Form("Tolerated mean cluster size: %f", mMinClusterSize[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
        } else if (meanClusterSize > mMaxClusterSize[iMod]) {
          result = Quality::Bad;
          msg->AddText(Form("Too big mean cluster size: %f", meanClusterSize));
          msg->AddText(Form("Tolerated mean cluster size: %f", mMaxClusterSize[iMod]));
          msg->SetFillColor(kRed);
          h->SetFillColor(kRed);
        } else {
          msg->AddText("OK");
          msg->SetFillColor(kGreen);
        }
        break; // exit modules loop for this particular object
      }

      if (mo->getName() == Form("DigitMapM%d", iMod + 2)) {

        auto* h = dynamic_cast<TH2F*>(mo->getObject());
        if (h == nullptr) {
          ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH2F*, skipping" << ENDM;
          continue;
        }
        result = Quality::Good; // default

        if (h->GetEntries() < mMinEventsToCheckDigitMap[iMod]) {
          continue;
        }
        TPaveText* msg = new TPaveText(0.6, 0.0, 1., 0.3, "NDC");
        msg->SetName(Form("%s_msg", mo->GetName()));
        msg->Clear();
        msg->SetMargin(0.005);

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
          result = Quality::Bad;
          msg->AddText(Form("hot 3Gassiplex cards (%d/%d)", nHot3Gassiplexes, mNHot3GassiplexAllowed[iMod]));
          msg->SetTextSize(20);
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
        }
        if (nCold3Gassiplexes > mNCold3GassiplexAllowed[iMod]) {
          result = Quality::Bad;
          msg->AddText(Form("cold 3Gassiplex cards (%d/%d)", nCold3Gassiplexes, mNCold3GassiplexAllowed[iMod]));
          msg->SetFillColor(kRed);
          msg->SetTextSize(20);
          h->GetListOfFunctions()->Add(msg);
        }
        h3GassiplexMap->Delete();

        // check quarters for HV trip
        bool isHVTrip = false;
        int tripQ = 0, nTripQ = 0;
        TH1* projectionX = h->ProjectionX();
        TF1 pol0("constant", "pol0", 0., 128.);
        for (int q = 0; q < 4; q++) {
          int firstBin = 1 + q * 32;
          int lastBin = (q + 1) * 32;
          projectionX->Fit(&pol0, "Q0N", "", firstBin - 1, lastBin);
          double mean = pol0.GetParameter(0);
          int nBadStrips = 0;
          for (int iBin = firstBin; iBin < lastBin; iBin++) {
            if (projectionX->GetBinContent(iBin) > 0 && (projectionX->GetBinContent(iBin) > mean * mStripPopulationDeviationAllowed[iMod] ||
                                                         projectionX->GetBinContent(iBin) < mean / mStripPopulationDeviationAllowed[iMod])) {
              nBadStrips++;
            }
          }
          if (nBadStrips > mNBadStripsPerQuarterAllowed[iMod]) {
            tripQ = (q + 1) + 10 * tripQ;
            nTripQ++;
            isHVTrip = true;
          }
        }
        projectionX->Delete();
        if (isHVTrip) {
          result = Quality::Bad;
          msg->AddText(Form("HV Trip in Q%d ?", tripQ));
          msg->SetTextSize(20);
          msg->SetFillColor(kRed);
          h->GetListOfFunctions()->Add(msg);
        }

        // show OK message if everything is OK
        if (result == Quality::Good) {
          TPaveText* msgOk = new TPaveText(0.9, 0.0, 1., 0.1, "NDC");
          msgOk->SetName(Form("%s_msg", mo->GetName()));
          msgOk->Clear();
          msgOk->AddText("OK");
          msgOk->SetFillColor(kGreen);
          h->GetListOfFunctions()->Add(msgOk);
        }
        break; // exit modules loop for this particular object
      }

    } // iMod cycle
  }   // moMap cycle

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
