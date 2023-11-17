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

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TMath.h>

#include "DataFormatsTRD/Constants.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/HelperMethods.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"
#include "TRD/DigitsTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <gsl/span>
#include <map>
#include <tuple>

using namespace o2::quality_control_modules::common;
using namespace o2::trd::constants;
using Helper = o2::trd::HelperMethods;

namespace o2::quality_control_modules::trd
{

void DigitsTask::buildChamberIgnoreBP()
{
  mChambersToIgnoreBP.reset();
  // Vector of string to save tokens
  std::vector<std::string> tokens;
  // stringstream class check1
  std::stringstream check1(mChambersToIgnore);
  std::string intermediate;
  // Tokenizing w.r.t. space ','
  while (getline(check1, intermediate, ',')) {
    tokens.push_back(intermediate);
  }
  // Printing the token vector
  for (auto& token : tokens) {
    // token now holds something like 16_3_0
    std::vector<std::string> parts;
    std::stringstream indexcheck(token);
    std::string tokenpart;
    // Tokenizing w.r.t. space ','
    while (getline(indexcheck, tokenpart, '_')) {
      parts.push_back(tokenpart);
    }
    // now flip the bit related to the sector:stack:layer stored in parts.
    int sector = std::stoi(parts[0]);
    int stack = std::stoi(parts[1]);
    int layer = std::stoi(parts[2]);
    mChambersToIgnoreBP.set(Helper::getDetector(sector, stack, layer));
  }
}

void DigitsTask::drawTrdLayersGrid(TH2F* hist)
{
  TLine* line;
  for (int i = 0; i < 5; ++i) {
    switch (i) {
      case 0:
        line = new TLine(15.5, 0, 15.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 1:
        line = new TLine(31.5, 0, 31.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 2:
        line = new TLine(43.5, 0, 43.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 3:
        line = new TLine(59.5, 0, 59.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
    }
  }
  for (int iSec = 1; iSec < 18; ++iSec) {
    float yPos = iSec * 144 - 0.5;
    line = new TLine(0, yPos, 76, yPos);
    line->SetLineStyle(kDashed);
    line->SetLineColor(kBlack);
    hist->GetListOfFunctions()->Add(line);
  }

  ILOG(Info, Support) << "Layer Grid redrawn in check for : " << hist->GetName() << ENDM;
  ILOG(Info, Support) << "Layer Grid redrawn in hist has function count of :  " << hist->GetListOfFunctions()->GetSize() << ENDM;
}

void DigitsTask::drawLinesOnPulseHeight(TH1F* h)
{
  TLine* lmin = new TLine(mPulseHeightPeakRegion.first, 0, mPulseHeightPeakRegion.first, 1e9);
  TLine* lmax = new TLine(mPulseHeightPeakRegion.second, 0, mPulseHeightPeakRegion.second, 1e9);
  lmin->SetLineStyle(2);
  lmax->SetLineStyle(2);
  lmin->SetLineColor(kRed);
  lmax->SetLineColor(kRed);
  h->GetListOfFunctions()->Add(lmin);
  h->GetListOfFunctions()->Add(lmax);
}

void DigitsTask::drawHashOnLayers(int layer, int hcid, int col, int rowstart, int rowend)
{
  // instead of using overlays, draw a simple box in red with a cross on it.
  std::pair<float, float> topright, bottomleft; // coordinates of box
  TLine* boxlines[8];
  int det = hcid / 2;
  int side = hcid % 2;
  int sec = hcid / 60;
  bottomleft.first = rowstart - 0.5;
  bottomleft.second = (sec * 2 + side) * 72;
  topright.first = rowend - 0.5;
  topright.second = (sec * 2 + side) * 72 + 72;

  // LOG(info) << "Box for layer : " << layer << " hcid : " << hcid << ": " << bottomleft.first << ":" << bottomleft.second << "(" << bottomleft.second/144 << ") -- " << topright.first << ":" << topright.second << "(" << topright.second/144 << ")";
  boxlines[0] = new TLine(bottomleft.first, bottomleft.second, topright.first, bottomleft.second);                                                                                     // bottom
  boxlines[1] = new TLine(bottomleft.first, topright.second, topright.first, topright.second);                                                                                         // top
  boxlines[2] = new TLine(bottomleft.first, bottomleft.second, bottomleft.first, topright.second);                                                                                     // left
  boxlines[3] = new TLine(topright.first, bottomleft.second, topright.first, topright.second);                                                                                         // right
  boxlines[4] = new TLine(bottomleft.first, topright.second - (topright.second - bottomleft.second) / 2, topright.first, topright.second - (topright.second - bottomleft.second) / 2); // horizontal middle
  boxlines[5] = new TLine(topright.first, bottomleft.second, bottomleft.first, topright.second);                                                                                       // backslash
  boxlines[6] = new TLine(bottomleft.first, bottomleft.second, topright.first, topright.second);                                                                                       // forwardslash
  boxlines[7] = new TLine(bottomleft.first + (topright.first - bottomleft.first) / 2, bottomleft.second, bottomleft.first + (topright.first - bottomleft.first) / 2, topright.second); // vertical middle
  for (int line = 0; line < 8; ++line) {
    boxlines[line]->SetLineColor(kBlack);
    mLayers[layer]->GetListOfFunctions()->Add(boxlines[line]);
  }
}

void DigitsTask::fillLinesOnHistsPerLayer(int iLayer)
{
  std::bitset<MAXHALFCHAMBER> hciddone;
  hciddone.reset();
  if (mChamberStatus == nullptr) {
    // protect for if the chamber status is not pulled from the ccdb for what ever reason.
    ILOG(Info, Support) << "No half chamber status object to be able to fill the mask" << ENDM;
  } else {
    for (int iSec = 0; iSec < 18; ++iSec) {
      for (int iStack = 0; iStack < 5; ++iStack) {
        int rowMax = (iStack == 2) ? 12 : 16;
        for (int side = 0; side < 2; ++side) {
          int det = iSec * 30 + iStack * 6 + iLayer;
          int hcid = (side == 0) ? det * 2 : det * 2 + 1;
          int rowstart = iStack < 3 ? iStack * 16 : 44 + (iStack - 3) * 16;                 // pad row within whole sector
          int rowend = iStack < 3 ? rowMax + iStack * 16 : rowMax + 44 + (iStack - 3) * 16; // pad row within whole sector
          if (mChamberStatus->isMasked(hcid) && (!hciddone.test(hcid))) {
            drawHashOnLayers(iLayer, hcid, 0, rowstart, rowend);
            hciddone.set(hcid); // to avoid mutliple instances of adding lines
          }
        }
      }
    }
  }
}

void DigitsTask::buildHistograms()
{
  constexpr int nLogBins = 100;
  float xBins[nLogBins + 1];
  float xBinLogMin = 0.f;
  float xBinLogMax = 8.f;
  float logBinWidth = (xBinLogMax - xBinLogMin) / nLogBins;
  for (int iBin = 0; iBin <= nLogBins; ++iBin) {
    xBins[iBin] = TMath::Power(10, xBinLogMin + iBin * logBinWidth);
  }
  mDigitsPerEvent.reset(new TH1F("digitsperevent", "Number of digits per Event", nLogBins, xBins));
  getObjectsManager()->startPublishing(mDigitsPerEvent.get());

  mDigitHCID.reset(new TH1F("digithcid", "Digit distribution over Halfchambers", 1080, -0.5, 1079.5));
  getObjectsManager()->startPublishing(mDigitHCID.get());

  mDigitsSizevsTrackletSize.reset(new TH1F("digitsvstracklets", "Digit count vs. Tracklet count; N_Digits / N_Tracklets;counts", 100, 0, 100));
  getObjectsManager()->startPublishing(mDigitsSizevsTrackletSize.get());

  mADCvalue.reset(new TH1F("ADCvalue", "ADC values for all digits;ADC value;Counts", 1024, -0.5, 1023.5));
  getObjectsManager()->startPublishing(mADCvalue.get());

  for (Int_t iSM = 0; iSM < NSECTOR; ++iSM) {
    mHCMCM[iSM].reset(new TH2F(Form("DigitsPerMCM_%i", iSM), Form("Digits per MCM in sector %i;pad row; MCM column", iSM), 76, -0.5, 75.5, 8, -0.5, 7.5));
    getObjectsManager()->startPublishing(mHCMCM[iSM].get());
    getObjectsManager()->setDefaultDrawOptions(mHCMCM[iSM]->GetName(), "COLZ");
  }

  if (mDoClusterize) {
    mClsNTb.reset(new TH1F("Cluster/ClsNTb", "ClsNTb", 30, -0.5, 29.5));
    mClsNTb->SetTitle("Clusters per time bin;Number of clusters;Timebin");
    getObjectsManager()->startPublishing(mClsNTb.get());
    mClsChargeTb.reset(new TH1F("Cluster/ClsChargeTb", "ClsChargeTb;", 30, -0.5, 29.5));
    getObjectsManager()->startPublishing(mClsChargeTb.get());
    mClsAmp.reset(new TH1F("Cluster/ClsAmp", "Amplitude of clusters;Amplitdu(ADC);Counts", 200, -0.5, 1999.5));
    getObjectsManager()->startPublishing(mClsAmp.get());
    mNCls.reset(new TH1F("Cluster/NCls", "Total number of clusters per sector", 18, -0.5, 17.5));
    mNCls->SetTitle("Total number of clusters per sector;Sector;Counts");
    getObjectsManager()->startPublishing(mNCls.get());
    mClsTb.reset(new TH2F("Cluster/ChargeTB", "Cluster charge;time bin;cluster charge", 30, -0.5, 29.5, 200, 0, 2000));
    getObjectsManager()->startPublishing(mClsTb.get());
  }

  mPulseHeight.reset(new TH1F("PulseHeight1D", "PH spectrum 1D;time bin;ADC sum", 30, -0.5, 29.5));
  drawLinesOnPulseHeight(mPulseHeight.get());
  getObjectsManager()->startPublishing(mPulseHeight.get());
  mPulseHeight.get()->GetYaxis()->SetTickSize(0.01);

  mTotalPulseHeight2D.reset(new TH2F("PulseHeight2D", "PH spectrum 2D;time bin;ADC sum", 30, 0., 30., 200, 0., 200.));
  getObjectsManager()->startPublishing(mTotalPulseHeight2D.get());
  getObjectsManager()->setDefaultDrawOptions(mTotalPulseHeight2D->GetName(), "COLZ");

  mPulseHeightpro.reset(new TProfile("PulseHeightProfile", "PH spectrum for all chambers combined;time bin;ADC sum", 30, -0.5, 29.5));
  mPulseHeightpro.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightpro.get());

  mPulseHeightperchamber.reset(new TProfile2D("PulseHeightPerChamber", "PH spectrum for all chambers;time bin;chamber", 30, -0.5, 29.5, 540, -0.5, 539.5));
  mPulseHeightperchamber.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightperchamber.get());
  getObjectsManager()->setDefaultDrawOptions(mPulseHeightperchamber.get()->GetName(), "colz");

  for (int iSec = 0; iSec < NSECTOR; ++iSec) {
    mPulseHeight2DperSM[iSec].reset(new TH1F(Form("PulseHeight_%i", iSec), Form("PH spectrum for sector %i;time bin;ADC sum count", iSec), 30, -0.5, 29.5));
    getObjectsManager()->startPublishing(mPulseHeight2DperSM[iSec].get());
  }

  for (int iLayer = 0; iLayer < NLAYER; ++iLayer) {
    mLayers[iLayer].reset(new TH2F(Form("DigitsPerLayer_%i", iLayer), Form("Digit count per pad in layer %i;glb pad row;glb pad col", iLayer), 76, -0.5, 75.5, 2592, -0.5, 2591.5));
    mLayers[iLayer]->SetStats(0);
    drawTrdLayersGrid(mLayers[iLayer].get());
    fillLinesOnHistsPerLayer(iLayer);
    getObjectsManager()->startPublishing(mLayers[iLayer].get());
    getObjectsManager()->setDefaultDrawOptions(mLayers[iLayer]->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers[iLayer].get(), "logz");
  }
}

void DigitsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TRDDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  mDoClusterize = getFromConfig<bool>(mCustomParameters, "doClusterize", false);
  mPulseHeightPeakRegion.first = getFromConfig<float>(mCustomParameters, "peakRegionStart", 0.f);
  mPulseHeightPeakRegion.second = getFromConfig<float>(mCustomParameters, "peakRegionEnd", 5.f);
  mPulseHeightThreshold = getFromConfig<unsigned int>(mCustomParameters, "phThreshold", 400u);
  mChambersToIgnore = getFromConfig<string>(mCustomParameters, "ignoreChambers", "16_3_0");
  mClsCutoff = getFromConfig<int>(mCustomParameters, "clsCutoff", 1000);
  mAdcBaseline = getFromConfig<int>(mCustomParameters, "adcBaseline", 10);

  buildChamberIgnoreBP();
  buildHistograms();
}

void DigitsTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
} // set stats/stacs

void DigitsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void DigitsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // Load CCDB objects (needs to be done only once)
  if (!mNoiseMap) {
    auto ptr = ctx.inputs().get<o2::trd::NoiseStatusMCM*>("noiseMap");
    mNoiseMap = ptr.get();
  }
  if (!mChamberStatus) {
    auto ptr = ctx.inputs().get<o2::trd::HalfChamberStatusQC*>("chamberStatus");
    mChamberStatus = ptr.get();
  }

  auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
  auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
  for (auto& trigger : triggerrecords) {
    auto nDigits = trigger.getNumberOfDigits();
    auto iDigitFirst = trigger.getFirstDigit();
    int iDigitLast = iDigitFirst + nDigits;
    if (nDigits == 0) {
      continue;
    }
    mDigitsPerEvent->Fill(nDigits);
    mDigitsSizevsTrackletSize->Fill(nDigits / trigger.getNumberOfTracklets());

    for (int iDigit = iDigitFirst; iDigit < iDigitLast; ++iDigit) {
      const auto& digit = digits[iDigit];
      if (digit.isSharedDigit()) {
        continue;
      }
      int detector = digit.getDetector();
      int sector = Helper::getSector(detector);
      int layer = Helper::getLayer(detector);
      int stack = Helper::getStack(detector);
      int row = digit.getPadRow();
      int col = digit.getPadCol();
      int rowGlb = FIRSTROW[stack] + row;
      int colGlb = col + sector * NCOLUMN;
      mLayers[layer]->Fill(rowGlb, colGlb);
      mHCMCM[sector]->Fill(rowGlb, digit.getMCMCol());
      mDigitHCID->Fill(digit.getHCId());
      for (auto adc : digit.getADC()) {
        mADCvalue->Fill(adc);
      }
      if (iDigit == iDigitFirst || iDigit == iDigitLast - 1) {
        // above histograms are filled for all digits, now we are looking for clusters
        continue;
      }

      const auto* digitLeft = &digits[iDigit + 1];
      const auto* digitRight = &digits[iDigit - 1];
      // due to shared digits the neighbouring element might still be in the same pad column
      if (digitLeft->getPadCol() == col && iDigit < iDigitLast - 2) {
        digitLeft = &(digits[iDigit + 2]);
      }
      if (digitRight->getPadCol() == col && iDigit > iDigitFirst + 2) {
        digitRight = &(digits[iDigit - 2]);
      }
      if (!digitLeft->isNeighbour(digit) || !digitRight->isNeighbour(digit)) {
        continue;
      }

      if (mDoClusterize) {
        for (int tb = 1; tb < TIMEBINS - 1; ++tb) {
          auto adc = digit.getADC()[tb];
          if (adc > digitLeft->getADC()[tb] && adc > digitRight->getADC()[tb]) {
            // local maximum
            int valueLU = digitLeft->getADC()[tb - 1] > mAdcBaseline ? digitLeft->getADC()[tb - 1] - mAdcBaseline : 0;
            int valueRU = digitRight->getADC()[tb - 1] > mAdcBaseline ? digitRight->getADC()[tb - 1] - mAdcBaseline : 0;
            int valueU = digit.getADC()[tb - 1] - mAdcBaseline; // why not protect against negative values here?

            int valueLD = digitLeft->getADC()[tb + 1] > mAdcBaseline ? digitLeft->getADC()[tb + 1] - mAdcBaseline : 0;
            int valueRD = digitRight->getADC()[tb + 1] > mAdcBaseline ? digitRight->getADC()[tb + 1] - mAdcBaseline : 0;
            int valueD = digit.getADC()[tb + 1] - mAdcBaseline; // same question as above

            int valueL = digitLeft->getADC()[tb] > mAdcBaseline ? digitLeft->getADC()[tb] - mAdcBaseline : 0;
            int valueR = digitRight->getADC()[tb] > mAdcBaseline ? digitRight->getADC()[tb] - mAdcBaseline : 0;

            int sum = adc + valueL + valueR;
            int sumU = valueU + valueLU + valueRU;
            int sumD = valueD + valueLD + valueRD;

            if (sumU < mAdcBaseline || sumD < mAdcBaseline) {
              continue;
            }
            if (TMath::Abs(1. * sum / sumU - 1) < 0.01) {
              continue;
            }
            if (TMath::Abs(1. * sum / sumD - 1) < 0.01) {
              continue;
            }
            if (TMath::Abs(1. * sumU / sumD - 1) < 0.01) {
              continue;
            }
            mNCls->Fill(sector);
            mClsTb->Fill(tb, sum);
            mClsAmp->Fill(sum);
            if (sum > mAdcBaseline && sum < mClsCutoff) {
              mClsChargeTb->Fill(tb, sum);
              mClsNTb->Fill(tb);
            }
          } // local maximum
        }   // loop over time bins
      }

      auto sumLeft = digitLeft->getADCsum();
      auto sumCenter = digit.getADCsum();
      auto sumRight = digitRight->getADCsum();
      if (!mChambersToIgnoreBP.test(detector)) {
        if (sumCenter > sumLeft && sumCenter > sumRight) {
          auto lowestSum = (sumLeft > sumRight) ? sumRight : sumLeft;
          if (lowestSum > mPulseHeightThreshold) {
            for (int tb = 0; tb < TIMEBINS; tb++) {
              int phVal = (digit.getADC()[tb] + digitLeft->getADC()[tb] + digitRight->getADC()[tb]);
              mPulseHeight->Fill(tb, phVal);
              mTotalPulseHeight2D->Fill(tb, phVal);
              mPulseHeight2DperSM[sector]->Fill(tb, phVal);
              mPulseHeightpro->Fill(tb, phVal);
              mPulseHeightperchamber->Fill(tb, detector, phVal);
            } // loop over time bins
          }   // lower ADC sum above threshold
        }     // local ADC maximum
      }       // chamber OK
    }         // for loop over digits
  }           // loop over triggers
}

void DigitsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void DigitsTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void DigitsTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histogram" << ENDM;
  mDigitsPerEvent->Reset();
  mDigitHCID->Reset();
  mADCvalue->Reset();
  mDigitsSizevsTrackletSize->Reset();
  mTotalPulseHeight2D->Reset();
  mPulseHeight->Reset();
  mPulseHeightpro->Reset();
  mPulseHeightperchamber->Reset();
  for (auto& h : mPulseHeight2DperSM) {
    h->Reset();
  }
  for (auto& h : mHCMCM) {
    h->Reset();
  }
  for (auto& h : mLayers) {
    h->Reset();
  }
  if (mDoClusterize) {
    mNCls->Reset();
    mClsTb->Reset();
    mClsChargeTb->Reset();
    mClsNTb->Reset();
    mClsAmp->Reset();
  }
}
} // namespace o2::quality_control_modules::trd
