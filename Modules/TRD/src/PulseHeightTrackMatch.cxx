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
    ILOG(Info, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Info, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
}

void PulseHeightTrackMatch::buildHistograms()
{
  mParsingTimePerTF.reset(new TH1F("parsingtimeperTF", "Time for processing each TF", 100000, 0, 100000));
  mParsingTimePerTF->GetXaxis()->SetTitle("Time in #mus");
  mParsingTimePerTF->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mParsingTimePerTF.get());

  mDigitsPerEvent.reset(new TH1F("digitsperevent", "Digits per Event", 10000, 0, 10000));
  getObjectsManager()->startPublishing(mDigitsPerEvent.get());

  mTracksPerEvent.reset(new TH1F("tracksperevent", "Matched TRD tracks per Event", 100, 0, 10));
  getObjectsManager()->startPublishing(mTracksPerEvent.get());

  mTrackletsPerMatchedTrack.reset(new TH1F("trackletspermatchedtrack", "Tracklets per matched TRD track", 100, 0, 10));
  getObjectsManager()->startPublishing(mTrackletsPerMatchedTrack.get());

  mTriggerPerTF.reset(new TH1F("triggerpertimeframe", "Triggers per TF", 1000, 0, 1000));
  getObjectsManager()->startPublishing(mTriggerPerTF.get());

  mTriggerWDigitPerTF.reset(new TH1F("triggerwdpertimeframe", "Triggers with Digits per TF", 10, 0, 10));
  getObjectsManager()->startPublishing(mTriggerWDigitPerTF.get());

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
  ILOG(Info) << "initialize TRDDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  retrieveCCDBSettings();
  buildHistograms();
}

void PulseHeightTrackMatch::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
} // set stats/stacs

void PulseHeightTrackMatch::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

bool digitIndexCompare_phtm(unsigned int A, unsigned int B, const std::vector<o2::trd::Digit>& originalDigits)
{
  // sort into ROC:padrow
  const o2::trd::Digit *a, *b;
  a = &originalDigits[A];
  b = &originalDigits[B];
  if (a->getDetector() < b->getDetector()) {
    return 1;
  }
  if (a->getDetector() > b->getDetector()) {
    return 0;
  }
  if (a->getPadRow() < b->getPadRow()) {
    return 1;
  }
  if (a->getPadRow() > b->getPadRow()) {
    return 0;
  }
  if (a->getPadCol() < b->getPadCol()) {
    return 1;
  }
  return 0;
}

void PulseHeightTrackMatch::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
    auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
    auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
    auto tracks = ctx.inputs().get<gsl::span<o2::trd::TrackTRD>>("tracks");
    auto triggerrecordstracks = ctx.inputs().get<gsl::span<o2::trd::TrackTriggerRecord>>("trigrectrk");

    std::vector<o2::trd::Digit> digitv(digits.begin(), digits.end());

    if (digitv.size() == 0)
      continue;

    std::vector<unsigned int> digitsIndex(digitv.size());
    std::iota(digitsIndex.begin(), digitsIndex.end(), 0);

    int nTrgWDigits = 0;
    int nTotalTrigger = 0;
    auto crustart = std::chrono::high_resolution_clock::now();

    for (const auto& trigger : triggerrecords) {
      // printf("-------------Trigger----------------------BCData for Trigger %i, %i \n ",trigger.getBCData().orbit,trigger.getBCData().bc);
      nTotalTrigger++;

      if (trigger.getNumberOfDigits() == 0) {
        continue;
      }

      else {
        nTrgWDigits++;
      }

      if (trigger.getNumberOfDigits() > 10000) {
        mDigitsPerEvent->Fill(9999);
      } else {
        mDigitsPerEvent->Fill(trigger.getNumberOfDigits());
      }

      // now sort digits to det,row,pad
      std::sort(std::begin(digitsIndex) + trigger.getFirstDigit(), std::begin(digitsIndex) + trigger.getFirstDigit() + trigger.getNumberOfDigits(),
                [&digitv](unsigned int i, unsigned int j) { return digitIndexCompare_phtm(i, j, digitv); });

      for (const auto& triggerTracks : triggerrecordstracks) {

        if (trigger.getBCData() != triggerTracks.getBCData()) {
          continue;
        }

        int nTracks = 0;
        for (int iTrack = triggerTracks.getFirstTrack(); iTrack < triggerTracks.getFirstTrack() + triggerTracks.getNumberOfTracks(); iTrack++) {
          nTracks++;
          const auto& track = tracks[iTrack];

          printf("Got track with %i tracklets and ID %i \n", track.getNtracklets(), track.getRefGlobalTrackId());
          int ntracklets = track.getNtracklets();
          if (ntracklets > 10)
            mTrackletsPerMatchedTrack->Fill(9);
          else
            mTrackletsPerMatchedTrack->Fill(ntracklets);

          for (int iLayer = 0; iLayer < 6; iLayer++) {
            int trackletIndex = track.getTrackletIndex(iLayer);

            if (trackletIndex == -1) {
              continue;
            }

            const auto& trklt = tracklets[trackletIndex];

            int det = trklt.getDetector();
            int sec = det / 30;
            int row = trklt.getPadRow();
            int col = trklt.getPadCol();

            // obtain pad number relative to MCM center
            // int padLocal = trklt.getPositionBinSigned() * o2::trd::constants::GRANULARITYTRKLPOS;
            // MCM number in column direction (0..7)
            // int mcmCol = (trklt.getMCM() % o2::trd::constants::NMCMROBINCOL) + o2::trd::constants::NMCMROBINCOL * (trklt.getROB() % 2);
            // int colInChamber = 6.f + mcmCol * ((float)o2::trd::constants::NCOLMCM) + padLocal;

            for (int iDigit = trigger.getFirstDigit() + 1; iDigit < trigger.getFirstDigit() + trigger.getNumberOfDigits() - 1; ++iDigit)

            {
              const auto& digit = digits[iDigit];
              const auto& digitBefore = digits[iDigit - 1];
              const auto& digitAfter = digits[iDigit + 1];

              std::tuple<unsigned int, unsigned int, unsigned int> aa, ba, ca;
              aa = std::make_tuple(digitBefore.getDetector(), digitBefore.getPadRow(), digitBefore.getPadCol());
              ba = std::make_tuple(digit.getDetector(), digit.getPadRow(), digit.getPadCol());
              ca = std::make_tuple(digitAfter.getDetector(), digitAfter.getPadRow(), digitAfter.getPadCol());

              auto [det1, row1, col1] = aa;
              auto [det2, row2, col2] = ba;
              auto [det3, row3, col3] = ca;

              bool consecutive = false;
              bool digitmatch = false;

              if (det == det2 && row == row2) {

                int coldif = col2 - col;
                if (TMath::Abs(coldif) < 4)
                  digitmatch = true;
              }
              if (det1 == det2 && det2 == det3 && row1 == row2 && row2 == row3 && col2 + 1 == col1 && col3 + 1 == col2)
                consecutive = true;

              if (digitmatch && consecutive) {

                int phVal = 0;

                for (int iTb = 0; iTb < o2::trd::constants::TIMEBINS; ++iTb) {
                  phVal = digitBefore.getADC()[iTb] + digit.getADC()[iTb] + digitAfter.getADC()[iTb];
                  mPulseHeightpro->Fill(iTb, phVal);
                  mPulseHeightperchamber->Fill(iTb, det1, phVal);
                }
              }
            }
          }
        }
        mTracksPerEvent->Fill(nTracks);
      }
    }
    if (nTotalTrigger > 1000)
      mTriggerPerTF->Fill(999);
    else
      mTriggerPerTF->Fill(nTotalTrigger);

    if (nTrgWDigits > 10)
      mTriggerWDigitPerTF->Fill(9);
    else
      mTriggerWDigitPerTF->Fill(nTrgWDigits);

    std::chrono::duration<double, std::micro> parsingtimeTF = std::chrono::high_resolution_clock::now() - crustart;
    auto t1 = (double)std::chrono::duration_cast<std::chrono::microseconds>(parsingtimeTF).count();
    if (t1 > 100000)
      mParsingTimePerTF->Fill(99999);
    else
      mParsingTimePerTF->Fill(t1);
  }
}

void PulseHeightTrackMatch::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void PulseHeightTrackMatch::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void PulseHeightTrackMatch::reset()
{
  // clean all the monitor objects here
  ILOG(Info) << "Resetting the histogram" << ENDM;
  mTracksPerEvent->Reset();
  mDigitsPerEvent->Reset();
  mTriggerPerTF->Reset();
  mTriggerWDigitPerTF->Reset();
  mParsingTimePerTF->Reset();
  mTrackletsPerMatchedTrack->Reset();

  mDigitsPerEvent.get()->Reset();
  mTriggerPerTF.get()->Reset();
  mTriggerWDigitPerTF.get()->Reset();
  mTracksPerEvent.get()->Reset();
  mPulseHeightpro.get()->Reset();
  mPulseHeightperchamber.get()->Reset();
  mParsingTimePerTF.get()->Reset();
  mTrackletsPerMatchedTrack.get()->Reset();
}
} // namespace o2::quality_control_modules::trd
