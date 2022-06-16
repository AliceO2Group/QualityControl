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
/// \file   DigitQcTask.cxx
/// \author My Name
/// LATEST modification for FDD on 24.05.2022 (akhuntia@cern.ch)

#include <TCanvas.h>
#include <TH1.h>
#include "QualityControl/QcInfoLogger.h"
#include "FDD/DigitQcTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include "DataFormatsFDD/ChannelData.h"
#include "DataFormatsFDD/Digit.h"
#include <DataFormatsFDD/LookUpTable.h>

namespace o2::quality_control_modules::fdd
{

DigitQcTask::~DigitQcTask()
{
  delete mListHistGarbage;
}

void DigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize DigitQcTask"
                      << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};

  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::fdd::ChannelData::EEventDataBit::kIsTimeInfoLost, "IsTimeInfoLost" });

  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::fdd::Triggers::bitSCen, "SemiCentral" });
  mMapDigitTrgNames.insert({ 5, "Laser" });
  mMapDigitTrgNames.insert({ 6, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ 7, "DataIsValid" });

  mHist2CorrTCMchAndPMch = std::make_unique<TH2F>("DiffTCMchAndPMch", "TCM charge  vs (TCM charge - (PM totalCh/8));TCM charge;TCM - PM TotalCh/8;", 3300, 0, 6600,
                                                  301, -150.5, 150.5);
  mHist2CorrTCMchAndPMch->SetOption("colz");
  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Time;Channel;", 4100, -2050, 2050,
                                        sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Amp;Channel;", sNCHANNELS_PM, 0,
                                       sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmp2Ch->SetOption("colz");
  mHistOrbit2BC = std::make_unique<TH2F>("OrbitPerBC", "BC-Orbit map;Orbit;BC;", 256, 0, 256, 3564, 0, 3564);
  mHistOrbit2BC->SetOption("colz");
  mHistBC = std::make_unique<TH1F>("BC", "BC;BC;counts;", 3564, 0, 3564);

  mHistEventDensity2Ch = std::make_unique<TH2F>("EventDensityPerChannel",
                                                "Event density(in BC) per Channel;Channel;BC;", sNCHANNELS_PM, 0,
                                                sNCHANNELS_PM, 10000, 0, 1e5);
  mHistEventDensity2Ch->SetOption("colz");

  mHistChDataBits = std::make_unique<TH2F>("ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit",
                                           sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapChTrgNames.size(), 0,
                                           mMapChTrgNames.size());
  mHistChDataBits->SetOption("colz");

  for (const auto& entry : mMapChTrgNames) {
    mHistChDataBits->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }

  mHistTriggers = std::make_unique<TH1F>("Triggers", "Triggers from TCM", mMapDigitTrgNames.size(), 0,
                                         mMapDigitTrgNames.size());
  for (const auto& entry : mMapDigitTrgNames) {
    mHistTriggers->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }

  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);
  //---------ak--------
  char* p;
  for (const auto& lutEntry : o2::fdd::SingleLUT::Instance().getVecMetadataFEE()) {
    int chId = std::strtol(lutEntry.mChannelID.c_str(), &p, 10);
    if (*p) {
      // lutEntry.mChannelID is not a number
      continue;
    }
    auto moduleName = lutEntry.mModuleName;
    if (moduleName == "TCM") {
      if (mMapPmModuleChannels.find(moduleName) == mMapPmModuleChannels.end()) {
        mMapPmModuleChannels.insert({ moduleName, std::vector<int>{} });
      }
      continue;
    }
    if (mMapPmModuleChannels.find(moduleName) != mMapPmModuleChannels.end()) {
      mMapPmModuleChannels[moduleName].push_back(chId);
    } else {
      std::vector<int> vChId = {
        chId,
      };
      mMapPmModuleChannels.insert({ moduleName, vChId });
    }
  }

  for (const auto& entry : mMapPmModuleChannels) {
    auto pairModuleBcOrbit = mMapPmModuleBcOrbit.insert({ entry.first, new TH2F(Form("BcOrbitMap_%s", entry.first.c_str()), Form("BC-orbit map for %s;Orbit;BC", entry.first.c_str()), 256, 0, 256, 3564, 0, 3564) });
    if (pairModuleBcOrbit.second) {
      getObjectsManager()->startPublishing(pairModuleBcOrbit.first->second);
      pairModuleBcOrbit.first->second->SetOption("colz");
      mListHistGarbage->Add(pairModuleBcOrbit.first->second);
    }
  }
  //---------ak--------
  mHistNchA = std::make_unique<TH1F>("NumChannelsA", "Number of channels(TCM), side A;Nch", sNCHANNELS_PM, 0,
                                     sNCHANNELS_PM);
  mHistNchC = std::make_unique<TH1F>("NumChannelsC", "Number of channels(TCM), side C;Nch", sNCHANNELS_PM, 0,
                                     sNCHANNELS_PM);
  mHistSumAmpA = std::make_unique<TH1F>("SumAmpA", "Sum of amplitudes(TCM), side A;", 10000, 0, 1e4);
  mHistSumAmpC = std::make_unique<TH1F>("SumAmpC", "Sum of amplitudes(TCM), side C;", 10000, 0, 1e4);
  mHistAverageTimeA = std::make_unique<TH1F>("AverageTimeA", "Average time(TCM), side A", 4100, -2050, 2050);
  mHistAverageTimeC = std::make_unique<TH1F>("AverageTimeC", "Average time(TCM), side C", 4100, -2050, 2050);
  mHistChannelID = std::make_unique<TH1F>("StatChannelID", "ChannelID statistics;ChannelID", sNCHANNELS_PM, 0,
                                          sNCHANNELS_PM);
  mHistCycleDuration = std::make_unique<TH1D>("CycleDuration", "Cycle Duration;;time [ns]", 1, 0, 2);
  mHistCycleDurationNTF = std::make_unique<TH1D>("CycleDurationNTF", "Cycle Duration;;time [TimeFrames]", 1, 0, 2);
  mHistCycleDurationRange = std::make_unique<TH1D>("CycleDurationRange",
                                                   "Cycle Duration (total cycle range);;time [ns]", 1, 0, 2);

  std::vector<unsigned int> vecChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  } else {
    for (unsigned int iCh = 0; iCh < sNCHANNELS_PM; iCh++) {
      vecChannelIDs.push_back(iCh);
    }
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }

  for (const auto& chID : mSetAllowedChIDs) {
    auto pairHistAmp = mMapHistAmp1D.insert(
      { chID, new TH1F(Form("Amp_channel%i", chID), Form("Amplitude, channel %i", chID), 4200, -100, 4100) });
    auto pairHistAmpCoincidence = mMapHistAmp1DCoincidence.insert({ chID,
                                                                    new TH1F(Form("Amp_channelCoincidence%i", chID),
                                                                             Form("AmplitudeCoincidence, channel %i",
                                                                                  chID),
                                                                             4200, -100, 4100) });
    auto pairHistTime = mMapHistTime1D.insert(
      { chID, new TH1F(Form("Time_channel%i", chID), Form("Time, channel %i", chID), 4100, -2050, 2050) });
    auto pairHistBits = mMapHistPMbits.insert({ chID, new TH1F(Form("Bits_channel%i", chID), Form("Bits, channel %i", chID),
                                                               mMapChTrgNames.size(), 0, mMapChTrgNames.size()) });
    auto pairHistAmpVsTime = mMapHistAmpVsTime.insert({ chID, new TH2F(Form("Amp_vs_time_channel%i", chID), Form("Amplitude vs time, channel %i;Amp;Time", chID), 420, -100, 4100, 410, -2050, 2050) });
    for (const auto& entry : mMapChTrgNames) {
      pairHistBits.first->second->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    }
    if (pairHistAmp.second) {
      getObjectsManager()->startPublishing(pairHistAmp.first->second);
      mListHistGarbage->Add(pairHistAmp.first->second);
    }

    if (pairHistAmpCoincidence.second) {
      getObjectsManager()->startPublishing(pairHistAmpCoincidence.first->second);
      mListHistGarbage->Add(pairHistAmpCoincidence.first->second);
    }

    if (pairHistTime.second) {
      mListHistGarbage->Add(pairHistTime.first->second);
      getObjectsManager()->startPublishing(pairHistTime.first->second);
    }

    if (pairHistBits.second) {
      mListHistGarbage->Add(pairHistBits.first->second);
      getObjectsManager()->startPublishing(pairHistBits.first->second);
    }

    if (pairHistAmpVsTime.second) {
      mListHistGarbage->Add(pairHistAmpVsTime.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
    }
  }

  getObjectsManager()->startPublishing(mHist2CorrTCMchAndPMch.get());
  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->startPublishing(mHistOrbit2BC.get());
  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistEventDensity2Ch.get());
  getObjectsManager()->startPublishing(mHistChDataBits.get());
  getObjectsManager()->startPublishing(mHistTriggers.get());
  getObjectsManager()->startPublishing(mHistNchA.get());
  getObjectsManager()->startPublishing(mHistNchC.get());
  getObjectsManager()->startPublishing(mHistSumAmpA.get());
  getObjectsManager()->startPublishing(mHistSumAmpC.get());
  getObjectsManager()->startPublishing(mHistAverageTimeA.get());
  getObjectsManager()->startPublishing(mHistAverageTimeC.get());
  getObjectsManager()->startPublishing(mHistChannelID.get());
  getObjectsManager()->startPublishing(mHistCycleDuration.get());
  getObjectsManager()->startPublishing(mHistCycleDurationNTF.get());
  getObjectsManager()->startPublishing(mHistCycleDurationRange.get());
}

void DigitQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mHist2CorrTCMchAndPMch->Reset();
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistOrbit2BC->Reset();
  mHistBC->Reset();
  mHistEventDensity2Ch->Reset();
  mHistChDataBits->Reset();
  mHistTriggers->Reset();
  mHistNchA->Reset();
  mHistNchC->Reset();
  mHistSumAmpA->Reset();
  mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  mHistCycleDuration->Reset();
  mHistCycleDurationNTF->Reset();
  mHistCycleDurationRange->Reset();

  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }

  for (auto& entry : mMapHistAmp1DCoincidence) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTime1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistPMbits) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

void DigitQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
  mTimeMinNS = -1;
  mTimeMaxNS = 0.;
  mTimeCurNS = 0.;
  mTfCounter = 0;
  mTimeSum = 0.;
}

void DigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  double curTfTimeMin = -1;
  double curTfTimeMax = 0;
  mTfCounter++;
  auto channels = ctx.inputs().get<gsl::span<o2::fdd::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::fdd::Digit>>("digits");
  // bool isFirst = true;
  // uint32_t firstOrbit;

  int mPMChargeTotalAside, mPMChargeTotalCside;
  for (auto& digit : digits) {
    mPMChargeTotalAside = 0;
    mPMChargeTotalCside = 0;
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    mTimeCurNS = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
    if (mTimeMinNS < 0) {
      mTimeMinNS = mTimeCurNS;
    }
    if (mTimeCurNS < mTimeMinNS) {
      mTimeMinNS = mTimeCurNS;
    }
    if (mTimeCurNS > mTimeMaxNS) {
      mTimeMaxNS = mTimeCurNS;
    }

    if (curTfTimeMin < 0) {
      curTfTimeMin = mTimeCurNS;
    }
    if (mTimeCurNS < curTfTimeMin) {
      curTfTimeMin = mTimeCurNS;
    }
    if (mTimeCurNS > curTfTimeMax) {
      curTfTimeMax = mTimeCurNS;
    }

    /* if (isFirst == true) {
       //firstOrbit = digit.getIntRecord().orbit;
       isFirst = false;
     }
     */
    if (digit.mTriggers.getAmplA() == fit::Triggers::DEFAULT_AMP && digit.mTriggers.getAmplC() == fit::Triggers::DEFAULT_AMP &&
        digit.mTriggers.getTimeA() == fit::Triggers::DEFAULT_TIME && digit.mTriggers.getTimeC() == fit::Triggers::DEFAULT_TIME) {
      isTCM = false;
    }
    mHistOrbit2BC->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, digit.getIntRecord().bc);
    mHistBC->Fill(digit.getIntRecord().bc);

    if (isTCM && digit.mTriggers.getDataIsValid()) {
      mHistNchA->Fill(digit.mTriggers.getNChanA());
      mHistNchC->Fill(digit.mTriggers.getNChanC());
      mHistSumAmpA->Fill(digit.mTriggers.getAmplA());
      mHistSumAmpC->Fill(digit.mTriggers.getAmplC());
      mHistAverageTimeA->Fill(digit.mTriggers.getTimeA());
      mHistAverageTimeC->Fill(digit.mTriggers.getTimeC());
      for (const auto& entry : mMapDigitTrgNames) {
        if (helper::digit::getTriggerBits(digit) & (1 << entry.first)) {
          mHistTriggers->Fill(static_cast<Double_t>(entry.first));
        }
      }
    }
    //-----ak------
    for (auto& entry : mMapPmModuleChannels) {
      for (const auto& chData : vecChData) {
        if (std::find(entry.second.begin(), entry.second.end(), chData.mPMNumber) != entry.second.end()) {
          mMapPmModuleBcOrbit[entry.first]->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
          break;
        }
      }
      if (entry.first == "TCM" && isTCM && digit.mTriggers.getDataIsValid()) {
        mMapPmModuleBcOrbit[entry.first]->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
      }
    }
    //-----ak------
    // Fill the amplitude, if there is a coincidence of the signals in in the front or back layers
    bool hasData[16] = { 0 };
    for (const auto& chData : vecChData) {
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(chData.mPMNumber)) != mSetAllowedChIDs.end()) {
        if (static_cast<int>(chData.mPMNumber) < 16) {
          hasData[static_cast<int>(chData.mPMNumber)] = 1;
        }
      }
    } // ak
    for (const auto& chData : vecChData) {
      mHistTime2Ch->Fill(static_cast<Double_t>(ch_data::getTime(chData)),
                         static_cast<Double_t>(ch_data::getChId(chData)));
      mHistAmp2Ch->Fill(static_cast<Double_t>(ch_data::getChId(chData)),
                        static_cast<Double_t>(ch_data::getCharge(chData)));
      mHistEventDensity2Ch->Fill(static_cast<Double_t>(ch_data::getCharge(chData)),
                                 static_cast<Double_t>(digit.getIntRecord().differenceInBC(
                                   mStateLastIR2Ch[ch_data::getChId(chData)])));
      mStateLastIR2Ch[ch_data::getChId(chData)] = digit.getIntRecord();
      mHistChannelID->Fill(ch_data::getChId(chData));
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(ch_data::getChId(chData))) != mSetAllowedChIDs.end()) {

        if (static_cast<int>(chData.mPMNumber) < 8) {
          if (chData.mFEEBits & (1 << o2::fdd::ChannelData::kIsCFDinADCgate))
            mPMChargeTotalCside += chData.mChargeADC;
        } else {
          if (chData.mFEEBits & (1 << o2::fdd::ChannelData::kIsCFDinADCgate))
            mPMChargeTotalAside += chData.mChargeADC;
        }

        mMapHistAmp1D[ch_data::getChId(chData)]->Fill(ch_data::getCharge(chData));

        if (static_cast<int>(chData.mPMNumber) == 0 && hasData[4]) {
          mMapHistAmp1DCoincidence[0]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 1 && hasData[5]) {
          mMapHistAmp1DCoincidence[1]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 2 && hasData[6]) {
          mMapHistAmp1DCoincidence[2]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 3 && hasData[7]) {
          mMapHistAmp1DCoincidence[3]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 4 && hasData[0]) {
          mMapHistAmp1DCoincidence[4]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 5 && hasData[1]) {
          mMapHistAmp1DCoincidence[5]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 6 && hasData[2]) {
          mMapHistAmp1DCoincidence[6]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 7 && hasData[3]) {
          mMapHistAmp1DCoincidence[7]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 8 && hasData[12]) {
          mMapHistAmp1DCoincidence[8]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 9 && hasData[13]) {
          mMapHistAmp1DCoincidence[9]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 10 && hasData[14]) {
          mMapHistAmp1DCoincidence[10]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 11 && hasData[15]) {
          mMapHistAmp1DCoincidence[11]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 12 && hasData[8]) {
          mMapHistAmp1DCoincidence[12]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 13 && hasData[9]) {
          mMapHistAmp1DCoincidence[13]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 14 && hasData[10]) {
          mMapHistAmp1DCoincidence[14]->Fill(chData.mChargeADC);
        }
        if (static_cast<int>(chData.mPMNumber) == 15 && hasData[11]) {
          mMapHistAmp1DCoincidence[15]->Fill(chData.mChargeADC);
        }

        mMapHistTime1D[ch_data::getChId(chData)]->Fill(ch_data::getTime(chData));
        mMapHistAmpVsTime[ch_data::getChId(chData)]->Fill(ch_data::getCharge(chData), ch_data::getTime(chData));
        for (const auto& entry : mMapChTrgNames) {
          if ((ch_data::getPMbits(chData) & (1 << entry.first))) {
            mMapHistPMbits[ch_data::getChId(chData)]->Fill(entry.first);
          }
        }
      }
      for (const auto& entry : mMapChTrgNames) {
        if ((ch_data::getPMbits(chData) & (1 << entry.first))) {
          mHistChDataBits->Fill(ch_data::getChId(chData), entry.first);
        }
      }
    }
    /// PM charge is scaled by 8 to compare with TCM charge
    mPMChargeTotalAside = std::lround(static_cast<float>(mPMChargeTotalAside / 8));
    mPMChargeTotalCside = std::lround(static_cast<float>(mPMChargeTotalCside / 8));

    if (isTCM && digit.mTriggers.getDataIsValid()) {
      mHist2CorrTCMchAndPMch->Fill(digit.mTriggers.getAmplA() + digit.mTriggers.getAmplC(), (digit.mTriggers.getAmplA() + digit.mTriggers.getAmplC()) - (mPMChargeTotalAside + mPMChargeTotalCside));
      // std::cout<<"TCM ch "<<digit.mTriggers.getAmplA()+digit.mTriggers.getAmplC()<<" PM ch "<<mPMChargeTotalAside+mPMChargeTotalCside<<std::endl;
    }
  }
  mTimeSum += curTfTimeMax - curTfTimeMin;
}

void DigitQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  mHistCycleDurationRange->SetBinContent(1., mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationRange->SetEntries(mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationNTF->SetBinContent(1., mTfCounter);
  mHistCycleDurationNTF->SetEntries(mTfCounter);
  mHistCycleDuration->SetBinContent(1., mTimeSum);
  mHistCycleDuration->SetEntries(mTimeSum);
  ILOG(Debug, Support) << "Cycle duration: NTF=" << mTfCounter << ", range = "
                       << (mTimeMaxNS - mTimeMinNS) / 1e6 / mTfCounter
                       << " ms/TF, sum = " << mTimeSum / 1e6 / mTfCounter << " ms/TF" << ENDM;
}

void DigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void DigitQcTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHist2CorrTCMchAndPMch->Reset();
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistOrbit2BC->Reset();
  mHistBC->Reset();
  mHistEventDensity2Ch->Reset();
  mHistChDataBits->Reset();
  mHistTriggers->Reset();
  mHistNchA->Reset();
  mHistNchC->Reset();
  mHistSumAmpA->Reset();
  mHistSumAmpC->Reset();
  mHistAverageTimeA->Reset();
  mHistAverageTimeC->Reset();
  mHistChannelID->Reset();
  for (auto& entry : mMapHistAmp1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmp1DCoincidence) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTime1D) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistPMbits) {
    entry.second->Reset();
  }

  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

} // namespace o2::quality_control_modules::fdd