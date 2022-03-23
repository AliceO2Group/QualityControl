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
#include <ITSMFTReconstruction/ChipMappingMFT.h>

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

  // create the index to link a RU+lane to a chip
  //==============================================
  generateChipIndex();
  const o2::itsmft::ChipMappingMFT mapMFT; // MFT maps
  auto chipMapData = mapMFT.getChipMappingData();

  // Defining DDW summary histogram
  //==============================================
  mDDWSummary = std::make_unique<TH1F>("mDDWSummary", "DDW Summary", 3, 0.5, 3.5);
  getObjectsManager()->startPublishing(mDDWSummary.get());
  mDDWSummary->GetXaxis()->SetBinLabel(1, "Warning");
  mDDWSummary->GetXaxis()->SetBinLabel(2, "Error");
  mDDWSummary->GetXaxis()->SetBinLabel(3, "Fault");
  mDDWSummary->GetYaxis()->SetTitle("#Entries per DDW");
  mDDWSummary->SetStats(0);

  // Defining RDH summary histogram
  //==============================================
  mRDHSummary = std::make_unique<TH1F>("mRDHSummary", "RDH Summary", 5, -0.5, 4.5);
  getObjectsManager()->startPublishing(mRDHSummary.get());
  mRDHSummary->GetXaxis()->SetBinLabel(1, "Missing data");
  mRDHSummary->GetXaxis()->SetBinLabel(2, "Warning");
  mRDHSummary->GetXaxis()->SetBinLabel(3, "Error");
  mRDHSummary->GetXaxis()->SetBinLabel(4, "Fault");
  mRDHSummary->GetXaxis()->SetBinLabel(5, "RDH");
  mRDHSummary->GetYaxis()->SetTitle("#Entries per DDW");
  mRDHSummary->SetStats(0);

  // Defining chip summary histograms
  //==============================================
  int nChips = 936;
  // --> error
  mSummaryChipError = std::make_unique<TH1F>("mSummaryChipError", "Summary of chips in Error", nChips, -0.5, nChips - 0.5);
  getObjectsManager()->startPublishing(mSummaryChipError.get());
  mSummaryChipError->GetYaxis()->SetTitle("#Entries per DDW");
  mSummaryChipError->SetStats(0);
  // --> fault
  mSummaryChipFault = std::make_unique<TH1F>("mSummaryChipFault", "Summary of chips in Fault", nChips, -0.5, nChips - 0.5);
  getObjectsManager()->startPublishing(mSummaryChipFault.get());
  mSummaryChipFault->GetYaxis()->SetTitle("#Entries per DDW");
  mSummaryChipFault->SetStats(0);
  // --> warning
  mSummaryChipWarning = std::make_unique<TH1F>("mSummaryChipWarning", "Summary of chips in Warning", nChips, -0.5, nChips - 0.5);
  getObjectsManager()->startPublishing(mSummaryChipWarning.get());
  mSummaryChipWarning->GetYaxis()->SetTitle("#Entries per DDW");
  mSummaryChipWarning->SetStats(0);
  // --> ok
  mSummaryChipOk = std::make_unique<TH1F>("mSummaryChipOk", "Summary of chips in OK", nChips, -0.5, nChips - 0.5);
  getObjectsManager()->startPublishing(mSummaryChipOk.get());
  mSummaryChipOk->GetYaxis()->SetTitle("#Entries per DDW");
  mSummaryChipOk->SetStats(0);

  for (int i = 0; i < nChips; i++) {
    int face = (chipMapData[i].layer) % 2;
    mSummaryChipError->GetXaxis()
      ->SetBinLabel(i + 1, Form("Chip %i:h%d-d%d-f%d-z%d-tr%d", i, chipMapData[i].half,
                                chipMapData[i].disk, face, chipMapData[i].zone, chipMapData[i].cable));
    mSummaryChipFault->GetXaxis()
      ->SetBinLabel(i + 1, Form("Chip %i:h%d-d%d-f%d-z%d-tr%d", i, chipMapData[i].half,
                                chipMapData[i].disk, face, chipMapData[i].zone, chipMapData[i].cable));
    mSummaryChipWarning->GetXaxis()
      ->SetBinLabel(i + 1, Form("Chip %i:h%d-d%d-f%d-z%d-tr%d", i, chipMapData[i].half,
                                chipMapData[i].disk, face, chipMapData[i].zone, chipMapData[i].cable));
    mSummaryChipOk->GetXaxis()
      ->SetBinLabel(i + 1, Form("Chip %i:h%d-d%d-f%d-z%d-tr%d", i, chipMapData[i].half,
                                chipMapData[i].disk, face, chipMapData[i].zone, chipMapData[i].cable));
  }
}

void QcMFTReadoutTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  
  // reset histograms
  reset();
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
    mRDHSummary->Fill(4);
    // fill status if set
    if (summaryLaneStatus & (1 << 0))
      mRDHSummary->Fill(0); // missing data
    if (summaryLaneStatus & (1 << 1))
      mRDHSummary->Fill(1); // warning
    if (summaryLaneStatus & (1 << 2))
      mRDHSummary->Fill(2); // error
    if (summaryLaneStatus & (1 << 3))
      mRDHSummary->Fill(3); // fault
    // check if last rdh in HBF and get the DDW word
    if (rdh->stop && it.size()) {
      auto const* ddw = reinterpret_cast<const MFTDDW*>(it.data());
      uint16_t ddwIndex = ddw->indexWord.indexBits.id;
      if (ddwIndex == 0xE4) { // it is a diagnostic data word
        // fill histogram bin with #DDW
        mDDWSummary->Fill(-1);         // counter stored in the underflow bin!
        mSummaryChipOk->Fill(-1);      // counter stored in the underflow bin!
        mSummaryChipWarning->Fill(-1); // counter stored in the underflow bin!
        mSummaryChipError->Fill(-1);   // counter stored in the underflow bin!
        mSummaryChipFault->Fill(-1);   // counter stored in the underflow bin!
        mRDHSummary->Fill(-1);         // counter stored in the underflow bin!
        uint64_t ddwLaneStatus = ddw->laneWord.laneBits.laneStatus;
        uint16_t rdhFeeIndex = rdh->feeId;
        int RUindex = (rdhFeeIndex & 127); // look only at the rightmost 7 bits
        // check the status of each lane
        for (int i = 0; i < nLanes; i++) {
          int idx = RUindex * nLanes + i;
          // check if it is a valide lane
          if (mChipIndex[idx] == -1)
            continue;
          // get the two bits corresponding to the lane i
          int MFTlaneStatus = ((ddwLaneStatus >> (i * 2)) & (3));
          // fill the info
          if (MFTlaneStatus == 0) {
            mSummaryChipOk->Fill(mChipIndex[idx]);
          }
          if (MFTlaneStatus == 1) {
            mSummaryChipWarning->Fill(mChipIndex[idx]);
            mDDWSummary->Fill(1);
          }
          if (MFTlaneStatus == 2) {
            mSummaryChipError->Fill(mChipIndex[idx]);
            mDDWSummary->Fill(2);
          }
          if (MFTlaneStatus == 3) {
            mSummaryChipFault->Fill(mChipIndex[idx]);
            mDDWSummary->Fill(3);
          }
        } // end loop over lanes
      }   // end if is a DDW
    }     // end if rdh->stop
  }       // end loop over input
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
  mDDWSummary->Reset();
  mSummaryChipError->Reset();
  mSummaryChipFault->Reset();
  mSummaryChipWarning->Reset();
  mSummaryChipOk->Reset();
  mRDHSummary->Reset();
}

void QcMFTReadoutTask::generateChipIndex()
// generate index to relate a RU+lane to a chip
{
  // initialise
  for (int i = 0; i < maxRUidx; i++) {
    for (int j = 0; j < nLanes; j++) {
      int idx = i * nLanes + j;
      mChipIndex[idx] = -1;
    }
  }

  // fill
  const o2::itsmft::ChipMappingMFT mapMFT; // MFT maps
  auto chipMapData = mapMFT.getChipMappingData();
  for (int i = 0; i < 936; i++) {
    int j = chipMapData[i].ruHWID;
    int k = chipMapData[i].cable;
    int idx = j * nLanes + k;
    mChipIndex[idx] = chipMapData[i].globalChipSWID;
  }
}

} // namespace o2::quality_control_modules::mft
