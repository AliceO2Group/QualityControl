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
/// \file   PulseHeightTrackMatch.cxx
/// \author Vikash Sumberia
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TStopwatch.h>

#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "DataFormatsTRD/TrackTriggerRecord.h"
#include "DataFormatsTRD/TrackTRD.h"
#include "CommonDataFormat/TFIDInfo.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "TRD/PulseHeightTrackMatch.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <gsl/span>
#include <map>
#include <tuple>
#include "CCDB/BasicCCDBManager.h"
namespace o2::quality_control_modules::trd
{
PulseHeightTrackMatch::~PulseHeightTrackMatch()
{
}

void PulseHeightTrackMatch::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
}

void PulseHeightTrackMatch::buildHistograms()
{
  mParsingTimePerTF.reset(new TH1F("parsingtimeperTF", "Time for processing each TF", 100000, 0, 100000));
  mParsingTimePerTF->GetXaxis()->SetTitle("Time in #mus");
  mParsingTimePerTF->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mParsingTimePerTF.get());

  mTracksPerEvent.reset(new TH1F("tracksperevent", "Matched TRD tracks per Event", 100, 0, 10));
  getObjectsManager()->startPublishing(mTracksPerEvent.get());

  mTrackletsPerMatchedTrack.reset(new TH1F("trackletspermatchedtrack", "Tracklets per matched TRD track", 100, 0, 10));
  getObjectsManager()->startPublishing(mTrackletsPerMatchedTrack.get());

  mPulseHeightpro.reset(new TProfile("PulseHeight/mPulseHeightpro", "Pulse height profile  plot;Timebins;Counts", 30, -0.5, 29.5));
  mPulseHeightpro.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightpro.get());

  mPulseHeightperchamber.reset(new TProfile2D("PulseHeight/mPulseHeightperchamber", "mPulseHeightperchamber;Timebin;Chamber", 30, -0.5, 29.5, 540, 0, 540));
  mPulseHeightperchamber.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightperchamber.get());
  getObjectsManager()->setDefaultDrawOptions(mPulseHeightperchamber.get()->GetName(), "colz");
}

void PulseHeightTrackMatch::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TRDPulseHeightQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  retrieveCCDBSettings();
  buildHistograms();
}

void PulseHeightTrackMatch::startOfActivity(Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
} // set stats/stacs

void PulseHeightTrackMatch::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void PulseHeightTrackMatch::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
  auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
  auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
  auto tracks = ctx.inputs().get<gsl::span<o2::trd::TrackTRD>>("tracks");
  auto triggerrecordstracks = ctx.inputs().get<gsl::span<o2::trd::TrackTriggerRecord>>("trigrectrk");

  auto crustart = std::chrono::high_resolution_clock::now();

  for (const auto& trigger : triggerrecords) {

    if (trigger.getNumberOfDigits() == 0) {
      continue;
    }

    for (const auto& triggerTracks : triggerrecordstracks) {

      if (trigger.getBCData() != triggerTracks.getBCData()) { // matching bunch crossing data from TRD trigger abd TRD track trigger
        continue;
      }

      mTracksPerEvent->Fill(triggerTracks.getNumberOfTracks());
      if (tracks[triggerTracks.getFirstTrack()].getPileUpTimeShiftMUS() < 0.7)
        continue; // rejecting triggers which end up in pileup

      for (int iTrack = triggerTracks.getFirstTrack(); iTrack < triggerTracks.getFirstTrack() + triggerTracks.getNumberOfTracks(); iTrack++) {

        const auto& track = tracks[iTrack];
        int ntracklets = track.getNtracklets();
        mTrackletsPerMatchedTrack->Fill(ntracklets);

        for (int iLayer = 0; iLayer < 6; iLayer++) {
          int trackletIndex = track.getTrackletIndex(iLayer);

          if (trackletIndex == -1) {
            continue;
          }

          const auto& trklt = tracklets[trackletIndex];

          // obtain pad number relative to MCM center
          // int padLocal = trklt.getPositionBinSigned() * o2::trd::constants::GRANULARITYTRKLPOS;
          // MCM number in column direction (0..7)
          // int mcmCol = (trklt.getMCM() % o2::trd::constants::NMCMROBINCOL) + o2::trd::constants::NMCMROBINCOL * (trklt.getROB() % 2);
          // int col = 6.f + mcmCol * ((float)o2::trd::constants::NCOLMCM) + padLocal;

          int coldif;
          for (int iDigit = trigger.getFirstDigit() + 1; iDigit < trigger.getFirstDigit() + trigger.getNumberOfDigits() - 1; ++iDigit) //+1 and -1 for finding the consecutive digits

          {
            const auto& digit = digits[iDigit];
            if (digit.getDetector() != trklt.getDetector())
              continue;
            if (digit.getPadRow() != trklt.getPadRow())
              continue;
            coldif = digit.getPadCol() - trklt.getPadCol();
            if (TMath::Abs(coldif) > 1)
              continue; // PadCol may be off by +-1 due to the slope of tracklet
            const auto& digitBefore = digits[iDigit - 1];
            const auto& digitAfter = digits[iDigit + 1];

            bool consecutive = false;
            if (digitBefore.getDetector() == digitAfter.getDetector() && digitBefore.getPadRow() == digit.getPadRow() && digit.getPadRow() == digitAfter.getPadRow() && digit.getPadCol() + 1 == digitBefore.getPadCol() && digitAfter.getPadCol() + 1 == digit.getPadCol())
              consecutive = true;

            if (consecutive) {
              int phVal = 0;
              for (int iTb = 0; iTb < o2::trd::constants::TIMEBINS; ++iTb) {
                phVal = digitBefore.getADC()[iTb] + digit.getADC()[iTb] + digitAfter.getADC()[iTb];
                mPulseHeightpro->Fill(iTb, phVal);
                mPulseHeightperchamber->Fill(iTb, digit.getDetector(), phVal);
              }
            }
          }
        }
      }
    }
  }

  std::chrono::duration<double, std::micro> parsingtimeTF = std::chrono::high_resolution_clock::now() - crustart;
  auto t1 = (double)std::chrono::duration_cast<std::chrono::microseconds>(parsingtimeTF).count();
  mParsingTimePerTF->Fill(t1);
}

void PulseHeightTrackMatch::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void PulseHeightTrackMatch::endOfActivity(Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void PulseHeightTrackMatch::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histogram" << ENDM;
  mTracksPerEvent->Reset();
  mParsingTimePerTF->Reset();
  mTrackletsPerMatchedTrack->Reset();
  mPulseHeightpro->Reset();
  mPulseHeightperchamber->Reset();
}
} // namespace o2::quality_control_modules::trd
