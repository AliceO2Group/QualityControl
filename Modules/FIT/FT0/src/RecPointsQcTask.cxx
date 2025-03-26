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
#include "DataFormatsParameters/GRPLHCIFData.h"
#include "FITCommon/HelperHist.h"
#include "FITCommon/HelperCommon.h"
#include "DetectorsBase/GRPGeomHelper.h"
#include "CommonDataFormat/BunchFilling.h"
namespace o2::quality_control_modules::ft0
{

using namespace o2::quality_control_modules::fit;

RecPointsQcTask::~RecPointsQcTask()
{
  delete mListHistGarbage;
}
void RecPointsQcTask::initHists()
{
  for (int iCh = 0; iCh < sNCHANNELS; iCh++) {
    const std::string name = fmt::format("hAmpVsTime_ch{}", iCh);
    const std::string title = fmt::format("Amp Vs Time channelID {}; Amp [ADC]; Time [ps]", iCh);
    mArrAmpTimeDistribution[iCh] = o2::fit::AmpTimeDistribution(name, title, 200, -2000., 2000., 50, 4095, 0); // in total 315 bins along x-axis
    getObjectsManager()->startPublishing(mArrAmpTimeDistribution[iCh].mHist.get());
  }
}

void RecPointsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "@@@@initialize RecoQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time [ps]", sNCHANNELS, 0, sNCHANNELS, 500, -2050, 2050);
  mHistTime2Ch->SetOption("colz");
  mHistAmp2Ch = std::make_unique<TH2F>("AmpPerChannel", "Amplitude vs Channel;Channel;Amp [#ADC channels]", sNCHANNELS, 0, sNCHANNELS, 200, 0, 1000);
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
    vecChannelIDs = helper::parseParameters<unsigned int>(chIDs, del);
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

  const std::tuple<int, float, float> binsTime{ 500, -2500, 2500 };
  const std::tuple<int, float, float> binsBC{ 3564, 0, 3564 };
  mTrgPos_minBias = mMapTrgBits.size();
  mMapTrgBits.insert({ mTrgPos_minBias, "MinBias" });
  mTrgPos_allEvents = mMapTrgBits.size();
  mMapTrgBits.insert({ mTrgPos_allEvents, "AllEvents" });
  mHistSumTimeAC_perTrg = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "SumTimeAC_perTrg", "(T0A+T0C)/2 per Trigger;Time [ps]; Trigger", binsTime, mMapTrgBits);
  mHistDiffTimeAC_perTrg = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "DiffTimeAC_perTrg", "(T0C-T0C)/2 per Trigger;Time [ps]; Trigger", binsTime, mMapTrgBits);
  mHistTimeA_perTrg = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "TimeA_perTrg", "T0A per Trigger;Time [ps]; Trigger", binsTime, mMapTrgBits);
  mHistTimeC_perTrg = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "TimeC_perTrg", "T0C per Trigger;Time [ps]; Trigger", binsTime, mMapTrgBits);
  mHistBC_perTriggers = helper::registerHist<TH2F>(getObjectsManager(), PublicationPolicy::Forever, "COLZ", "BC_perTriggers", "BC per Triggers;BC; Trigger", binsBC, mMapTrgBits);
  /*
    for (const auto& chID : mSetAllowedChIDs) {
      auto pairHistAmpVsTime = mMapHistAmpVsTime.insert({ chID, new TH2F(Form("Amp_vs_time_channel%i", chID), Form("Amplitude vs time, channel %i;Amp;Time", chID), 1000, 0, 4000, 100, -1000, 1000) });
      if (pairHistAmpVsTime.second) {
        mListHistGarbage->Add(pairHistAmpVsTime.first->second);
        getObjectsManager()->startPublishing(pairHistAmpVsTime.first->second);
      }
    }
  */
  initHists();
  ILOG(Info, Support) << "@@@ histos created" << ENDM;
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
  mHistSumTimeAC_perTrg->Reset();
  mHistDiffTimeAC_perTrg->Reset();
  mHistTimeA_perTrg->Reset();
  mHistTimeC_perTrg->Reset();
  mHistBC_perTriggers->Reset();
  for (int iCh = 0; iCh < sNCHANNELS; iCh++) {
    mArrAmpTimeDistribution[iCh].mHist->Reset();
  }
}

void RecPointsQcTask::startOfCycle()
{
}

void RecPointsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto chan = ctx.inputs().get<gsl::span<o2::ft0::ChannelDataFloat>>("channels");
  auto recpoints = ctx.inputs().get<gsl::span<o2::ft0::RecPoints>>("recpoints");
  const auto& grplhcif = o2::base::GRPGeomHelper::instance().getGRPLHCIF();
  const auto& bcSchema = grplhcif ? grplhcif->getBunchFilling().getBCPattern() : o2::BunchFilling::Pattern{};
  const bool isPbPb = grplhcif ? grplhcif->getAtomicNumberB1() == 82 && grplhcif->getAtomicNumberB2() == 82 : false;
  for (const auto& recpoint : recpoints) {
    const auto bc = recpoint.getInteractionRecord().bc;
    int time[o2::ft0::Constants::sNCHANNELS_PM] = { 0 };
    int amp[o2::ft0::Constants::sNCHANNELS_PM] = { 0 };
    o2::ft0::Triggers triggersignals = recpoint.getTrigger();
    bool vertexTrigger = triggersignals.getVertex();
    auto channels = recpoint.getBunchChannelData(chan);
    mHistCollTimeAC->Fill(static_cast<Float_t>(recpoint.getCollisionTimeMean()));
    mHistCollTimeA->Fill(static_cast<Float_t>(recpoint.getCollisionTimeA()));
    mHistCollTimeC->Fill(static_cast<Float_t>(recpoint.getCollisionTimeC()));
    const bool minBias = triggersignals.getVertex() && (triggersignals.getCen() || triggersignals.getSCen());
    // Preparing trigger word
    uint64_t trgWord = triggersignals.getTriggersignals() & 0b11111;
    trgWord |= (static_cast<int>(minBias) << mTrgPos_minBias);
    trgWord |= (1ull << mTrgPos_allEvents);
    // Filling hists with trg bits
    for (const auto& entry : mMapTrgBits) {
      if ((trgWord & (1ull << entry.first)) > 0) {
        mHistBC_perTriggers->Fill(static_cast<double>(bc), static_cast<double>(entry.first));
        const auto timeA = static_cast<double>(recpoint.getCollisionTimeA());
        const auto timeC = static_cast<double>(recpoint.getCollisionTimeC());
        mHistTimeA_perTrg->Fill(timeA, static_cast<double>(entry.first));
        mHistTimeC_perTrg->Fill(timeC, static_cast<double>(entry.first));
        mHistDiffTimeAC_perTrg->Fill(static_cast<double>(recpoint.getVertex()), static_cast<double>(entry.first));
        if (timeA < o2::ft0::RecPoints::sDummyCollissionTime && timeC < o2::ft0::RecPoints::sDummyCollissionTime) {
          mHistSumTimeAC_perTrg->Fill(static_cast<double>(recpoint.getCollisionTimeMean()), static_cast<double>(entry.first));
        }
      }
    }
    const bool isMinBiasEvent = (isPbPb && minBias) || (!isPbPb && triggersignals.getVertex()); // for pp only vertex, for PbPb vrt && (CENT || SCENT)
    for (const auto& chData : channels) {
      time[chData.ChId] = chData.CFDTime;
      amp[chData.ChId] = chData.QTCAmpl;
      mHistTime2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.CFDTime));
      mHistAmp2Ch->Fill(static_cast<Double_t>(chData.ChId), static_cast<Double_t>(chData.QTCAmpl));
      if (bcSchema.test(bc) && isMinBiasEvent) { // ampt-time dependency
        mArrAmpTimeDistribution[chData.ChId].mHist->Fill(chData.QTCAmpl, chData.CFDTime);
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
  mHistSumTimeAC_perTrg->Reset();
  mHistDiffTimeAC_perTrg->Reset();
  mHistTimeA_perTrg->Reset();
  mHistTimeC_perTrg->Reset();
  mHistBC_perTriggers->Reset();
  for (int iCh = 0; iCh < sNCHANNELS; iCh++) {
    mArrAmpTimeDistribution[iCh].mHist->Reset();
  }
}

} // namespace o2::quality_control_modules::ft0
