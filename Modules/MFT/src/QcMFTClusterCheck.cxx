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
/// \file   QcMFTClusterCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
/// \author David Grund
/// \author Sara Haidlova
/// \author Jakub Juracka
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
#include "MFT/QcMFTClusterCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTUtilTables.h"
#include "QualityControl/UserCodeInterface.h"
#include "QualityControl/CustomParameters.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTClusterCheck::configure()
{

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("LadderThresholdMedium"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - LadderThresholdMedium: " << param->second << ENDM;
    mLadderThresholdMedium = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("LadderThresholdBad"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - LadderThresholdBad: " << param->second << ENDM;
    mLadderThresholdBad = stoi(param->second);
  }

  // no call to beautifier yet
  mFirstCall = true;
  mEmptyCount = 0;
  mAdjacentLaddersEmpty = false;
}

Quality QcMFTClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  bool isEmpty = true;
  int adjacentCount = 0;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mFirstCall) {
      mFirstCall = false;
      readMaskedChips(mo);
      getChipMapData();
      createMaskedChipsNames();
    }

    if (mo->getName() == "mClusterOccupancy") {
      auto* hClusterOccupancy = dynamic_cast<TH1F*>(mo->getObject());
      if (hClusterOccupancy == nullptr) {
        ILOG(Error, Support) << "Could not cast mClusterOccupancy to TH1F." << ENDM;
        return Quality::Null;
      }

      for (int iBin = 0; iBin < hClusterOccupancy->GetNbinsX(); iBin++) {
        if (hClusterOccupancy->GetBinContent(iBin + 1) == 0) {
          hClusterOccupancy->Fill(937); // number of chips with zero clusters stored in the overflow bin
        }
      }
    }

    if (mo->getName() == "mClusterPatternIndex") {
      auto* hClusterPatternIndex = dynamic_cast<TH1F*>(mo->getObject());
      if (hClusterPatternIndex == nullptr) {
        ILOG(Error, Support) << "Could not cast mClusterPatternIndex to TH1F." << ENDM;
        return Quality::Null;
      }
    }

    if (mo->getName() == "mClusterSizeSummary") {
      auto* hClusterSizeSummary = dynamic_cast<TH1F*>(mo->getObject());
      if (hClusterSizeSummary == nullptr) {
        ILOG(Error, Support) << "Could not cast hClusterSizeSummary to TH1F." << ENDM;
        return Quality::Null;
      }
    }

    if (mo->getName() == "mGroupedClusterSizeSummary") {
      auto* hGroupedClusterSizeSummary = dynamic_cast<TH1F*>(mo->getObject());
      if (hGroupedClusterSizeSummary == nullptr) {
        ILOG(Error, Support) << "Could not cast hGroupedClusterSizeSummary to TH1F." << ENDM;
        return Quality::Null;
      }
    }
    // checker for empty ladders
    QcMFTUtilTables MFTTable;
    for (int i = 0; i < 20; i++) {
      if (mo->getName() == MFTTable.mClusterChipMapNames[i]) {
        adjacentCount = 0;
        auto* hClusterChipOccupancyMap = dynamic_cast<TH2F*>(mo->getObject());
        if (hClusterChipOccupancyMap == nullptr) {
          return Quality::Null;
        }
        // loop over bins in occupancy maps
        for (int iBinX = 0; iBinX < hClusterChipOccupancyMap->GetNbinsX(); iBinX++) {
          int emptyValidChips = 0;
          bool hasNonEmptyChip = false;

          for (int iBinY = 0; iBinY < hClusterChipOccupancyMap->GetNbinsY(); iBinY++) {
            // Check if the bin contains data before further checks
            if (hClusterChipOccupancyMap->GetBinContent(iBinX + 1, iBinY + 1) != 0) {
              hasNonEmptyChip = true;
              break; // Exit early if a non-empty chip is found (most of them should be non-empty)
            }

            bool isMasked = false;
            bool isOutsideAcc = false;

            // Check if chip is outside acceptance
            for (int k = 0; k < 21; k++) {
              if (mo->getName().find(MFTTable.mClusterChipMapNames[i]) != std::string::npos) {
                if (iBinX + 1 == MFTTable.mBinX[i][k] && iBinY + 1 == MFTTable.mBinY[i][k]) {
                  isOutsideAcc = true;
                  break;
                }
              }
            }

            // Check if chip is masked if it is in detector acceptance
            if (!isOutsideAcc) {
              for (int j = 0; j < mMaskedChips.size(); j++) {
                if (mo->getName().find(mChipMapName[j]) != std::string::npos) {
                  int maskedX = hClusterChipOccupancyMap->GetXaxis()->FindBin(mX[mMaskedChips[j]]);
                  int maskedY = hClusterChipOccupancyMap->GetYaxis()->FindBin(mY[mMaskedChips[j]]);
                  if (iBinX + 1 == maskedX && iBinY + 1 == maskedY) {
                    isMasked = true;
                    break; // break the loop if you find the bin in the masked list
                  }
                }
              }
            }

            // If chip is not masked and not outside acceptance, count it
            if (!isMasked && !isOutsideAcc) {
              emptyValidChips++;
            }
          }

          // Determine if column is empty
          isEmpty = (emptyValidChips > 0 && !hasNonEmptyChip);

          if (isEmpty) {
            mEmptyCount++;
            adjacentCount++;
          } else {
            adjacentCount = 0;
          }

          if (adjacentCount >= mLadderThresholdBad) {
            mAdjacentLaddersEmpty = true;
          }
        }
      }
    }

    if (mo->getName() == "mClusterOccupancySummary") {
      auto* hClusterOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());
      if (hClusterOccupancySummary == nullptr) {
        ILOG(Error, Support) << "Could not cast hClusterOccupancySummary to TH2F." << ENDM;
        return Quality::Null;
      }

      if (mAdjacentLaddersEmpty) {
        result = Quality::Bad;
      } else if (mEmptyCount >= mLadderThresholdMedium) {
        result = Quality::Medium;
      } else {
        result = Quality::Good;
      }
      // We rely on 'mClusterOccupancySummary' being run after chip maps in the list of MOs in the config file
      mEmptyCount = 0;
      mAdjacentLaddersEmpty = false;
    }
  }
  return result;
}

void QcMFTClusterCheck::readMaskedChips(std::shared_ptr<MonitorObject> mo)
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

void QcMFTClusterCheck::getChipMapData()
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

void QcMFTClusterCheck::createMaskedChipsNames()
{
  for (int i = 0; i < mMaskedChips.size(); i++) {
    mChipMapName.push_back(Form("ChipOccupancyMaps/Half_%d/Disk_%d/Face_%d/mClusterChipOccupancyMap",
                                mHalf[mMaskedChips[i]], mDisk[mMaskedChips[i]], mFace[mMaskedChips[i]]));
  }
}

void QcMFTClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  // print skull in maps to display dead chips
  int nMaskedChips = mMaskedChips.size();
  for (int i = 0; i < nMaskedChips; i++) {
    if (mo->getName().find(mChipMapName[i]) != std::string::npos) {
      auto* hMap = dynamic_cast<TH2F*>(mo->getObject());
      int binCx = hMap->GetXaxis()->FindBin(mX[mMaskedChips[i]]);
      int binCy = hMap->GetYaxis()->FindBin(mY[mMaskedChips[i]]);
      TLatex* tl = new TLatex(hMap->GetXaxis()->GetBinCenter(binCx), hMap->GetYaxis()->GetBinCenter(binCy), "N");
      tl->SetTextAlign(22);
      tl->SetTextFont(142);
      tl->SetTextSize(0.08);
      hMap->GetListOfFunctions()->Add(tl);
    }
  }

  QcMFTUtilTables MFTTable;
  for (int iHalf = 0; iHalf < 2; iHalf++) {
    for (int iDisk = 0; iDisk < 5; iDisk++) {
      for (int iFace = 0; iFace < 2; iFace++) {
        int idx = (iDisk * 2 + iFace) + (10 * iHalf);
        if (mo->getName().find(MFTTable.mClusterChipMapNames[idx]) != std::string::npos) {
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
          }
        }
      }
    }
  }

  if (mo->getName().find("mClusterOccupancySummary") != std::string::npos) {
    auto* hOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());
    TPaveText* msg1 = new TPaveText(0.05, 0.9, 0.35, 1.0, "NDC NB");
    TPaveText* msg2 = new TPaveText(0.65, 0.9, 0.95, 1.0, "NDC NB");
    hOccupancySummary->GetListOfFunctions()->Add(msg1);
    hOccupancySummary->GetListOfFunctions()->Add(msg2);
    msg1->SetName(Form("%s_msg1", mo->GetName()));
    msg2->SetName(Form("%s_msg2", mo->GetName()));
    if (checkResult == Quality::Good) {
      LOG(info) << "Quality::Good";
      msg1->Clear();
      msg1->AddText("Quality Good");
      msg1->SetFillColor(kGreen);
      msg2->Clear();
      msg2->AddText("No action needed");
      msg2->SetFillColor(kGreen);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::Medium";
      msg1->Clear();
      msg1->AddText("Quality medium");
      msg1->SetFillColor(kOrange);
      msg2->Clear();
      msg2->AddText("Write a logbook entry tagging MFT");
      msg2->SetFillColor(kOrange);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad";
      msg1->Clear();
      msg1->AddText("Quality bad");
      msg1->SetFillColor(kRed);
      msg2->Clear();
      msg2->AddText("Call the on-call!");
      msg2->SetFillColor(kRed);
    }
  }
}

} // namespace o2::quality_control_modules::mft
