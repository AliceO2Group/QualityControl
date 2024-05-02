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
/// \file   QcMFTDigitCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
/// \author David Grund
///

// C++
#include <string>
// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
#include <TPaveText.h>
#include <TBox.h>
#include <TPad.h>
#include <TCanvas.h>
// O2
#include <DataFormatsITSMFT/NoiseMap.h>
#include <ITSMFTReconstruction/ChipMappingMFT.h>

// Quality Control
#include "MFT/QcMFTDigitCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTUtilTables.h"
#include "QualityControl/UserCodeInterface.h"
#include "QualityControl/CustomParameters.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTDigitCheck::configure()
{
  if (auto param = mCustomParameters.find("ZoneThresholdMedium"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ZoneThresholdMedium: " << param->second << ENDM;
    mZoneThresholdMedium = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("ZoneThresholdBad"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ZoneThresholdBad: " << param->second << ENDM;
    mZoneThresholdBad = stoi(param->second);
  }
  mNoiseScan = 0;
  if (auto param = mCustomParameters.find("NoiseScan"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - NoiseScan: " << param->second << ENDM;
    mNoiseScan = stoi(param->second);
  }
  mNCyclesNoiseMap = 3;
  if (auto param = mCustomParameters.find("NCyclesNoiseMap"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - NCyclesNoiseMap: " << param->second << ENDM;
    mNCyclesNoiseMap = stoi(param->second);
  }

  // no call to beautifier yet
  mFirstCall = true;

  mNCycles = 0;
  mNewNoisy = 0;
  mDissNoisy = 0;
  mTotalNoisy = 0;
}

Quality QcMFTDigitCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mDigitChipOccupancy") {
      auto* hDigitChipOccupancy = dynamic_cast<TH1F*>(mo->getObject());
      if (hDigitChipOccupancy == nullptr) {
        ILOG(Error, Support) << "Could not cast mDigitChipOccupancy to TH1F." << ENDM;
        return Quality::Null;
      }

      for (int iBin = 0; iBin < hDigitChipOccupancy->GetNbinsX(); iBin++) {
        if (hDigitChipOccupancy->GetBinContent(iBin + 1) == 0) {
          hDigitChipOccupancy->Fill(937); // number of chips with zero digits stored in the overflow bin
        }
      }
    }

    if (mo->getName() == "mDigitOccupancySummary") {
      auto* hDigitOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());
      if (hDigitOccupancySummary == nullptr) {
        ILOG(Error, Support) << "Could not cast mDigitOccupancySummary to TH2F." << ENDM;
        return Quality::Null;
      }

      float nEmptyBins = 0; // number of empty zones

      for (int iBinX = 0; iBinX < hDigitOccupancySummary->GetNbinsX(); iBinX++) {
        for (int iBinY = 0; iBinY < hDigitOccupancySummary->GetNbinsY(); iBinY++) {
          if ((hDigitOccupancySummary->GetBinContent(iBinX + 1, iBinY + 1)) == 0) {
            nEmptyBins = nEmptyBins + 1;
          }
        }
      }

      if (nEmptyBins <= mZoneThresholdMedium) {
        result = Quality::Good;
      }
      if (nEmptyBins > mZoneThresholdMedium && nEmptyBins <= mZoneThresholdBad) {
        result = Quality::Medium;
      }
      if (nEmptyBins > mZoneThresholdBad) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string QcMFTDigitCheck::getAcceptedType() { return "TH1"; }

void QcMFTDigitCheck::readMaskedChips(std::shared_ptr<MonitorObject> mo)
{
  long timestamp = mo->getValidity().getMin();
  map<string, string> headers;
  map<std::string, std::string> filter;
  auto calib = UserCodeInterface::retrieveConditionAny<o2::itsmft::NoiseMap>("MFT/Calib/DeadMap/", filter, timestamp);
  if (calib == nullptr) {
    ILOG(Error, Support) << "Could not retrieve deadmap from CCDB." << ENDM;
    return;
  }
  for (int i = 0; i < calib->size(); i++) {
    if (calib->isFullChipMasked(i)) {
      mMaskedChips.push_back(i);
    }
  }
}

void QcMFTDigitCheck::readNoiseMap(std::shared_ptr<MonitorObject> mo, long timestamp)
{
  map<string, string> headers;
  map<std::string, std::string> filter;
  auto calib = UserCodeInterface::retrieveConditionAny<o2::itsmft::NoiseMap>("MFT/Calib/NoiseMap/", filter, timestamp);
  if (calib == nullptr) {
    ILOG(Error, Support) << "Could not retrieve noisemap from CCDB." << ENDM;
    return;
  }
  mNoisyPix.clear();
  for (int chipID = 0; chipID < 936; chipID++) {
    for (int row = 0; row < 512; row++) {
      for (int col = 0; col < 1024; col++) {
        int noise = calib->getNoiseLevel(chipID, row, col);
        if (!noise) {
          noise = -1;
        }
        mNoisyPix.push_back(noise);
      }
    }
  }
}

void QcMFTDigitCheck::getChipMapData()
{
  const o2::itsmft::ChipMappingMFT mapMFT;
  auto chipMapData = mapMFT.getChipMappingData();
  QcMFTUtilTables MFTTable;

  for (int i = 0; i < 936; i++) {
    mHalf[i] = chipMapData[i].half;
    mDisk[i] = chipMapData[i].disk;
    mLayer[i] = chipMapData[i].layer;
    mFace[i] = mLayer[i] % 2;
    mZone[i] = chipMapData[i].zone;
    mSensor[i] = chipMapData[i].localChipSWID;
    mTransID[i] = chipMapData[i].cable;
    mLadder[i] = MFTTable.mLadder[i];
    mX[i] = MFTTable.mX[i];
    mY[i] = MFTTable.mY[i];
  }
}

void QcMFTDigitCheck::createMaskedChipsNames()
{
  for (int i = 0; i < mMaskedChips.size(); i++) {
    mChipMapName.push_back(Form("ChipOccupancyMaps/Half_%d/Disk_%d/Face_%d/mDigitChipOccupancyMap",
                                mHalf[mMaskedChips[i]], mDisk[mMaskedChips[i]], mFace[mMaskedChips[i]]));
  }
}

void QcMFTDigitCheck::createOutsideAccNames()
{
  for (int iHalf = 0; iHalf < 2; iHalf++) {
    for (int iDisk = 0; iDisk < 5; iDisk++) {
      for (int iFace = 0; iFace < 2; iFace++) {
        mOutsideAccName.push_back(Form("ChipOccupancyMaps/Half_%d/Disk_%d/Face_%d/mDigitChipOccupancyMap", iHalf, iDisk, iFace));
      }
    }
  }
}

void QcMFTDigitCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  mNCycles++;
  // set up masking of dead chips once
  if (mFirstCall) {
    mFirstCall = false;
    readMaskedChips(mo);
    getChipMapData();
    createMaskedChipsNames();
    createOutsideAccNames();
  }
  // print skull in maps to display dead chips
  int nMaskedChips = mMaskedChips.size();
  for (int i = 0; i < nMaskedChips; i++) {
    if (mo->getName().find(mChipMapName[i]) != std::string::npos) {
      auto* hMap = dynamic_cast<TH2F*>(mo->getObject());
      int binCx = hMap->GetXaxis()->FindBin(mX[mMaskedChips[i]]);
      int binCy = hMap->GetYaxis()->FindBin(mY[mMaskedChips[i]]);
      // the -0.5 is a shift to centre better the skulls
      TLatex* tl = new TLatex(hMap->GetXaxis()->GetBinCenter(binCx) - 0.5, hMap->GetYaxis()->GetBinCenter(binCy), "N");
      tl->SetTextFont(142);
      tl->SetTextSize(0.08);
      hMap->GetListOfFunctions()->Add(tl);
      tl->Draw();
    }
  }

  QcMFTUtilTables MFTTable;
  for (int iHalf = 0; iHalf < 2; iHalf++) {
    for (int iDisk = 0; iDisk < 5; iDisk++) {
      for (int iFace = 0; iFace < 2; iFace++) {
        int idx = (iDisk * 2 + iFace) + (10 * iHalf);
        if (mo->getName().find(mOutsideAccName[idx]) != std::string::npos) {
          auto* h = dynamic_cast<TH2F*>(mo->getObject());
          for (int i = 0; i < 21; i++) {
            int binX = MFTTable.mBinX[idx][i];
            int binY = MFTTable.mBinY[idx][i];
            if (binX == -1 || binY == -1) {
              continue;
            }
            TBox* b = new TBox(h->GetXaxis()->GetBinLowEdge(binX), h->GetYaxis()->GetBinLowEdge(binY), h->GetXaxis()->GetBinWidth(binX) + h->GetXaxis()->GetBinLowEdge(binX), h->GetYaxis()->GetBinWidth(binY) + h->GetYaxis()->GetBinLowEdge(binY));
            b->SetFillStyle(4055);
            b->SetFillColor(15);
            h->GetListOfFunctions()->Add(b);
            b->Draw();
          }
        }
      }
    }
  }
  if (mNoiseScan == 1) {
    if (mNCycles == 1) {
      // long timestamp = mo->getValidity().getMin();
      long timestamp = 1714531157487;
      readNoiseMap(mo, timestamp);
      mOldNoisyPix = mNoisyPix;
    }

    if (mNCycles == mNCyclesNoiseMap) {
      long timestamp = o2::ccdb::getCurrentTimestamp();
      readNoiseMap(mo, timestamp);
      mNewNoisyPix = mNoisyPix;

      for (int i = 0; i < mNewNoisyPix.size(); i++) {
        if (mNewNoisyPix[i] == -1 && mOldNoisyPix[i] != -1) {
          mDissNoisy++;
        }
        if (mNewNoisyPix[i] != -1 && mOldNoisyPix[i] == -1) {
          mNewNoisy++;
        }
        if (mNewNoisyPix[i] != -1) {
          mTotalNoisy++;
        }
      }
    }

    if (mo->getName().find("mDigitChipOccupancy") != std::string::npos) {
      auto* DigitOccupancy = dynamic_cast<TH1F*>(mo->getObject());
      if (DigitOccupancy != nullptr) {
        TLatex* tl_total = new TLatex(0.14, 0.87, Form("Total noisy pixels: %i", mTotalNoisy));
        TLatex* tl_new = new TLatex(0.14, 0.83, Form("New noisy pixels: %i", mNewNoisy));
        TLatex* tl_dis = new TLatex(0.14, 0.79, Form("Disappeared noisy pixels: %i", mDissNoisy));
        tl_total->SetNDC();
        tl_total->SetTextFont(42);
        tl_total->SetTextSize(0.03);
        tl_total->SetTextColor(kBlue);
        tl_new->SetNDC();
        tl_new->SetTextFont(42);
        tl_new->SetTextSize(0.03);
        tl_new->SetTextColor(kBlue);
        tl_dis->SetNDC();
        tl_dis->SetTextFont(42);
        tl_dis->SetTextSize(0.03);
        tl_dis->SetTextColor(kBlue);
        // add it to the histo
        DigitOccupancy->GetListOfFunctions()->Add(tl_total);
        DigitOccupancy->GetListOfFunctions()->Add(tl_new);
        DigitOccupancy->GetListOfFunctions()->Add(tl_dis);
        tl_total->Draw();
        tl_new->Draw();
        tl_dis->Draw();
      }
    }
  }

  if (mo->getName().find("mDigitOccupancySummary") != std::string::npos) {
    auto* hDigitOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());
    TPaveText* msg1 = new TPaveText(0.05, 0.9, 0.35, 1.0, "NDC NB");
    TPaveText* msg2 = new TPaveText(0.65, 0.9, 0.95, 1.0, "NDC NB");
    hDigitOccupancySummary->GetListOfFunctions()->Add(msg1);
    hDigitOccupancySummary->GetListOfFunctions()->Add(msg2);
    msg1->SetName(Form("%s_msg", mo->GetName()));
    msg2->SetName(Form("%s_msg2", mo->GetName()));
    if (checkResult == Quality::Good) {
      LOG(info) << "Quality::Good";
      msg1->Clear();
      msg1->AddText("Quality Good");
      msg1->SetFillColor(kGreen);
      msg1->Draw();
      msg2->Clear();
      msg2->AddText("No action needed");
      msg2->SetFillColor(kGreen);
      msg2->Draw();
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::Medium";
      msg1->Clear();
      msg1->AddText("Quality medium");
      msg1->SetFillColor(kOrange);
      msg1->Draw();
      msg2->Clear();
      msg2->AddText("Write a logbook entry tagging MFT");
      msg2->SetFillColor(kOrange);
      msg2->Draw();
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad";
      msg1->Clear();
      msg1->AddText("Quality bad");
      msg1->SetFillColor(kRed);
      msg1->Draw();
      msg2->Clear();
      msg2->AddText("Call the on-call!");
      msg2->SetFillColor(kRed);
      msg2->Draw();
    }
  }
}

} // namespace o2::quality_control_modules::mft
