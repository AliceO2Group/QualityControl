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
/// \file   RecPointsQcTask.cxx
/// \author Artur Furs afurs@cern.ch
/// developed by A Khuntia for FDD akhuntia@cern.ch

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TROOT.h>

#include "QualityControl/QcInfoLogger.h"
#include <FDD/RecPointsQcTask.h>
#include <DataFormatsFDD/ChannelData.h>
#include <Framework/InputRecord.h>
#include <vector>
#include "CommonConstants/PhysicsConstants.h"

namespace o2::quality_control_modules::fdd
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

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time [ns]", NCHANNELS, 0, NCHANNELS, 420, -10.50, 10.50);
  mVertexVsCollTimeAllBC = std::make_unique<TH2F>("VertexVsCollTimeAllBC", "FDD vertex vs Collision time;FDD vertex (cm);  Collision Time [ns]", 2001, -100.5, 100.5, 2100, -20.5, 20.5);
  mVertexVsCollTimeVertexTrigger = std::make_unique<TH2F>("VertexVsCollTimeVertexTrigger", "FDD vertex vs Collision time (Vertex trigger);FDD vertex (cm);  Collision Time [ns]", 2001, -100.5, 100.5, 2100, -20.5, 20.5);
  mVertexNsVsCollTimeAllBC = std::make_unique<TH2F>("VertexNsVsCollTimeAllBC", "FDD vertex vs Collision time;FDD vertex (ns);  Collision Time [ns]", 2100, -20.5, 20.5, 2100, -20.5, 20.5);
  mVertexNsVsCollTimeVertexTrigger = std::make_unique<TH2F>("VertexNsVsCollTimeVertexTrigger", "FDD vertex vs Collision time (Vertex trigger);FDD vertex (ns);  Collision Time [ns]", 2100, -20.5, 20.5, 2100, -20.5, 20.5);
  mTimeAvsTimeC = std::make_unique<TH2F>("TimeAvsTimeC", "FDD time A  vs time C;time A (ns);time C (ns)", 1610, -80.5, 80.5, 1610, -80.5, 80.5);
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp [#ADC channels]", NCHANNELS, 0, NCHANNELS, 2200, -100, 4100);
  mHistCollTimeA = std::make_unique<TH1F>("CollTimeA", "T0A;Time [ns]", 4100, -20.5, 20.5);
  mHistCollTimeC = std::make_unique<TH1F>("CollTimeC", "T0C;Time [ns]", 4100, -20.5, 20.5);
  mHistBC = std::make_unique<TH1F>("BC", "BC;BC;counts;", sBCperOrbit, 0, sBCperOrbit);
  mHistBCVetex = std::make_unique<TH1F>("BCVetex", "BC Vertex trigger;BC;counts;", sBCperOrbit, 0, sBCperOrbit);
  mHistBCorA = std::make_unique<TH1F>("BCorA", "BC orA;BC;counts;", sBCperOrbit, 0, sBCperOrbit);
  mHistBCorC = std::make_unique<TH1F>("BCorC", "BC orC;BC;counts;", sBCperOrbit, 0, sBCperOrbit);
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
  getObjectsManager()->startPublishing(mHistTime2Ch.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTime2Ch.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistAmp2Ch.get());
  getObjectsManager()->setDefaultDrawOptions(mHistAmp2Ch.get(), "COLZ");
  getObjectsManager()->startPublishing(mVertexVsCollTimeAllBC.get());
  getObjectsManager()->setDefaultDrawOptions(mVertexVsCollTimeAllBC.get(), "COLZ");
  getObjectsManager()->startPublishing(mVertexVsCollTimeVertexTrigger.get());
  getObjectsManager()->setDefaultDrawOptions(mVertexVsCollTimeVertexTrigger.get(), "COLZ");
  getObjectsManager()->startPublishing(mVertexNsVsCollTimeAllBC.get());
  getObjectsManager()->setDefaultDrawOptions(mVertexNsVsCollTimeAllBC.get(), "COLZ");
  getObjectsManager()->startPublishing(mVertexNsVsCollTimeVertexTrigger.get());
  getObjectsManager()->setDefaultDrawOptions(mVertexNsVsCollTimeVertexTrigger.get(), "COLZ");
  getObjectsManager()->startPublishing(mTimeAvsTimeC.get());
  getObjectsManager()->setDefaultDrawOptions(mTimeAvsTimeC.get(), "COLZ");
  getObjectsManager()->startPublishing(mHistCollTimeA.get());
  getObjectsManager()->startPublishing(mHistCollTimeC.get());
  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistBCVetex.get());
  getObjectsManager()->startPublishing(mHistBCorA.get());
  getObjectsManager()->startPublishing(mHistBCorC.get());

  for (const auto& chID : mSetAllowedChIDs) {
    auto pairHistAmpVsTime = mMapHistAmpVsTime.insert({ chID, new TH2F(Form("Amp_vs_time_channel%i", chID), Form("Amplitude vs time, channel %i;Amp;Time (ns)", chID), 2200, -100, 4100, 410, -20.5, 20.5) });
    if (pairHistAmpVsTime.second) {
      mListHistGarbage->Add(pairHistAmpVsTime.first->second);
      getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
      getObjectsManager()->setDefaultDrawOptions(pairHistAmpVsTime.first->second, "COLZ");
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
  mVertexVsCollTimeAllBC->Reset();
  mVertexVsCollTimeVertexTrigger->Reset();
  mVertexNsVsCollTimeAllBC->Reset();
  mVertexNsVsCollTimeVertexTrigger->Reset();
  mTimeAvsTimeC->Reset();
  mHistCollTimeA->Reset();
  mHistCollTimeC->Reset();
  mHistBC->Reset();
  mHistBCVetex->Reset();
  mHistBCorA->Reset();
  mHistBCorC->Reset();
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
  auto chan = ctx.inputs().get<gsl::span<o2::fdd::ChannelDataFloat>>("channels");
  auto recpoints = ctx.inputs().get<gsl::span<o2::fdd::RecPoint>>("recpoints");
  bool isFirst = true;
  uint32_t firstOrbit;
  bool isTCM = true;
  for (auto& recpoint : recpoints) {
    if (recpoint.getTrigger().getTimeA() == fit::Triggers::DEFAULT_TIME && recpoint.getTrigger().getTimeC() == fit::Triggers::DEFAULT_TIME) {
      isTCM = false;
    }
    o2::fdd::Triggers triggersignals = recpoint.getTrigger();

    auto channels = recpoint.getBunchChannelData(chan);
    mHistBC->Fill(recpoint.getInteractionRecord().bc);

    if (isTCM) {
      int bc = recpoint.getInteractionRecord().bc;
      bool vertexTrigger = triggersignals.getVertex();
      double collisionTimeA = static_cast<Double_t>(recpoint.getCollisionTimeA() * 1.e-3); // time ps-->ns
      double collisionTimeC = static_cast<Double_t>(recpoint.getCollisionTimeC() * 1.e-3); // time ps-->ns
      mHistCollTimeA->Fill(collisionTimeA);                                                // time ps-->ns
      mHistCollTimeC->Fill(collisionTimeC);                                                // time ps-->ns
      mTimeAvsTimeC->Fill(collisionTimeA, collisionTimeC);
      if ((collisionTimeA > -15 && collisionTimeA < 15) && (collisionTimeC > -15 && collisionTimeC < 15)) { // avoid dummytime
        mVertexVsCollTimeAllBC->Fill(((collisionTimeA - collisionTimeC) / 2) * o2::constants::physics::LightSpeedCm2NS, (collisionTimeA + collisionTimeC) / 2);
        mVertexNsVsCollTimeAllBC->Fill((collisionTimeA - collisionTimeC) / 2., (collisionTimeA + collisionTimeC) / 2.);
      }
      if (vertexTrigger) {
        mHistBCVetex->Fill(bc);
        mVertexVsCollTimeVertexTrigger->Fill(((collisionTimeA - collisionTimeC) / 2) * o2::constants::physics::LightSpeedCm2NS, (collisionTimeA + collisionTimeC) / 2);
        mVertexNsVsCollTimeVertexTrigger->Fill((collisionTimeA - collisionTimeC) / 2., (collisionTimeA + collisionTimeC) / 2.);
      }
      if (triggersignals.getOrA()) {
        mHistBCorA->Fill(bc);
      }
      if (triggersignals.getOrC()) {
        mHistBCorC->Fill(bc);
      }
    } /// TCM
    for (const auto& chData : channels) {
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.mPMNumber), static_cast<Double_t>(chData.mTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.mPMNumber), static_cast<Double_t>(chData.mChargeADC));
      if (mSetAllowedChIDs.find(static_cast<unsigned int>(chData.mPMNumber)) != mSetAllowedChIDs.end()) {
        mMapHistAmpVsTime[chData.mPMNumber]->Fill(chData.mChargeADC, chData.mTime);
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
  mVertexVsCollTimeAllBC->Reset();
  mVertexVsCollTimeVertexTrigger->Reset();
  mVertexNsVsCollTimeAllBC->Reset();
  mVertexNsVsCollTimeVertexTrigger->Reset();
  mTimeAvsTimeC->Reset();
  mHistCollTimeA->Reset();
  mHistCollTimeC->Reset();
  mHistBC->Reset();
  mHistBCVetex->Reset();
  mHistBCorA->Reset();
  mHistBCorC->Reset();
  for (auto& entry : mMapHistAmpVsTime) {
    entry.second->Reset();
  }
}
} // namespace o2::quality_control_modules::fdd
