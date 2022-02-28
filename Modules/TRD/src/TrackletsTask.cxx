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
/// \file   TrackletsTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>

#include "QualityControl/QcInfoLogger.h"
#include "TRD/TrackletsTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/NoiseCalibration.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "CCDB/BasicCCDBManager.h"

namespace o2::quality_control_modules::trd
{

TrackletsTask::~TrackletsTask()
{
}

void TrackletsTask::drawLinesMCM(TH2F* histo)
{

  TLine* l;
  Int_t nPos[o2::trd::constants::NSTACK - 1] = { 16, 32, 44, 60 };

  for (Int_t iStack = 0; iStack < o2::trd::constants::NSTACK - 1; ++iStack) {
    l = new TLine(nPos[iStack] - 0.5, -0.5, nPos[iStack] - 0.5, 47.5);
    l->SetLineStyle(2);
    //std::cout << " adding vertical line to histo" << std::endl;
    //std::cout << " l = new TLine(" << nPos[iStack] - 0.5 << "," << -0.5 << "," << nPos[iStack] - 0.5 << "," << 47.5 << ");" << std::endl;
    histo->GetListOfFunctions()->Add(l);
  }

  for (Int_t iLayer = 0; iLayer < o2::trd::constants::NLAYER; ++iLayer) {
    l = new TLine(-0.5, iLayer * 8 - 0.5, 75.5, iLayer * 8 - 0.5);
    l = new TLine(0.5, iLayer * 8 - 0.5, 75.5, iLayer * 8 - 0.5);
    l->SetLineStyle(2);
    //std::cout << " adding horizontal line to histo" << std::endl;
    //std::cout << " l = new TLine(" << -0.5 <<","<< iLayer * 8 - 0.5 << "," << 75.5 << "," << iLayer * 8 - 0.5 << ");" << std::endl;
    histo->GetListOfFunctions()->Add(l);
  }
}

void TrackletsTask::connectToCCDB()
{
  auto& ccdbmgr = o2::ccdb::BasicCCDBManager::instance();
  //ccdbmgr.setURL("http://localhost:8080");
  mNoiseMap.reset(ccdbmgr.get<o2::trd::NoiseStatusMCM>("/TRD/Calib/NoiseMapMCM"));
}
void TrackletsTask::buildHistograms()
{
  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    std::string label = fmt::format("TrackletHCMCM_{0}", sm);
    std::string title = fmt::format("MCM in Tracklets data stream for sector {0}", sm);
    moHCMCM[sm].reset(new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 5, -0.5, 8 * 5 - 0.5));
    moHCMCM[sm]->GetYaxis()->SetTitle("ROB in stack");
    moHCMCM[sm]->GetXaxis()->SetTitle("mcm in rob in layer");
    getObjectsManager()->startPublishing(moHCMCM[sm].get());
    getObjectsManager()->setDefaultDrawOptions(moHCMCM[sm]->GetName(), "COLZ");
    drawLinesMCM(moHCMCM[sm].get());
  }
  mTrackletSlope.reset(new TH1F("trackletslope", "uncalibrated Slope of tracklets", 1024, -6.0, 6.0)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlope.get());
  mTrackletSlopeRaw.reset(new TH1F("trackletsloperaw", "Raw Slope of tracklets", 256, 0, 256)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopeRaw.get());
  mTrackletHCID.reset(new TH1F("tracklethcid", "Tracklet distribution over Halfchambers", 1080, 0, 1080));
  getObjectsManager()->startPublishing(mTrackletHCID.get());
  mTrackletPosition.reset(new TH1F("trackletpos", "Uncalibrated Position of Tracklets", 1400, -70, 70));
  getObjectsManager()->startPublishing(mTrackletPosition.get());
  mTrackletPositionRaw.reset(new TH1F("trackletposraw", "Raw Position of Tracklets", 2048, 0, 2048));
  getObjectsManager()->startPublishing(mTrackletPositionRaw.get());
  mTrackletsPerEvent.reset(new TH1F("trackletsperevent", "Number of Tracklets per event", 2500, 0, 250000));
  getObjectsManager()->startPublishing(mTrackletsPerEvent.get());

  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    std::string label = fmt::format("TrackletHCMCMnoise_{0}", sm);
    std::string title = fmt::format("MCM in Tracklets data stream for sector {0} noise in", sm);
    moHCMCMn[sm].reset(new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 5, -0.5, 8 * 5 - 0.5));
    moHCMCMn[sm]->GetYaxis()->SetTitle("ROB in stack");
    moHCMCMn[sm]->GetXaxis()->SetTitle("mcm in rob in layer");
    getObjectsManager()->startPublishing(moHCMCMn[sm].get());
    getObjectsManager()->setDefaultDrawOptions(moHCMCMn[sm]->GetName(), "COLZ");
    drawLinesMCM(moHCMCM[sm].get());
  }
  mTrackletSlopen.reset(new TH1F("trackletslopenoise", "uncalibrated Slope of tracklets noise in", 1024, -6.0, 6.0)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopen.get());
  mTrackletSlopeRawn.reset(new TH1F("trackletsloperawnoise", "Raw Slope of tracklets noise in", 256, 0, 256)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopeRawn.get());
  mTrackletHCIDn.reset(new TH1F("tracklethcidnoise", "Tracklet distribution over Halfchambers noise in", 1080, 0, 1080));
  getObjectsManager()->startPublishing(mTrackletHCIDn.get());
  mTrackletPositionn.reset(new TH1F("trackletposnoise", "Uncalibrated Position of Tracklets noise in", 1400, -70, 70));
  getObjectsManager()->startPublishing(mTrackletPositionn.get());
  mTrackletPositionRawn.reset(new TH1F("trackletposrawnoise", "Raw Position of Tracklets noise in", 2048, 0, 2048));
  getObjectsManager()->startPublishing(mTrackletPositionRawn.get());
  mTrackletsPerEventn.reset(new TH1F("trackletspereventn", "Number of Tracklets per event noise in", 2500, 0, 250000));
  getObjectsManager()->startPublishing(mTrackletsPerEventn.get());
}

void TrackletsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TrackletsTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  buildHistograms();
  connectToCCDB();
}

void TrackletsTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    moHCMCM[sm]->Reset();
  }
}

void TrackletsTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TrackletsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr && input.payload != nullptr) {

      auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
      auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
      auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
      for (auto& trigger : triggerrecords) {
        if (trigger.getNumberOfTracklets() == 0)
          continue; //bail if we have no digits in this trigger
        //now sort digits to det,row,pad
        mTrackletsPerEvent->Fill(trigger.getNumberOfTracklets());
        for (int currenttracklet = trigger.getFirstTracklet(); currenttracklet < trigger.getFirstTracklet() + trigger.getNumberOfTracklets() - 1; ++currenttracklet) {
          int detector = tracklets[currenttracklet].getDetector();
          int sm = detector / 30;
          int detLoc = detector % 30;
          int layer = detector % 6;
          int istack = detLoc / 6;
          int iChamber = sm * 30 + istack * o2::trd::constants::NLAYER + layer;
          int stackoffset = istack * o2::trd::constants::NSTACK * o2::trd::constants::NROBC1;
          if (istack >= 2) {
            stackoffset -= 2; // only 12in stack 2
          }
          //8 rob x 16 mcm each per chamber
          // 5 stack(y), 6 layers(x)
          // y=stack_rob, x=layer_mcm
          int x = o2::trd::constants::NMCMROB * layer + tracklets[currenttracklet].getMCM();
          int y = o2::trd::constants::NROBC1 * istack + tracklets[currenttracklet].getROB();
          if (mNoiseMap.get()->isTrackletFromNoisyMCM(tracklets[currenttracklet])) {
            moHCMCMn[sm]->Fill(x, y);
            mTrackletSlopen->Fill(tracklets[currenttracklet].getUncalibratedDy());
            mTrackletSlopeRawn->Fill(tracklets[currenttracklet].getSlope());
            mTrackletPositionn->Fill(tracklets[currenttracklet].getUncalibratedY());
            mTrackletPositionRawn->Fill(tracklets[currenttracklet].getPosition());
            mTrackletHCIDn->Fill(tracklets[currenttracklet].getHCID());
          } else {
            moHCMCM[sm]->Fill(x, y);
            mTrackletSlope->Fill(tracklets[currenttracklet].getUncalibratedDy());
            mTrackletSlopeRaw->Fill(tracklets[currenttracklet].getSlope());
            mTrackletPosition->Fill(tracklets[currenttracklet].getUncalibratedY());
            mTrackletPositionRaw->Fill(tracklets[currenttracklet].getPosition());
            mTrackletHCID->Fill(tracklets[currenttracklet].getHCID());
          }
        }
      }
    }
  }
}

void TrackletsTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  //scale 2d mHCMCM plots so they all have the same max height.
  int max = 0;
  for (auto& hist : moHCMCM) {
    if (hist->GetMaximum() > max) {
      max = hist->GetMaximum();
    }
  }
  for (auto& hist : moHCMCM) {
    hist->SetMaximum(max);
  }
}

void TrackletsTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TrackletsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mTrackletPosition.get()->Reset();
  mTrackletPositionRaw.get()->Reset();
  mTrackletSlope.get()->Reset();
  mTrackletSlopeRaw.get()->Reset();
  mTrackletHCID.get()->Reset();
  mTrackletsPerEvent.get()->Reset();
}

} // namespace o2::quality_control_modules::trd
