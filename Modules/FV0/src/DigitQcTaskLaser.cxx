// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright
// holders. All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   DigitQcTaskLaser.cxx
/// \author Based on the existing DigitQcTask.cxx, developed by Sandor Lokos
/// (sandor.lokos@cern.ch)
///

#include <TCanvas.h>
#include <TH1.h>
#include <TROOT.h>

#include "QualityControl/QcInfoLogger.h"
#include "FV0/DigitQcTaskLaser.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

#include <DataFormatsFV0/LookUpTable.h>

namespace o2::quality_control_modules::fv0
{

DigitQcTaskLaser::~DigitQcTaskLaser()
{
  delete mListHistGarbage;
}

void DigitQcTaskLaser::rebinFromConfig()
{
  /* Examples:
     "binning_SumAmpC": "100, 0, 100"
     "binning_BcOrbitMap_TrgOrA": "25, 0, 256, 10, 0, 3564"
   hashtag = all channel IDs (mSetAllowedChIDs), e.g.
     "binning_Amp_channel#": "5,-10,90"
   is equivalent to:
     "binning_Amp_channel0": "5,-10,90"
     "binning_Amp_channel1": "5,-10,90"
     "binning_Amp_channel2": "5,-10,90" ...
  */
  auto rebinHisto = [](std::string hName, std::string binning) {
    std::vector<std::string> tokenizedBinning;
    boost::split(tokenizedBinning, binning, boost::is_any_of(","));
    if (tokenizedBinning.size() == 3) { // TH1
      ILOG(Debug, Support) << "config: rebinning TH1 " << hName << " -> " << binning << ENDM;
      auto htmp = (TH1F*)gROOT->FindObject(hName.data());
      htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()));
    } else if (tokenizedBinning.size() == 6) { // TH2
      auto htmp = (TH2F*)gROOT->FindObject(hName.data());
      ILOG(Debug, Support) << "config: rebinning TH2 " << hName << " -> " << binning << ENDM;
      htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()),
                    std::atof(tokenizedBinning[3].c_str()), std::atof(tokenizedBinning[4].c_str()), std::atof(tokenizedBinning[5].c_str()));
    } else {
      ILOG(Warning, Support) << "config: invalid binning parameter: " << hName << " -> " << binning << ENDM;
    }
  };

  const std::string rebinKeyword = "binning";
  const char* channelIdPlaceholder = "#";
  for (auto& param : mCustomParameters) {
    if (param.first.rfind(rebinKeyword, 0) != 0)
      continue;
    std::string hName = param.first.substr(rebinKeyword.length() + 1);
    std::string binning = param.second.c_str();
    if (hName.find(channelIdPlaceholder) != std::string::npos) {
      for (const auto& chID : mSetAllowedChIDs) {
        std::string hNameCur = hName.substr(0, hName.find(channelIdPlaceholder)) + std::to_string(chID) + hName.substr(hName.find(channelIdPlaceholder) + 1);
        rebinHisto(hNameCur, binning);
      }
    } else if (!gROOT->FindObject(hName.data())) {
      ILOG(Warning, Support) << "config: histogram named \"" << hName << "\" not found" << ENDM;
      continue;
    } else {
      rebinHisto(hName, binning);
    }
  }
}

void DigitQcTaskLaser::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize DigitQcTaskLaser" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};
  // Not ready yet
  /*
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kNumberADC, "NumberADC" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsDoubleEvent, "IsDoubleEvent" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsCFDinADCgate, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoLate, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsAmpHigh, "IsAmpHigh" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsEventInTVDC, "IsEventInTVDC" });
  mMapChTrgNames.insert({ o2::fv0::ChannelData::kIsTimeInfoLost, "IsTimeInfoLost" });

  mMapDigitTrgNames.insert({ o2::fv0::Triggers::bitA, "OrA" });
  mMapDigitTrgNames.insert({ o2::fv0::Triggers::bitC, "OrC" });
  mMapDigitTrgNames.insert({ o2::fv0::Triggers::bitVertex, "Vertex" });
  mMapDigitTrgNames.insert({ o2::fv0::Triggers::bitCen, "Central" });
  mMapDigitTrgNames.insert({ o2::fv0::Triggers::bitSCen, "SemiCentral" });
  */
  mMapChTrgNames.insert({ 0, "NumberADC" });
  mMapChTrgNames.insert({ 1, "IsDoubleEvent" });
  mMapChTrgNames.insert({ 2, "IsTimeInfoNOTvalid" });
  mMapChTrgNames.insert({ 3, "IsCFDinADCgate" });
  mMapChTrgNames.insert({ 4, "IsTimeInfoLate" });
  mMapChTrgNames.insert({ 5, "IsAmpHigh" });
  mMapChTrgNames.insert({ 6, "IsEventInTVDC" });
  mMapChTrgNames.insert({ 7, "IsTimeInfoLost" });

  mMapDigitTrgNames.insert({ ETrgMenu::kMinBias, "MinBias" });
  mMapDigitTrgNames.insert({ ETrgMenu::kOuterRing, "OuterRing" });
  mMapDigitTrgNames.insert({ ETrgMenu::kNChannels, "NChannels" });
  mMapDigitTrgNames.insert({ ETrgMenu::kCharge, "Charge" });
  mMapDigitTrgNames.insert({ ETrgMenu::kInnerRing, "InnerRing" });
  mMapDigitTrgNames.insert({ 5, "Laser" });
  mMapDigitTrgNames.insert({ 6, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ 7, "DataIsValid" });

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmp2Ch->SetOption("colz");
  mHistOrbit2BC = std::make_unique<TH2F>("OrbitPerBC", "BC-Orbit map;Orbit;BC;", 256, 0, 256, 3564, 0, 3564);
  mHistOrbit2BC->SetOption("colz");
  mHistBC = std::make_unique<TH1F>("BC", "BC;BC;counts;", 3564, 0, 3564);

  mHistEventDensity2Ch = std::make_unique<TH2F>("EventDensityPerChannel", "Event density(in BC) per Channel;Channel;BC;", sNCHANNELS_PM, 0, sNCHANNELS_PM, 10000, 0, 1e5);
  mHistEventDensity2Ch->SetOption("colz");

  mHistChDataBits = std::make_unique<TH2F>("ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  mHistChDataBits->SetOption("colz");

  for (const auto& entry : mMapChTrgNames) {
    mHistChDataBits->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  mHistTriggersCorrelation = std::make_unique<TH2F>("TriggersCorrelation", "Correlation of triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size(), mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  mHistTriggersCorrelation->SetOption("colz");
  mHistTriggers = std::make_unique<TH1F>("Triggers", "Triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());

  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);
  for (const auto& entry : mMapDigitTrgNames) {
    mHistTriggers->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistTriggersCorrelation->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    mHistTriggersCorrelation->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());

    auto pairTrgBcOrbit = mMapTrgBcOrbit.insert({ entry.first, new TH2F(Form("BcOrbitMap_Trg%s", entry.second.c_str()), Form("BC-orbit map: %s fired;Orbit;BC", entry.second.c_str()), 256, 0, 256, 3564, 0, 3564) });
    if (pairTrgBcOrbit.second) {
      getObjectsManager()->startPublishing(pairTrgBcOrbit.first->second);
      pairTrgBcOrbit.first->second->SetOption("colz");
      mListHistGarbage->Add(pairTrgBcOrbit.first->second);
    }
  }

  char* p;
  for (const auto& lutEntry : o2::fv0::SingleLUT::Instance().getVecMetadataFEE()) {
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

  mHistNchA = std::make_unique<TH1F>("NumChannelsA", "Number of channels(TCM), side A;Nch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNchC = std::make_unique<TH1F>("NumChannelsC", "Number of channels(TCM), side C;Nch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistSumAmpA = std::make_unique<TH1F>("SumAmpA", "Sum of amplitudes(TCM), side A;", 1000, 0, 1e4);
  mHistSumAmpC = std::make_unique<TH1F>("SumAmpC", "Sum of amplitudes(TCM), side C;", 1000, 0, 1e4);
  mHistAverageTimeA = std::make_unique<TH1F>("AverageTimeA", "Average time(TCM), side A", 4100, -2050, 2050);
  mHistAverageTimeC = std::make_unique<TH1F>("AverageTimeC", "Average time(TCM), side C", 4100, -2050, 2050);
  // mHistTimeSum2Diff = std::make_unique<TH2F>("timeSumVsDiff", "time A/C side: sum VS diff;(TOC-TOA)/2 [ns];(TOA+TOC)/2 [ns]", 400, -52.08, 52.08, 400, -52.08, 52.08); // range of 52.08 ns = 4000*13.02ps = 4000 channels
  // mHistTimeSum2Diff->SetOption("colz");
  mHistChannelID = std::make_unique<TH1F>("StatChannelID", "ChannelID statistics;ChannelID", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNumADC = std::make_unique<TH1F>("HistNumADC", "HistNumADC", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNumCFD = std::make_unique<TH1F>("HistNumCFD", "HistNumCFD", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCFDEff = std::make_unique<TH1F>("CFD_efficiency", "CFD efficiency;ChannelID;efficiency", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistCycleDuration = std::make_unique<TH1D>("CycleDuration", "Cycle Duration;;time [ns]", 1, 0, 2);
  mHistCycleDurationNTF = std::make_unique<TH1D>("CycleDurationNTF", "Cycle Duration;;time [TimeFrames]", 1, 0, 2);
  mHistCycleDurationRange = std::make_unique<TH1D>("CycleDurationRange", "Cycle Duration (total cycle range);;time [ns]", 1, 0, 2);

  std::vector<unsigned int> vecChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  } else {
    for (unsigned int iCh = 0; iCh < sNCHANNELS_PM; iCh++)
      vecChannelIDs.push_back(iCh);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }

  for (const auto& chID : mSetAllowedChIDs) {
    auto pairHistAmp = mMapHistAmp1D.insert({ chID, new TH1F(Form("Amp_channel%i", chID), Form("Amplitude, channel %i", chID), 4200, -100, 4100) });
    auto pairHistTime = mMapHistTime1D.insert({ chID, new TH1F(Form("Time_channel%i", chID), Form("Time, channel %i", chID), 4100, -2050, 2050) });
    auto pairHistBits = mMapHistPMbits.insert({ chID, new TH1F(Form("Bits_channel%i", chID), Form("Bits, channel %i", chID), mMapChTrgNames.size(), 0, mMapChTrgNames.size()) });
    auto pairHistAmpVsTime = mMapHistAmpVsTime.insert({ chID, new TH2F(Form("Amp_vs_time_channel%i", chID), Form("Amplitude vs time, channel %i;Amp;Time", chID), 420, -100, 4100, 410, -2050, 2050) });
    for (const auto& entry : mMapChTrgNames) {
      pairHistBits.first->second->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
    }
    if (pairHistAmp.second) {
      getObjectsManager()->startPublishing(pairHistAmp.first->second);
      mListHistGarbage->Add(pairHistAmp.first->second);
    }
    if (pairHistTime.second) {
      mListHistGarbage->Add(pairHistTime.first->second);
      getObjectsManager()->startPublishing(pairHistTime.first->second);
    }
    if (pairHistBits.second) {
      mListHistGarbage->Add(pairHistBits.first->second);
      // getObjectsManager()->startPublishing(pairHistBits.first->second); // TODO: why commented out
    }
    if (pairHistAmpVsTime.second) {
      mListHistGarbage->Add(pairHistAmpVsTime.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
    }
  }

  rebinFromConfig(); // after all histos are created

  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->startPublishing(mHistOrbit2BC.get());
  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistEventDensity2Ch.get());
  // getObjectsManager()->startPublishing(mHistChDataBits.get());
  getObjectsManager()->startPublishing(mHistTriggers.get());
  getObjectsManager()->startPublishing(mHistNchA.get());
  // getObjectsManager()->startPublishing(mHistNchC.get());
  getObjectsManager()->startPublishing(mHistSumAmpA.get());
  // getObjectsManager()->startPublishing(mHistSumAmpC.get());
  // getObjectsManager()->startPublishing(mHistAverageTimeA.get());
  // getObjectsManager()->startPublishing(mHistAverageTimeC.get());
  getObjectsManager()->startPublishing(mHistChannelID.get());
  getObjectsManager()->startPublishing(mHistCFDEff.get());
  getObjectsManager()->startPublishing(mHistTriggersCorrelation.get());
  // getObjectsManager()->startPublishing(mHistTimeSum2Diff.get());
  getObjectsManager()->startPublishing(mHistCycleDuration.get());
  getObjectsManager()->startPublishing(mHistCycleDurationNTF.get());
  getObjectsManager()->startPublishing(mHistCycleDurationRange.get());
}

void DigitQcTaskLaser::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
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
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  mHistTriggersCorrelation->Reset();
  // mHistTimeSum2Diff->Reset();
  mHistCycleDuration->Reset();
  mHistCycleDurationNTF->Reset();
  mHistCycleDurationRange->Reset();
  for (auto& entry : mMapHistAmp1D) {
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
  for (auto& entry : mMapTrgBcOrbit) {
    entry.second->Reset();
  }
  for (auto& entry : mMapPmModuleBcOrbit) {
    entry.second->Reset();
  }
}

void DigitQcTaskLaser::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
  mTimeMinNS = -1;
  mTimeMaxNS = 0.;
  mTimeCurNS = 0.;
  mTfCounter = 0;
  mTimeSum = 0.;
}

void DigitQcTaskLaser::monitorData(o2::framework::ProcessingContext& ctx)
{
  double curTfTimeMin = -1;
  double curTfTimeMax = 0;
  mTfCounter++;
  auto channels = ctx.inputs().get<gsl::span<ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<Digit>>("digits");
  // bool isFirst = true;
  // uint32_t firstOrbit;

  for (auto& digit : digits) {
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    mTimeCurNS = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
    if (mTimeMinNS < 0)
      mTimeMinNS = mTimeCurNS;
    /*
    if (isFirst == true) {
      //firstOrbit = digit.getOrbit();
      isFirst = false;
    }
    */
    if (mTimeCurNS < mTimeMinNS)
      mTimeMinNS = mTimeCurNS;
    if (mTimeCurNS > mTimeMaxNS)
      mTimeMaxNS = mTimeCurNS;

    if (curTfTimeMin < 0)
      curTfTimeMin = mTimeCurNS;
    if (mTimeCurNS < curTfTimeMin)
      curTfTimeMin = mTimeCurNS;
    if (mTimeCurNS > curTfTimeMax)
      curTfTimeMax = mTimeCurNS;
    /*
    if (digit.mTriggers.amplA == -5000 && digit.mTriggers.amplC == -5000 && digit.mTriggers.timeA == -5000 && digit.mTriggers.timeC == -5000)
      isTCM = false;
    */
    mHistOrbit2BC->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
    mHistBC->Fill(digit.getBC());

    if (isTCM && !digit.mTriggers.getLaserBit()) {
      mHistNchA->Fill(digit.mTriggers.nChanA);
      // mHistNchC->Fill(digit.mTriggers.nChanC);
      mHistSumAmpA->Fill(digit.mTriggers.amplA);
      // mHistSumAmpC->Fill(digit.mTriggers.amplC);
      // mHistAverageTimeA->Fill(digit.mTriggers.timeA);
      // mHistAverageTimeC->Fill(digit.mTriggers.timeC);
      // mHistTimeSum2Diff->Fill((digit.mTriggers.timeC - digit.mTriggers.timeA) * mCFDChannel2NS / 2, (digit.mTriggers.timeC + digit.mTriggers.timeA) * mCFDChannel2NS / 2);
      for (const auto& entry : mMapDigitTrgNames) {
        if (digit.mTriggers.triggersignals & (1 << entry.first))
          mHistTriggers->Fill(static_cast<Double_t>(entry.first));
        for (const auto& entry2 : mMapDigitTrgNames) {
          if ((digit.mTriggers.triggersignals & (1 << entry.first)) && (digit.mTriggers.triggersignals & (1 << entry2.first)))
            mHistTriggersCorrelation->Fill(static_cast<Double_t>(entry.first), static_cast<Double_t>(entry2.first));
        }
      }
      for (auto& entry : mMapTrgBcOrbit) {
        if (digit.mTriggers.triggersignals & (1 << entry.first)) {
          entry.second->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
        }
      }
    }

    for (auto& entry : mMapPmModuleChannels) {
      for (const auto& chData : vecChData) {
        if (std::find(entry.second.begin(), entry.second.end(), chData.ChId) != entry.second.end()) {
          mMapPmModuleBcOrbit[entry.first]->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
          break;
        }
      }
      if (entry.first == "TCM" && isTCM && (digit.getTriggers().triggersignals & (1 << sDataIsValidBitPos))) {
        mMapPmModuleBcOrbit[entry.first]->Fill(digit.getOrbit() % sOrbitsPerTF, digit.getBC());
      }
    }

    for (const auto& chData : vecChData) {
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.CFDTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      mHistEventDensity2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(digit.mIntRecord.differenceInBC(mStateLastIR2Ch[chData.ChId])));
      mStateLastIR2Ch[chData.ChId] = digit.mIntRecord;
      mHistChannelID->Fill(chData.ChId);
      if (chData.QTCAmpl > 0)
        mHistNumADC->Fill(chData.ChId);
      mHistNumCFD->Fill(chData.ChId);
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(chData.ChId)) != mSetAllowedChIDs.end()) {
        mMapHistAmp1D[chData.ChId]->Fill(chData.QTCAmpl);
        mMapHistTime1D[chData.ChId]->Fill(chData.CFDTime);
        mMapHistAmpVsTime[chData.ChId]->Fill(chData.QTCAmpl, chData.CFDTime);
        /*
        for (const auto& entry : mMapChTrgNames) {
          if ((chData.QTCAmpl & (1 << entry.first))) {
            mMapHistPMbits[chData.ChId]->Fill(entry.first);
          }
        }
        */
      }
      /*
      for (const auto& entry : mMapChTrgNames) {
        if ((chData.ChainQTC & (1 << entry.first))) {
          mHistChDataBits->Fill(chData.ChId, entry.first);
        }
      }
      */
    }
    mHistCFDEff->Reset();
    mHistCFDEff->Divide(mHistNumADC.get(), mHistNumCFD.get());
  }
  mTimeSum += curTfTimeMax - curTfTimeMin;
}

void DigitQcTaskLaser::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
  mHistCycleDurationRange->SetBinContent(1., mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationRange->SetEntries(mTimeMaxNS - mTimeMinNS);
  mHistCycleDurationNTF->SetBinContent(1., mTfCounter);
  mHistCycleDurationNTF->SetEntries(mTfCounter);
  mHistCycleDuration->SetBinContent(1., mTimeSum);
  mHistCycleDuration->SetEntries(mTimeSum);
  ILOG(Debug, Support) << "Cycle duration: NTF=" << mTfCounter << ", range = " << (mTimeMaxNS - mTimeMinNS) / 1e6 / mTfCounter << " ms/TF, sum = " << mTimeSum / 1e6 / mTfCounter << " ms/TF" << ENDM;
}

void DigitQcTaskLaser::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void DigitQcTaskLaser::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
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
  mHistCFDEff->Reset();
  mHistNumADC->Reset();
  mHistNumCFD->Reset();
  mHistTriggersCorrelation->Reset();
  // mHistTimeSum2Diff->Reset();
  mHistCycleDuration->Reset();
  mHistCycleDurationNTF->Reset();
  mHistCycleDurationRange->Reset();
  for (auto& entry : mMapHistAmp1D) {
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
  for (auto& entry : mMapTrgBcOrbit) {
    entry.second->Reset();
  }
  for (auto& entry : mMapPmModuleBcOrbit) {
    entry.second->Reset();
  }
}

} // namespace o2::quality_control_modules::fv0
