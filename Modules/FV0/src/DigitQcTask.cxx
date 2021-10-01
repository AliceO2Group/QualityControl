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
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "FV0/DigitQcTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

namespace o2::quality_control_modules::fv0
{

DigitQcTask::~DigitQcTask()
{
  delete mListHistGarbage;
}

void DigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize DigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};
  //Not ready yet
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

  mMapDigitTrgNames.insert({ 0, "OrA" });
  mMapDigitTrgNames.insert({ 1, "OrC" });
  mMapDigitTrgNames.insert({ 2, "Vertex" });
  mMapDigitTrgNames.insert({ 3, "Central" });
  mMapDigitTrgNames.insert({ 4, "SemiCentral" });
  mMapDigitTrgNames.insert({ 5, "Laser" });
  mMapDigitTrgNames.insert({ 6, "OutputsAreBlocked" });
  mMapDigitTrgNames.insert({ 7, "DataIsValid" });

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Time;Channel;", 4100, -2050, 2050, sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Amp;Channel;", 4200, -100, 4100, sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistAmp2Ch->SetOption("colz");
  mHistOrbit2BC = std::make_unique<TH2F>("OrbitPerBC", "BC-Orbit map;Orbit;BC;", 256, 0, 256, 3564, 0, 3564);
  mHistOrbit2BC->SetOption("colz");

  mHistEventDensity2Ch = std::make_unique<TH2F>("EventDensityPerChannel", "Event density(in BC) per Channel;Channel;BC;", sNCHANNELS_PM, 0, sNCHANNELS_PM, 10000, 0, 1e5);
  mHistEventDensity2Ch->SetOption("colz");

  mHistChDataBits = std::make_unique<TH2F>("ChannelDataBits", "ChannelData bits per ChannelID;Channel;Bit", sNCHANNELS_PM, 0, sNCHANNELS_PM, mMapChTrgNames.size(), 0, mMapChTrgNames.size());
  mHistChDataBits->SetOption("colz");

  for (const auto& entry : mMapChTrgNames) {
    mHistChDataBits->GetYaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }

  mHistTriggers = std::make_unique<TH1F>("Triggers", "Triggers from TCM", mMapDigitTrgNames.size(), 0, mMapDigitTrgNames.size());
  for (const auto& entry : mMapDigitTrgNames) {
    mHistTriggers->GetXaxis()->SetBinLabel(entry.first + 1, entry.second.c_str());
  }
  mHistNchA = std::make_unique<TH1F>("NumChannelsA", "Number of channels(TCM), side A;Nch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistNchC = std::make_unique<TH1F>("NumChannelsC", "Number of channels(TCM), side C;Nch", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mHistSumAmpA = std::make_unique<TH1F>("SumAmpA", "Sum of amplitudes(TCM), side A;", 1000, 0, 1e4);
  mHistSumAmpC = std::make_unique<TH1F>("SumAmpC", "Sum of amplitudes(TCM), side C;", 1000, 0, 1e4);
  mHistAverageTimeA = std::make_unique<TH1F>("AverageTimeA", "Average time(TCM), side A", 4100, -2050, 2050);
  mHistAverageTimeC = std::make_unique<TH1F>("AverageTimeC", "Average time(TCM), side C", 4100, -2050, 2050);
  mHistChannelID = std::make_unique<TH1F>("StatChannelID", "ChannelID statistics;ChannelID", sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);
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
      //getObjectsManager()->startPublishing(pairHistBits.first->second);
    }

    if (pairHistAmpVsTime.second) {
      mListHistGarbage->Add(pairHistAmpVsTime.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
    }
  }
  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->startPublishing(mHistOrbit2BC.get());
  getObjectsManager()->startPublishing(mHistEventDensity2Ch.get());
  //getObjectsManager()->startPublishing(mHistChDataBits.get());
  getObjectsManager()->startPublishing(mHistTriggers.get());
  getObjectsManager()->startPublishing(mHistNchA.get());
  //getObjectsManager()->startPublishing(mHistNchC.get());
  getObjectsManager()->startPublishing(mHistSumAmpA.get());
  //getObjectsManager()->startPublishing(mHistSumAmpC.get());
  //getObjectsManager()->startPublishing(mHistAverageTimeA.get());
  //getObjectsManager()->startPublishing(mHistAverageTimeC.get());
  getObjectsManager()->startPublishing(mHistChannelID.get());
}

void DigitQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistOrbit2BC->Reset();
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
}

void DigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto channels = ctx.inputs().get<gsl::span<ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<Digit>>("digits");
  //bool isFirst = true;
  //uint32_t firstOrbit;
  for (auto& digit : digits) {
    const auto& vecChData = digit.getBunchChannelData(channels);
    bool isTCM = true;
    /*
    if (isFirst == true) {
      //firstOrbit = digit.getIntRecord().orbit;
      isFirst = false;
    }
    */
    /*
    if (digit.getTriggers().amplA == -5000 && digit.getTriggers().amplC == -5000 && digit.getTriggers().timeA == -5000 && digit.getTriggers().timeC == -5000)
      isTCM = false;
    */
    mHistOrbit2BC->Fill(digit.getIntRecord().orbit % sOrbitsPerTF, digit.getIntRecord().bc);
    if (isTCM && !(digit.getTriggers().triggerSignals & (1 << sLaserBitPos))) {
      mHistNchA->Fill(digit.mTriggers.nChanA);
      //mHistNchC->Fill(digit.mTriggers.nChanC);
      mHistSumAmpA->Fill(digit.mTriggers.amplA);
      //mHistSumAmpC->Fill(digit.mTriggers.amplC);
      //mHistAverageTimeA->Fill(digit.mTriggers.timeA);
      //mHistAverageTimeC->Fill(digit.mTriggers.timeC);

      for (const auto& entry : mMapDigitTrgNames) {
        if (digit.getTriggers().triggerSignals & (1 << entry.first))
          mHistTriggers->Fill(static_cast<Double_t>(entry.first));
      }
    }

    for (const auto& chData : vecChData) {
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.time), static_cast<Double_t>(ch_data::getChId(chData)));
      mHistAmp2Ch->Fill(static_cast<Double_t>(ch_data::getCharge(chData)), static_cast<Double_t>(ch_data::getChId(chData)));
      mHistEventDensity2Ch->Fill(static_cast<Double_t>(ch_data::getChId(chData)), static_cast<Double_t>(digit.getIntRecord().differenceInBC(mStateLastIR2Ch[ch_data::getChId(chData)])));
      mStateLastIR2Ch[ch_data::getChId(chData)] = digit.getIntRecord();
      mHistChannelID->Fill(ch_data::getChId(chData));
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(ch_data::getChId(chData))) != mSetAllowedChIDs.end()) {
        mMapHistAmp1D[ch_data::getChId(chData)]->Fill(ch_data::getCharge(chData));
        mMapHistTime1D[ch_data::getChId(chData)]->Fill(chData.time);
        mMapHistAmpVsTime[ch_data::getChId(chData)]->Fill(ch_data::getCharge(chData), chData.time);
        /*
        for (const auto& entry : mMapChTrgNames) {
          if ((chData.ChainQTC & (1 << entry.first))) {
            mMapHistPMbits[ch_data::getChId(chData)]->Fill(entry.first);
          }
        }
        */
      }
      /*
      for (const auto& entry : mMapChTrgNames) {
        if ((chData.ChainQTC & (1 << entry.first))) {
          mHistChDataBits->Fill(ch_data::getChId(chData), entry.first);
        }
      }
      */
    }
  }
}

void DigitQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void DigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void DigitQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistOrbit2BC->Reset();
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

} // namespace o2::quality_control_modules::fv0
