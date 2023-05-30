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
/// \author Artur Furs afurs@cern.ch
/// modified by Sebastin Bysiak sbysiak@cern.ch

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TROOT.h>

#include "QualityControl/QcInfoLogger.h"
#include <FT0/RecPointsQcTask.h>
#include <DataFormatsFT0/RecPoints.h>
#include <DataFormatsFT0/ChannelData.h>
#include <Framework/InputRecord.h>
#include <vector>

namespace o2::quality_control_modules::ft0
{

RecPointsQcTask::~RecPointsQcTask()
{
  delete mListHistGarbage;
}

void RecPointsQcTask::rebinFromConfig()
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
      ILOG(Debug) << "config: rebinning TH1 " << hName << " -> " << binning << ENDM;
      auto htmp = (TH1F*)gROOT->FindObject(hName.data());
      htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()));
    } else if (tokenizedBinning.size() == 6) { // TH2
      auto htmp = (TH2F*)gROOT->FindObject(hName.data());
      ILOG(Debug) << "config: rebinning TH2 " << hName << " -> " << binning << ENDM;
      htmp->SetBins(std::atof(tokenizedBinning[0].c_str()), std::atof(tokenizedBinning[1].c_str()), std::atof(tokenizedBinning[2].c_str()),
                    std::atof(tokenizedBinning[3].c_str()), std::atof(tokenizedBinning[4].c_str()), std::atof(tokenizedBinning[5].c_str()));
    } else {
      ILOG(Warning) << "config: invalid binning parameter: " << hName << " -> " << binning << ENDM;
    }
  };

  const std::string rebinKeyword = "binning";
  const char* channelIdPlaceholder = "#";
  for (auto& param : mCustomParameters.getAllDefaults()) {
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
      ILOG(Warning) << "config: histogram named \"" << hName << "\" not found" << ENDM;
      continue;
    } else {
      rebinHisto(hName, binning);
    }
  }
}

void RecPointsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "@@@@initialize RecoQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time [ps]", NCHANNELS, 0, NCHANNELS, 500, -2050, 2050);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp [#ADC channels]", NCHANNELS, 0, NCHANNELS, 200, 0, 1000);
  mHistAmp2Ch->SetOption("colz");
  mHistCollTimeAC = std::make_unique<TH1F>("CollTimeAC", "(T0A+T0C)/2;ps", 100, -1000, 1000);
  mHistCollTimeA = std::make_unique<TH1F>("CollTimeA", "T0A;ps", 100, -1000, 1000);
  mHistCollTimeC = std::make_unique<TH1F>("CollTimeC", "T0C;ps", 100, -1000, 1000);
  mHistResCollTimeA = std::make_unique<TH1F>("ResCollTimeA", "(T0Aup-T0Adown)/2;ps", 100, -500, 500);
  mHistResCollTimeC = std::make_unique<TH1F>("ResCollTimeC", "(T0Cup-T0Cdown)/2;ps", 100, -500, 500);
  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);

  std::vector<unsigned int> vecChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  } else {
    for (unsigned int iCh = 0; iCh < o2::ft0::Constants::sNCHANNELS_PM; iCh++)
      vecChannelIDs.push_back(iCh);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }

  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->startPublishing(mHistCollTimeAC.get());
  getObjectsManager()->startPublishing(mHistCollTimeA.get());
  getObjectsManager()->startPublishing(mHistCollTimeC.get());
  getObjectsManager()->startPublishing(mHistResCollTimeA.get());
  getObjectsManager()->startPublishing(mHistResCollTimeC.get());

  for (const auto& chID : mSetAllowedChIDs) {
    auto pairHistAmpVsTime = mMapHistAmpVsTime.insert({ chID, new TH2F(Form("Amp_vs_time_channel%i", chID), Form("Amplitude vs time, channel %i;Amp;Time", chID), 420, -100, 500, 410, -2050, 2050) });
    if (pairHistAmpVsTime.second) {
      mListHistGarbage->Add(pairHistAmpVsTime.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
    }
  }

  ILOG(Info, Support) << "@@@ histos created" << ENDM;
  rebinFromConfig(); // after all histos are created
}

void RecPointsQcTask::startOfActivity(const Activity& activity)
{
  ILOG(Info, Support) << "@@@@ startOfActivity" << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistCollTimeAC->Reset();
  mHistCollTimeA->Reset();
  mHistCollTimeC->Reset();
  mHistResCollTimeA->Reset();
  mHistResCollTimeC->Reset();
  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

void RecPointsQcTask::startOfCycle()
{
  mTimeMinNS = -1;
  mTimeMaxNS = 0.;
  mTimeCurNS = 0.;
  mTfCounter = 0;
  mTimeSum = 0.;
}

void RecPointsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  double curTfTimeMin = -1;
  double curTfTimeMax = 0;
  mTfCounter++;
  auto chan = ctx.inputs().get<gsl::span<o2::ft0::ChannelDataFloat>>("channels");
  auto recpoints = ctx.inputs().get<gsl::span<o2::ft0::RecPoints>>("recpoints");
  bool isFirst = true;
  uint32_t firstOrbit;

  for (auto& recpoint : recpoints) {
    int time[o2::ft0::Constants::sNCHANNELS_PM] = { 0 };
    int amp[o2::ft0::Constants::sNCHANNELS_PM] = { 0 };
    o2::ft0::Triggers triggersignals = recpoint.getTrigger();
    bool vertexTrigger = triggersignals.getVertex();
    auto channels = recpoint.getBunchChannelData(chan);
    mHistCollTimeAC->Fill(static_cast<Float_t>(recpoint.getCollisionTimeMean()));
    mHistCollTimeA->Fill(static_cast<Float_t>(recpoint.getCollisionTimeA()));
    mHistCollTimeC->Fill(static_cast<Float_t>(recpoint.getCollisionTimeC()));
    for (const auto& chData : channels) {
      time[chData.ChId] = chData.CFDTime;
      amp[chData.ChId] = chData.QTCAmpl;
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.CFDTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(chData.ChId)) != mSetAllowedChIDs.end()) {
        mMapHistAmpVsTime[chData.ChId]->Fill(chData.QTCAmpl, chData.CFDTime);
      }
    }
    if (vertexTrigger) {
      int avtimeAup = 0, avtimeAdown = 0, naup = 0, nadown = 0;
      int avtimeCup = 0, avtimeCdown = 0, ncup = 0, ncdown = 0;
      for (int ich = 0; ich < 48; ich++) {
        if (amp[ich] > 5 && std::abs(time[ich]) < 1000) {
          avtimeAup += time[ich];
          naup++;
        }
      }
      if (naup > 0)
        avtimeAup /= naup;
      for (int ich = 48; ich < 96; ich++) {
        if (amp[ich] > 5 && std::abs(time[ich]) < 1000) {
          avtimeAdown += time[ich];
          nadown++;
        }
      }
      if (nadown > 0)
        avtimeAdown /= nadown;
      if (nadown > 0 && naup > 0) {
        mHistResCollTimeA->Fill((avtimeAdown - avtimeAup) / 2);
      }
      for (int ich = 96; ich < 152; ich++) {
        if (amp[ich] > 5 && std::abs(time[ich]) < 1000) {
          avtimeCup += time[ich];
          ncup++;
        }
      }
      if (ncup > 0)
        avtimeCup /= ncup;
      for (int ich = 152; ich < 208; ich++) {
        if (amp[ich] > 14 && std::abs(time[ich]) < 1000) {
          avtimeCdown += time[ich];
          ncdown++;
        }
      }
      if (ncdown > 0)
        avtimeCdown /= ncdown;
      if (ncdown > 0 && ncup > 0) {
        mHistResCollTimeC->Fill((avtimeCdown - avtimeCup) / 2);
      }
    }
  }
  mTimeSum += curTfTimeMax - curTfTimeMin;
}

void RecPointsQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
}

void RecPointsQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void RecPointsQcTask::reset()
{
  // clean all the monitor objects here
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistCollTimeAC->Reset();
  mHistCollTimeA->Reset();
  mHistCollTimeC->Reset();
  mHistResCollTimeA->Reset();
  mHistResCollTimeC->Reset();
  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}

} // namespace o2::quality_control_modules::ft0
