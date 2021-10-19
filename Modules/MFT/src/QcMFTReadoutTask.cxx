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
/// \file   QcMFTReadoutTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

// ROOT
#include <TH1.h>
#include <TH2.h>
// O2
#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
// #include <Framework/InputRecord.h> // needed??
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTReadoutTask.h"

using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::mft
{

QcMFTReadoutTask::~QcMFTReadoutTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTReadoutTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize QcMFTReadoutTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  // Defining summary histogram
  //==============================================
  mSummaryLaneStatus = std::make_unique<TH1F>("mMFTSummaryLaneStatus", "Lane Status Summary", 5, -0.5, 4.5);
  getObjectsManager()->startPublishing(mSummaryLaneStatus.get());
  mSummaryLaneStatus->GetXaxis()->SetBinLabel(1, "Missing data");
  mSummaryLaneStatus->GetXaxis()->SetBinLabel(2, "Warning");
  mSummaryLaneStatus->GetXaxis()->SetBinLabel(3, "Error");
  mSummaryLaneStatus->GetXaxis()->SetBinLabel(4, "Fault");
  mSummaryLaneStatus->GetXaxis()->SetBinLabel(5, "#Entries");
  mSummaryLaneStatus->GetYaxis()->SetTitle("#Entries");
  mSummaryLaneStatus->SetStats(0);

  //  Defining individual histograms
  //==============================================
  generateRUindexMap(); // create a map of possible FEEid to
  // histo name defined by half, disk, face, zone
  int zone = -1;
  int plane = -1;
  int disc = -1;
  int half = -1;
  for (int i = 0; i < maxNumberToIdentifyRU; i++) { // loop over potential IDs
    if (mIndexOfRUMap[i] == -1)
      continue;                                // skip not asigned IDs
    unpackRUindex(i, zone, plane, disc, half); // get geo info
    // define histogram
    auto histogramRU = std::make_unique<TH2F>(
      Form("mMFTIndividualLaneStatus/h%d-d%d-f%d-z%d", half, disc, plane, zone),
      Form("h%d-d%d-f%d-z%d", half, disc, plane, zone),
      25, -0.5, 24.5, // lanes
      4, -0.5, 3.5);  // status
    histogramRU->GetYaxis()->SetBinLabel(1, "OK");
    histogramRU->GetYaxis()->SetBinLabel(2, "Warning");
    histogramRU->GetYaxis()->SetBinLabel(3, "Error");
    histogramRU->GetYaxis()->SetBinLabel(4, "Fault");
    histogramRU->SetXTitle("Lane");
    histogramRU->SetStats(0);
    histogramRU->SetOption("colz");
    // push the histo into the vector of histograms and publish it
    mIndividualLaneStatus.push_back(std::move(histogramRU));
    getObjectsManager()->startPublishing(mIndividualLaneStatus[mIndexOfRUMap[i]].get());
  } // end-potential IDs
}

void QcMFTReadoutTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mSummaryLaneStatus->Reset();
  for (int i = 0; i < numberOfRU; i++) {
    mIndividualLaneStatus[i]->Reset();
  }
}

void QcMFTReadoutTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void QcMFTReadoutTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input
  DPLRawParser parser(ctx.inputs());

  // loop over input
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // get the header
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    // get detector field
    uint64_t summaryLaneStatus = rdh->detectorField;
    // fill histogram bin with #entries
    mSummaryLaneStatus->Fill(4);
    // fill status if set
    if (summaryLaneStatus & (1 << 0))
      mSummaryLaneStatus->Fill(0); // missing data
    if (summaryLaneStatus & (1 << 1))
      mSummaryLaneStatus->Fill(1); // warning
    if (summaryLaneStatus & (1 << 2))
      mSummaryLaneStatus->Fill(2); // error
    if (summaryLaneStatus & (1 << 3))
      mSummaryLaneStatus->Fill(3); // fault
    if (rdh->stop && it.size()) {
      auto const* ddw = reinterpret_cast<const MFTDDW*>(it.data());
      uint16_t ddwIndex = ddw->indexWord.indexBits.id;
      if (ddwIndex == 0xE4) { // it is a diagnostic data word
        uint64_t ddwLaneStatus = ddw->laneWord.laneBits.laneStatus;
        uint16_t rdhFeeIndex = rdh->feeId;
        int RUindex = (rdhFeeIndex & 127); // look only at the rightmost 7 bits
        // check the status of each lane
        for (int i = 0; i < 25; i++) {
          // get the two bits corresponding to the lane i
          int laneStatus = ((ddwLaneStatus >> (i * 2)) & (3));
          // fill the status in the histogram
          mIndividualLaneStatus[mIndexOfRUMap[RUindex]]->Fill(i, laneStatus);
        }
      } // end if is a DDW
    }   // end if rdh->stop

  } // end loop over input
}

void QcMFTReadoutTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void QcMFTReadoutTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void QcMFTReadoutTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mSummaryLaneStatus->Reset();
  for (int i = 0; i < numberOfRU; i++) {
    mIndividualLaneStatus[i]->Reset();
  }
}

void QcMFTReadoutTask::unpackRUindex(int RUindex, int& zone, int& plane, int& disc, int& half)
// unpacks RU ID into geometry information needed to name histograms
{
  zone = (RUindex & 3);         // first and second bits from right to left
  plane = ((RUindex >> 2) & 1); // third bit from right to left
  disc = ((RUindex >> 3) & 7);  // fourth to sixth bits from right to left
  half = ((RUindex >> 6) & 1);  // seventh bits from right to left
}

void QcMFTReadoutTask::generateRUindexMap()
// maps RUindex into an index from 0 to 79 to be used
// to access the vector of histograms
{
  // space to store geometrical info
  int zone = -1;
  int plane = -1;
  int disc = -1;
  int half = -1;

  // loop over potential IDs
  int idx = 0; // count good IDs
  for (int i = 0; i < maxNumberToIdentifyRU; i++) {
    mIndexOfRUMap[i] = -1; // -1 means no RU has such an ID
    unpackRUindex(i, zone, plane, disc, half);
    if (zone > 3)
      continue;
    if (plane > 1)
      continue;
    if (disc > 4)
      continue;
    if (half > 1)
      continue;
    mIndexOfRUMap[i] = idx;
    idx++;
  }
}

} // namespace o2::quality_control_modules::mft
