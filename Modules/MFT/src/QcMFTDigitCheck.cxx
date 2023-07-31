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

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTDigitCheck::configure()
{
  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("ZoneThresholdMedium"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ZoneThresholdMedium: " << param->second << ENDM;
    mZoneThresholdMedium = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("ZoneThresholdBad"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ZoneThresholdBad: " << param->second << ENDM;
    mZoneThresholdBad = stoi(param->second);
  }

  // no call to beautifier yet
  mFirstCall = true;
}

Quality QcMFTDigitCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mDigitChipOccupancy") {
      auto* hChipOccupancy = dynamic_cast<TH1F*>(mo->getObject());

      float den = hChipOccupancy->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < hChipOccupancy->GetNbinsX(); iBin++) {
        if (hChipOccupancy->GetBinContent(iBin + 1) == 0) {
          hChipOccupancy->Fill(937); // number of chips with zero digits stored in the overflow bin
        }
        float num = hChipOccupancy->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hChipOccupancy->SetBinContent(iBin + 1, ratio);
      }
    }

    if (mo->getName() == "mDigitOccupancySummary") {
      auto* hOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());

      float den = hOccupancySummary->GetBinContent(0, 0); // normalisation stored in the uderflow bin
      float nEmptyBins = 0;                               // number of empty zones stored here

      for (int iBinX = 0; iBinX < hOccupancySummary->GetNbinsX(); iBinX++) {
        for (int iBinY = 0; iBinY < hOccupancySummary->GetNbinsY(); iBinY++) {
          float num = hOccupancySummary->GetBinContent(iBinX + 1, iBinY + 1);
          float ratio = (den > 0) ? (num / den) : 0.0;
          hOccupancySummary->SetBinContent(iBinX + 1, iBinY + 1, ratio);
          if ((hOccupancySummary->GetBinContent(iBinX + 1, iBinY + 1)) == 0) {
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
  for (int i = 0; i < calib->size(); i++) {
    if (calib->isFullChipMasked(i)) {
      mMaskedChips.push_back(i);
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

void QcMFTDigitCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // set up masking of dead chips once
  if (mFirstCall) {
    mFirstCall = false;
    readMaskedChips(mo);
    getChipMapData();
    createMaskedChipsNames();
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

  if (mo->getName().find("mDigitOccupancySummary") != std::string::npos) {
    auto* hOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());
    TPaveText* msg1 = new TPaveText(0.05, 0.9, 0.35, 1.0, "NDC NB");
    TPaveText* msg2 = new TPaveText(0.65, 0.9, 0.95, 1.0, "NDC NB");
    hOccupancySummary->GetListOfFunctions()->Add(msg1);
    hOccupancySummary->GetListOfFunctions()->Add(msg2);
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
