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
/// \file   PulseHeight.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TStopwatch.h>

#include "QualityControl/QcInfoLogger.h"
#include "TRD/PulseHeight.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <gsl/span>
#include <map>
#include <tuple>
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "CCDB/BasicCCDBManager.h"

namespace o2::quality_control_modules::trd
{

PulseHeight::~PulseHeight()
{
}

void PulseHeight::connectCCDB()
{
  auto& ccdbmgr = o2::ccdb::BasicCCDBManager::instance();
  //ccdbmgr.setURL("http://localhost:8080");
  mNoiseMap.reset(ccdbmgr.get<o2::trd::NoiseStatusMCM>("/TRD/Calib/NoiseMapMCM"));
}

void PulseHeight::buildHistograms()
{
  mPulseHeight.reset(new TH1F("mPulseHeight", "Pulse height plot", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mPulseHeight.get());
  mPulseHeight2.reset(new TH1F("mPulseHeight2", "Pulse height plot v2", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mPulseHeight2.get());
  mPulseHeight2n.reset(new TH1F("mPulseHeight2nonoise", "Pulse height plot v2 excluding noise", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mPulseHeight2n.get());

  mPulseHeightScaled.reset(new TH1F("mPulseHeightScaled", "Scaled Pulse height plot", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mPulseHeightScaled.get());
  mPulseHeightScaled2.reset(new TH1F("mPulseHeightScaled2", "Scaled Pulse height plot", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mPulseHeightScaled2.get());

  mTotalPulseHeight2D.reset(new TH2F("TotalPulseHeight", "Total Pulse Height", 30, 0., 30., 200, 0., 200.));
  getObjectsManager()->startPublishing(mTotalPulseHeight2D.get());
  getObjectsManager()->setDefaultDrawOptions(mTotalPulseHeight2D->GetName(), "COLZ");
  mTotalPulseHeight2D2.reset(new TH2F("TotalPulseHeight2", "Total Pulse Height", 30, 0., 30., 200, 0., 200.));
  getObjectsManager()->startPublishing(mTotalPulseHeight2D2.get());
  getObjectsManager()->setDefaultDrawOptions(mTotalPulseHeight2D2->GetName(), "COLZ");

  for (int count = 0; count < 18; ++count) {
    std::string label = fmt::format("pulseheight2d_sm_{0}", count);
    std::string title = fmt::format("Pulse Height Spectrum for SM {0}", count);
    TH1F* h = new TH1F(label.c_str(), title.c_str(), 30, -0.5, 29.5);
    mPulseHeight2DperSM[count].reset(h);
    getObjectsManager()->startPublishing(h);
    label = fmt::format("pulseheight2d2_sm_{0}", count);
    title = fmt::format("Pulse Height Spectrum v 2 for SM {0}", count);
    TH1F* h2 = new TH1F(label.c_str(), title.c_str(), 30, -0.5, 29.5);
    mPulseHeight2DperSM2[count].reset(h2);
    getObjectsManager()->startPublishing(h2);
    getObjectsManager()->setDefaultDrawOptions(h->GetName(), "COLZ");
    getObjectsManager()->setDefaultDrawOptions(h2->GetName(), "COLZ");
  }

  mPulseHeightDuration.reset(new TH1F("mPulseHeightDuration", "Pulse height duration", 10000, 0, 5.0));
  getObjectsManager()->startPublishing(mPulseHeightDuration.get());
  mPulseHeightDuration1.reset(new TH1F("mPulseHeightDuration1", "Pulse height duration v2", 10000, 0, 5.0));
  getObjectsManager()->startPublishing(mPulseHeightDuration1.get());
  mPulseHeightDurationDiff.reset(new TH1F("mPulseHeightDurationDiff", "Pulse height plot v2", 100000, -5.0, 5.0));
  getObjectsManager()->startPublishing(mPulseHeightDurationDiff.get());
}
void PulseHeight::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PulseHeight" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  if (auto param = mCustomParameters.find("peakregionstart"); param != mCustomParameters.end()) {
    mDriftRegion.first = stof(param->second);
    ILOG(Info, Support) << "configure() : using peakregionstart = " << mDriftRegion.first << ENDM;
  } else {
    mDriftRegion.first = 7.0;
    ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("peakregionend"); param != mCustomParameters.end()) {
    mDriftRegion.second = stof(param->second);
    ILOG(Info, Support) << "configure() : using peakregionstart = " << mDriftRegion.second << ENDM;
  } else {
    mDriftRegion.second = 20.0;
    ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("pulsheightpeaklower"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.first = stof(param->second);
    ILOG(Info, Support) << "configure() : using pulsehheightlower	= " << mPulseHeightPeakRegion.first << ENDM;
  } else {
    mPulseHeightPeakRegion.first = 1.0;
    ILOG(Info, Support) << "configure() : using default pulseheightlower = " << mPulseHeightPeakRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("pulsheightpeakupper"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.second = stof(param->second);
    ILOG(Info, Support) << "configure() : using pulsehheightupper	= " << mPulseHeightPeakRegion.second << ENDM;
  } else {
    mPulseHeightPeakRegion.second = 5.0;
    ILOG(Info, Support) << "configure() : using default pulseheightupper = " << mPulseHeightPeakRegion.second << ENDM;
  }
  buildHistograms();
  connectCCDB();
}

void PulseHeight::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
}

void PulseHeight::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

bool pulseheightdigitindexcompare(unsigned int A, unsigned int B, const std::vector<o2::trd::Digit>& originalDigits)
{
  // sort into ROC:padrow
  const o2::trd::Digit *a, *b;
  a = &originalDigits[A];
  b = &originalDigits[B];
  if (a->getDetector() < b->getDetector()) {
    return 1;
  }
  if (a->getDetector() > b->getDetector()) {
    return 0;
  }
  if (a->getPadRow() < b->getPadRow()) {
    return 1;
  }
  if (a->getPadRow() > b->getPadRow()) {
    return 0;
  }
  if (a->getPadCol() < b->getPadCol()) {
    return 1;
  }
  return 0;
}

void PulseHeight::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
  auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
  auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
  uint16_t tbsum[540][16][144];
  std::map<std::tuple<int, int, int>, o2::trd::ArrayADC> dataMap;
  uint64_t digitcount = 0;
  std::vector<int> digitIndex;
  auto start = std::chrono::steady_clock::now();
  int triggercount = 0;
  for (auto& trigger : triggerrecords) {
    uint64_t numtracklets = trigger.getNumberOfTracklets();
    uint64_t numdigits = trigger.getNumberOfDigits();

    int tbmax = 0;
    int tbhi = 0;
    int tblo = 0;

    int det = 0;
    int row = 0;
    int pad = 0;
    int channel = 0;

    memset(tbsum, 0, sizeof(tbsum));
    dataMap.clear();
    for (int i = trigger.getFirstDigit(); i < trigger.getFirstDigit() + trigger.getNumberOfDigits(); ++i) {

      //digit comes in sorted by hcid.
      //resort them by chamber,row, pad, this then removes the need for the map.
      //
      // loop over det, pad, row?
      channel = digits[i].getChannel();
      if (channel == 0 || channel == 1 || channel == 20) {
        continue;
      }

      auto adcs = digits[i].getADC();
      det = digits[i].getDetector();
      row = digits[i].getPadRow();
      pad = digits[i].getPadCol();
      int supermod = det / 30;

      tbsum[det][row][pad] = digits[i].getADCsum();

      if (tbsum[det][row][pad] >= 400) {
        dataMap.insert(std::make_pair(std::make_tuple(det, row, pad), adcs));
      } else {
        tbsum[det][row][pad] = 0;
      }
    } // end digitcont

    //std::cout << "start updating..... trigger:" << triggercount++ << " with " << trigger.getNumberOfDigits() << " digits " << std::endl;
    for (int d = 0; d < 540; d++) {
      int sector = d / 30;
      for (int r = 0; r < 16; r++) {
        for (int c = 2; c < 142; c++) {
          if (d == 50 && tbsum[d][r][c] > 0) {
            //std::cout << "updating on detector 50 " << d << " " << r << " " << c-1 << "("<<tbsum[d][r][c-1]<<") -- " << d << " " << r << " " << c << "("<<tbsum[d][r][c]<<") -- " << d << " " << r << " " << c+1 <<"("<< tbsum[d][r][c+1]<< ")" << std::endl;
          }
          if (tbsum[d][r][c] > tbsum[d][r][c - 1] && tbsum[d][r][c] > tbsum[d][r][c + 1]) {
            if (tbsum[d][r][c - 1] > tbsum[d][r][c + 1]) {
              tbmax = tbsum[d][r][c];
              tbhi = tbsum[d][r][c - 1];
              tblo = tbsum[d][r][c + 1];
              auto adcMax = dataMap.find(std::make_tuple(d, r, c));
              auto adcHi = dataMap.find(std::make_tuple(d, r, c - 1));
              auto adcLo = dataMap.find(std::make_tuple(d, r, c + 1));

              if (dataMap.find(std::make_tuple(d, r, c - 2)) == dataMap.end()) {
                if (tblo > 400) {
                  int phVal = 0;
                  //std::cout << "updatea " << d << " " << r << " " << c-1 << "("<<tbsum[d][r][c-1]<<") -- " << d << " " << r << " " << c << "("<<tbsum[d][r][c]<<") -- " << d << " " << r << " " << c+1 <<"("<< tbsum[d][r][c+1]<< ")" << std::endl;
                  for (int tb = 0; tb < 30; tb++) {
                    phVal = ((adcMax->second)[tb] + (adcHi->second)[tb] + (adcLo->second)[tb]);
                    //TODO do we have a corresponding tracklet?
                    mPulseHeight->Fill(tb, phVal);
                    mTotalPulseHeight2D->Fill(tb, phVal);
                    mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                  }
                }
              } else {
                auto adcHiNeighbour = dataMap.find(std::make_tuple(d, r, c - 2));
                if (tblo > 400) {
                  //std::cout << "updateb " << d << " " << r << " " << c-1 << "("<<tbsum[d][r][c-1]<<") -- " << d << " " << r << " " << c << "("<<tbsum[d][r][c]<<") -- " << d << " " << r << " " << c+1 <<"("<< tbsum[d][r][c+1]<< ")" << std::endl;
                  int phVal = 0;
                  for (int tb = 0; tb < 30; tb++) {
                    phVal = ((adcMax->second)[tb] + (adcHi->second)[tb] + (adcLo->second)[tb]);
                    mPulseHeight->Fill(tb, phVal);
                    mTotalPulseHeight2D->Fill(tb, phVal);
                    mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                  }
                }
              }
            } else {
              tbmax = tbsum[d][r][c];
              tbhi = tbsum[d][r][c + 1];
              tblo = tbsum[d][r][c - 1];
              auto adcMax = dataMap.find(std::make_tuple(d, r, c));
              auto adcHi = dataMap.find(std::make_tuple(d, r, c + 1));
              auto adcLo = dataMap.find(std::make_tuple(d, r, c - 1));
              if (dataMap.find(std::make_tuple(d, r, c + 2)) == dataMap.end()) {

                if (tblo > 400) {
                  //std::cout << "updatec " << d << " " << r << " " << c-1 << "("<<tbsum[d][r][c-1]<<") -- " << d << " " << r << " " << c << "("<<tbsum[d][r][c]<<") -- " << d << " " << r << " " << c+1 <<"("<< tbsum[d][r][c+1]<< ")" << std::endl;
                  int phVal = 0;
                  for (int tb = 0; tb < 30; tb++) {
                    phVal = ((adcMax->second)[tb] + (adcHi->second)[tb] + (adcLo->second)[tb]);
                    mPulseHeight->Fill(tb, phVal);
                    mTotalPulseHeight2D->Fill(tb, phVal);
                    mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                  }
                }
              } else {
                auto adcHiNeighbour = dataMap.find(std::make_tuple(d, r, c + 2));
                if (tblo > 400) {
                  //std::cout << "updated " << d << " " << r << " " << c-1 << "("<<tbsum[d][r][c-1]<<") -- " << d << " " << r << " " << c << "("<<tbsum[d][r][c]<<") -- " << d << " " << r << " " << c+1 <<"("<< tbsum[d][r][c+1]<< ")" << std::endl;
                  int phVal = 0;
                  for (int tb = 0; tb < 30; tb++) {
                    phVal = ((adcMax->second)[tb] + (adcHi->second)[tb] + (adcLo->second)[tb]);
                    mPulseHeight->Fill(tb, phVal);
                    mTotalPulseHeight2D->Fill(tb, phVal);
                    mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                  }
                }
              }
            } //end else
          }   // end if (tbsum[d][r][c]>tbsum[d][r][c-1] && tbsum[d][r][c]>tbsum[d][r][c+1])
        }     // end for c
      }       //end for r
    }         // end for d
    //std::cout << "finishedupdating....." << std::endl;
    dataMap.clear();
  } //end trigger event

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> pulseheightduration = end - start;
  LOG(info) << "Digits into pulsheight spectrum: " << digitcount;
  triggercount = 0;
  //alternate formulation:
  auto start1 = std::chrono::steady_clock::now();
  std::vector<o2::trd::Digit> digitv(digits.begin(), digits.end());
  std::vector<unsigned int> digitsIndex(digitv.size());
  std::iota(digitsIndex.begin(), digitsIndex.end(), 0);

  for (auto& trigger : triggerrecords) {
    uint64_t numtracklets = trigger.getNumberOfTracklets();
    uint64_t numdigits = trigger.getNumberOfDigits();

    int tbmax = 0;
    int tbhi = 0;
    int tblo = 0;

    int det = 0;
    int row = 0;
    int pad = 0;
    int channel = 0;

    memset(tbsum, 0, sizeof(tbsum));

    if (digitv.size() == 0)
      continue;

    if (trigger.getNumberOfDigits() == 0)
      continue; //bail if we have no digits in this trigger
    //now sort digits to det,row,pad
    std::sort(std::begin(digitsIndex) + trigger.getFirstDigit(), std::begin(digitsIndex) + trigger.getFirstDigit() + trigger.getNumberOfDigits(),
              [&digitv](unsigned int i, unsigned int j) { return pulseheightdigitindexcompare(i, j, digitv); });
    //std::cout << " staring updating second ... for trigger:" << triggercount++ << " with " << trigger.getNumberOfDigits() << " digits" << std::endl;
    for (int currentdigit = trigger.getFirstDigit() + 1; currentdigit < trigger.getFirstDigit() + trigger.getNumberOfDigits() - 1; ++currentdigit) { // -1 and +1 as we are looking for consecutive digits pre and post the current one indexed.
      int detector = digits[digitsIndex[currentdigit]].getDetector();
      auto adcs = digits[digitsIndex[currentdigit]].getADC();
      det = digits[digitsIndex[currentdigit]].getDetector();
      row = digits[digitsIndex[currentdigit]].getPadRow();
      pad = digits[digitsIndex[currentdigit]].getPadCol();
      int supermod = detector / 30;
      int sector = detector / 30;
      int detLoc = detector % 30;
      int layer = detector % 6;
      int istack = detLoc / 6;
      int iChamber = supermod * 30 + istack * o2::trd::constants::NLAYER + layer;
      int nADChigh = 0;
      //do we have 3 digits next to each other:
      std::tuple<unsigned int, unsigned int, unsigned int> aa, ba, ca;
      aa = std::make_tuple(digits[digitsIndex[currentdigit - 1]].getDetector(), digits[digitsIndex[currentdigit - 1]].getPadRow(), digits[digitsIndex[currentdigit - 1]].getPadCol());
      ba = std::make_tuple(digits[digitsIndex[currentdigit]].getDetector(), digits[digitsIndex[currentdigit]].getPadRow(), digits[digitsIndex[currentdigit]].getPadCol());
      ca = std::make_tuple(digits[digitsIndex[currentdigit + 1]].getDetector(), digits[digitsIndex[currentdigit + 1]].getPadRow(), digits[digitsIndex[currentdigit + 1]].getPadCol());
      auto [det1, row1, col1] = aa;
      auto [det2, row2, col2] = ba;
      auto [det3, row3, col3] = ca;
      // check we have 3 consecutive adc
      // if (det1 == det2 && det2 == det3 && row1 == row2 && row2 == row3 && col1 + 1 == col2 && col2 + 1 == col3) {

      const o2::trd::Digit* b = &digits[digitsIndex[currentdigit]];
      const o2::trd::Digit* a = &digits[digitsIndex[currentdigit - 1]];
      const o2::trd::Digit* c = &digits[digitsIndex[currentdigit + 1]];
      uint32_t suma = a->getADCsum();
      uint32_t sumb = b->getADCsum();
      uint32_t sumc = c->getADCsum();
      if (det2 == 50) {
        //std::cout << "on detector 50 " << det1 << " " << row1 << " " << col1 << "("<<suma<<") -- " << det2 << " " << row2 << " " << col2 << "("<<sumb<<") -- " << det3 << " " << row3 << " " << col3 <<"("<< sumc<< ")" << std::endl;
      }
      if (det1 == det2 && det2 == det3 && row1 == row2 && row2 == row3 && col1 + 1 == col2 && col2 + 1 == col3) {
        if (sumb > suma && sumb > sumc) {
          if (suma > sumc) {
            tbmax = sumb;
            tbhi = suma;
            tblo = sumc;
            if (tblo > 400) {
              int phVal = 0;
              //std::cout << "updating2a " << det1 << " " << row1 << " " << col1 << " -- " << det2 << " " << row2 << " " << col2 << " -- " << det3 << " " << row3 << " " << col3 << std::endl;
              for (int tb = 0; tb < 30; tb++) {
                phVal = (b->getADC()[tb] + a->getADC()[tb] + c->getADC()[tb]);
                //TODO do we have a corresponding tracklet?
                mPulseHeight2->Fill(tb, phVal);
                if (!mNoiseMap.get()->getIsNoisy(b->getHCId(), b->getROB(), b->getMCM()))
                  mPulseHeight2n->Fill(tb, phVal);
                mTotalPulseHeight2D2->Fill(tb, phVal);
                mPulseHeight2DperSM2[sector]->Fill(tb, phVal);
              }
            }
          } else {
            tbmax = sumb;
            tblo = suma;
            tbhi = sumc;
            if (tblo > 400) {
              int phVal = 0;
              //std::cout << "updating2b " << det1 << " " << row1 << " " << col1 << " -- " << det2 << " " << row2 << " " << col2 << " -- " << det3 << " " << row3 << " " << col3 << std::endl;
              for (int tb = 0; tb < 30; tb++) {
                phVal = (b->getADC()[tb] + a->getADC()[tb] + c->getADC()[tb]);
                mPulseHeight2->Fill(tb, phVal);
                if (!mNoiseMap.get()->getIsNoisy(b->getHCId(), b->getROB(), b->getMCM()))
                  mPulseHeight2n->Fill(tb, phVal);
                mTotalPulseHeight2D2->Fill(tb, phVal);
                mPulseHeight2DperSM2[sector]->Fill(tb, phVal);
              }
            }
          } //end else
        }   // end if (tbsum[d][r][c]>tbsum[d][r][c-1] && tbsum[d][r][c]>tbsum[d][r][c+1])
      }     // end for c
    }       //end for r
    //std::cout << " finished updating second ... " << std::endl;
  } // end for d
  auto end1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> pulseheightduration1 = end1 - start1;

  //plot the 2 pulseheight durations and the difference.
  mPulseHeightDuration->Fill(pulseheightduration.count());
  mPulseHeightDuration1->Fill(pulseheightduration1.count());
  mPulseHeightDurationDiff->Fill(pulseheightduration.count() - pulseheightduration1.count());
}

void PulseHeight::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  double scale = mPulseHeight->GetEntries() / 30;
  for (int i = 0; i < 30; ++i)
    mPulseHeightScaled->SetBinContent(i, mPulseHeight->GetBinContent(i));
  mPulseHeightScaled->Scale(1 / scale);
}

void PulseHeight::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void PulseHeight::reset()
{
  // clean all the monitor objects here
  mTotalPulseHeight2D->Reset();
  mPulseHeight->Reset();

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
}

} // namespace o2::quality_control_modules::trd
