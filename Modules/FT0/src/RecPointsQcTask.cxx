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

#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
#include "TROOT.h"

#include "QualityControl/QcInfoLogger.h"
#include "FT0/RecPointsQcTask.h"
#include "DataFormatsFT0/RecPoints.h"
#include "DataFormatsFT0/ChannelData.h"
#include "Framework/InputRecord.h"
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
      ILOG(Warning) << "config: histogram named \"" << hName << "\" not found" << ENDM;
      continue;
    } else {
      rebinHisto(hName, binning);
    }
  }
}

void RecPointsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize RecoQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mStateLastIR2Ch = {};

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time [ps]", NCHANNELS, 0, NCHANNELS, 200, -2050, 2050);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp [#ADC channels]", NCHANNELS, 0, NCHANNELS, 500, 0, 500);
  mHistAmp2Ch->SetOption("colz");
  mHistCollTimeAC = std::make_unique<TH1F>("CollTimeAC", "(T0A+T0C)/2;ps", 500, -2000, 2000);
  mHistCollTimeA = std::make_unique<TH1F>("CollTimeA", "T0A;ps", 500, -2000, 2000);
  mHistCollTimeC = std::make_unique<TH1F>("CollTimeC", "T0C;ps", 500, -2000, 2000);
  mHistResCollTimeA = std::make_unique<TH1F>("ResCollTimeA", "(T0Aup-T0Adown)/2;ps", 100, -500, 500);
  mHistResCollTimeC = std::make_unique<TH1F>("ResCollTimeC", "(T0Cup-T0Cdown)/2;ps", 100, -500, 500);
  mListHistGarbage = new TList();
  mListHistGarbage->SetOwner(kTRUE);
  mHistTimeSum2Diff = std::make_unique<TH2F>("timeSumVsDiff", "time A/C side: sum VS diff;(TOC-TOA)/2 [ns];(TOA+TOC)/2 [ns]", 400, -52.08, 52.08, 400, -52.08, 52.08); // range of 52.08 ns = 4000*13.02ps = 4000 channels
  mHistTimeSum2Diff->SetOption("colz");

  std::vector<unsigned int> vecChannelIDs;
  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChannelIDs = parseParameters<unsigned int>(chIDs, del);
  } else {
    for (unsigned int iCh = 0; iCh < NCHANNELS; iCh++)
      vecChannelIDs.push_back(iCh);
  }
  for (const auto& entry : vecChannelIDs) {
    mSetAllowedChIDs.insert(entry);
  }

  rebinFromConfig(); // after all histos are created

  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->startPublishing(mHistTimeSum2Diff.get());
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
}

void RecPointsQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistTimeSum2Diff->Reset();
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
  ILOG(Info, Support) << "startOfCycle" << ENDM;
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
  int channelTimeCut = 1000;

  mTfCounter++;
  auto chan = ctx.inputs().get<gsl::span<o2::ft0::ChannelDataFloat>>("channels");
  auto recpoints = ctx.inputs().get<gsl::span<o2::ft0::RecPoints>>("recpoints");

  bool isFirst = true;
  uint32_t firstOrbit;

  for (auto& recpoint : recpoints) {
    o2::ft0::Triggers triggersignals = recpoint.getTrigger();
    bool ora = triggersignals.getOrA();
    bool orc = triggersignals.getOrC();
    bool vertexTrigger = triggersignals.getVertex();
    auto channels = recpoint.getBunchChannelData(chan);
    mHistCollTimeAC->Fill(static_cast<Float_t>(recpoint.getCollisionTimeMean()));
    mHistCollTimeA->Fill(static_cast<Float_t>(recpoint.getCollisionTimeA()));
    mHistCollTimeC->Fill(static_cast<Float_t>(recpoint.getCollisionTimeC()));
    int time[NCHANNELS] = {}, amp[NCHANNELS] = {};
    for (const auto& chData : channels) {
      time[chData.ChId] = static_cast<int>(chData.CFDTime);
      amp[chData.ChId] = static_cast<int>(chData.QTCAmpl);
      mHistTime2Ch->Fill(static_cast<Float_t>(chData.ChId), static_cast<Float_t>(chData.CFDTime));
      mHistAmp2Ch->Fill(static_cast<Float_t>(chData.ChId), static_cast<Float_t>(chData.QTCAmpl));
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(chData.ChId)) != mSetAllowedChIDs.end()) {
        mMapHistAmpVsTime[chData.ChId]->Fill(chData.QTCAmpl, chData.CFDTime);
      }
    }
    // A and C resolution
    if (vertexTrigger) {
      int avtimeAup = 0, avtimeAdown = 0, naup = 0, nadown = 0;
      int avtimeCup = 0, avtimeCdown = 0, ncup = 0, ncdown = 0;
      int a_up = 2 * o2::ft0::Geometry::NCellsA ;            //A upper part 0 -47 ch; down : 48 - 95
      int c_up = 2 * a_up + 2 * o2::ft0::Geometry::NCellsC; //C upper 96 -151; C down 152-208
      for (int ich = 0; ich < a_up; ich++) {
        if (amp[ich] > 0 && std::abs(time[ich]) < channelTimeCut) {
          avtimeAup += time[ich];
          naup++;
        }
      }
      for (int ich = a_up; ich < 2 * a_up; ich++) {
        if (amp[ich] > 0 && std::abs(time[ich]) < channelTimeCut) {
          avtimeAdown += time[ich];
          nadown++;
        }
      }
      for (int ich = 2 * a_up; ich < c_up; ich++) {
        if (amp[ich] > 0 && std::abs(time[ich]) < channelTimeCut) {
          avtimeCup += time[ich];
          ncup++;
        }
      }
      for (int ich = c_up; ich < NCHANNELS; ich++) {
        if (amp[ich] > 0 && std::abs(time[ich]) < channelTimeCut) {
          avtimeCdown += time[ich];
          ncdown++;
        }
      }
      if (naup > 0) {
        avtimeAup /= naup;
      }
      if (nadown > 0) {
        avtimeAdown /= nadown;
      }
      if (nadown > 0 && naup > 0) {
        mHistResCollTimeA->Fill((avtimeAdown - avtimeAup) / 2);
        ILOG(Info) << " @@@ A res filled " << (avtimeAdown - avtimeAup) / 2 << ENDM;
      }

      if (ncup > 0) {
        avtimeCup /= ncup;
      }
      if (ncdown > 0) {
        avtimeCdown /= ncdown;
      }
      if (ncdown > 0 && ncup > 0) {
        mHistResCollTimeC->Fill((avtimeCdown - avtimeCup) / 2);
        ILOG(Info) << " @@@ C res filled " << (avtimeCdown - avtimeCup) / 2 << ENDM;
      }
    }
  }

  mTimeSum += curTfTimeMax - curTfTimeMin;
}

void RecPointsQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
}

void RecPointsQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void RecPointsQcTask::reset()
{
  // clean all the monitor objects here
  mHistTime2Ch->Reset();
  mHistAmp2Ch->Reset();
  mHistTimeSum2Diff->Reset();
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
