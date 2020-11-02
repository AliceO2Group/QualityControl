// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   BasicDigitQcTask.cxx
/// \author Milosz Filus
///

#include "TCanvas.h"
#include "TH1.h"
#include "TFile.h"
#include "TGraph.h"

#include "QualityControl/QcInfoLogger.h"
#include "FT0/BasicDigitQcTask.h"
#include "FT0/Utilities.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

namespace o2::quality_control_modules::ft0
{

BasicDigitQcTask::~BasicDigitQcTask()
{
}

void BasicDigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mChargeHistogram = std::make_unique<TH1F>("Charge", "Charge", 200, 0, 200);
  mTimeHistogram = std::make_unique<TH1F>("Time", "Time", 200, 0, 200);
  mAmplitudeAndTime = std::make_unique<TH2F>("ChargeAndTime", "ChargeAndTime", 10, 0, 200, 10, 0, 200);
  mTTree = std::make_unique<TTree>("EventTree", "EventTree");

  getObjectsManager()->startPublishing(mChargeHistogram.get());
  getObjectsManager()->startPublishing(mTimeHistogram.get());
  getObjectsManager()->startPublishing(mTTree.get());
  getObjectsManager()->startPublishing(mAmplitudeAndTime.get());
}

void BasicDigitQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mTTree->Reset();
}

void BasicDigitQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void BasicDigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");

  std::vector<o2::ft0::ChannelData> channelDataCopy(channels.begin(), channels.end());
  std::vector<o2::ft0::Digit> digitDataCopy(digits.begin(), digits.end());

  EventWithChannelData event;
  mTTree->Branch("EventWithChannelData", &event);

  for (auto& digit : digits) {
    auto currentChannels = digit.getBunchChannelData(channels);
    auto timestamp = o2::InteractionRecord::bc2ns(digit.getBC(), digit.getOrbit());
    event = EventWithChannelData{ digit.getEventID(), digit.getBC(), digit.getOrbit(), timestamp, std::vector<o2::ft0::ChannelData>(currentChannels.begin(), currentChannels.end()) };
    mTTree->Fill();

    for (auto& channel : currentChannels) {
      mChargeHistogram->Fill(channel.QTCAmpl);
      mTimeHistogram->Fill(channel.CFDTime);
      mAmplitudeAndTime->Fill(channel.QTCAmpl, channel.CFDTime);
    }
  }

  // saving TTree if you want to read it from disk
  // its required to make a copy of tree - but it is only for debbuging

  // auto hFile = std::make_unique<TFile>("debugTree.root", "RECREATE");
  // auto treeCopy = mTTree->CloneTree();
  // treeCopy->SetEntries();
  // treeCopy->Write();
  // mTTree->Print();
  // hFile->Close();
}

void BasicDigitQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicDigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicDigitQcTask::reset()
{
  // clean all the monitor objects here

  mTimeHistogram->Reset();
  mChargeHistogram->Reset();
  mTTree->Reset();
  mAmplitudeAndTime->Reset();
}

} // namespace o2::quality_control_modules::ft0
