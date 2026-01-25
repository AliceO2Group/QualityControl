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
#include "MFT/QcMFTUtilTables.h"

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
  ILOG(Debug, Devel) << "initialize QcMFTReadoutTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // create the index to link a RU+lane to a chip
  //==============================================
  generateChipIndex();
  const o2::itsmft::ChipMappingMFT mapMFT; // MFT maps
  auto chipMapData = mapMFT.getChipMappingData();

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

  // Defining summary histograms per zone
  mZoneSummaryChipWarning = std::make_unique<TH2F>(
    "mZoneSummaryChipWarning",
    "Summary of chips in warning per zone;;",
    10, -0.5, 9.5, 8, -0.5, 7.5);
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(1, "d0-f0");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(2, "d0-f1");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(3, "d1-f0");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(4, "d1-f1");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(5, "d2-f0");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(6, "d2-f1");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(7, "d3-f0");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(8, "d3-f1");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(9, "d4-f0");
  mZoneSummaryChipWarning->GetXaxis()->SetBinLabel(10, "d4-f1");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(1, "h0-z0");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(2, "h0-z1");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(3, "h0-z2");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(4, "h0-z3");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(5, "h1-z0");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(6, "h1-z1");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(7, "h1-z2");
  mZoneSummaryChipWarning->GetYaxis()->SetBinLabel(8, "h1-z3");
  mZoneSummaryChipWarning->SetStats(0);
  getObjectsManager()->startPublishing(mZoneSummaryChipWarning.get());
  getObjectsManager()->setDefaultDrawOptions(mZoneSummaryChipWarning.get(), "colz");

  mZoneSummaryChipError = std::make_unique<TH2F>(
    "mZoneSummaryChipError",
    "Summary of chips in error per zone;;",
    10, -0.5, 9.5, 8, -0.5, 7.5);
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(1, "d0-f0");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(2, "d0-f1");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(3, "d1-f0");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(4, "d1-f1");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(5, "d2-f0");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(6, "d2-f1");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(7, "d3-f0");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(8, "d3-f1");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(9, "d4-f0");
  mZoneSummaryChipError->GetXaxis()->SetBinLabel(10, "d4-f1");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(1, "h0-z0");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(2, "h0-z1");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(3, "h0-z2");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(4, "h0-z3");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(5, "h1-z0");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(6, "h1-z1");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(7, "h1-z2");
  mZoneSummaryChipError->GetYaxis()->SetBinLabel(8, "h1-z3");
  mZoneSummaryChipError->SetStats(0);
  getObjectsManager()->startPublishing(mZoneSummaryChipError.get());
  getObjectsManager()->setDefaultDrawOptions(mZoneSummaryChipError.get(), "colz");

  mZoneSummaryChipFault = std::make_unique<TH2F>(
    "mZoneSummaryChipFault",
    "Summary of chips in fault per zone;;",
    10, -0.5, 9.5, 8, -0.5, 7.5);
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(1, "d0-f0");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(2, "d0-f1");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(3, "d1-f0");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(4, "d1-f1");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(5, "d2-f0");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(6, "d2-f1");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(7, "d3-f0");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(8, "d3-f1");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(9, "d4-f0");
  mZoneSummaryChipFault->GetXaxis()->SetBinLabel(10, "d4-f1");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(1, "h0-z0");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(2, "h0-z1");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(3, "h0-z2");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(4, "h0-z3");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(5, "h1-z0");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(6, "h1-z1");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(7, "h1-z2");
  mZoneSummaryChipFault->GetYaxis()->SetBinLabel(8, "h1-z3");
  mZoneSummaryChipFault->SetStats(0);
  getObjectsManager()->startPublishing(mZoneSummaryChipFault.get());
  getObjectsManager()->setDefaultDrawOptions(mZoneSummaryChipFault.get(), "colz");

  // get map data for summary histograms per zone
  getChipMapData();
}

void QcMFTReadoutTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  // reset histograms
  reset();
}

void QcMFTReadoutTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void QcMFTReadoutTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input
  DPLRawParser parser(ctx.inputs());

  // loop over input
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // get the header
    auto rdh = reinterpret_cast<const o2::header::RDHAny*>(it.raw());
    auto feeID = o2::raw::RDHUtils::getFEEID(rdh);
    // get detector field
    uint32_t summaryLaneStatus = o2::raw::RDHUtils::getDetectorField(rdh);
    // check if last rdh in HBF and get the DDW word
    if ((int)(o2::raw::RDHUtils::getStop(rdh)) && it.size()) {
      auto const* ddw = reinterpret_cast<const MFTDDW*>(it.data());
      uint16_t ddwIndex = ddw->indexWord.indexBits.id;
      if (ddwIndex == 0xE4) { // it is a diagnostic data word
        // fill histogram bin with #DDW
        mSummaryChipOk->Fill(-1);              // counter stored in the underflow bin!
        mSummaryChipWarning->Fill(-1);         // counter stored in the underflow bin!
        mSummaryChipError->Fill(-1);           // counter stored in the underflow bin!
        mSummaryChipFault->Fill(-1);           // counter stored in the underflow bin!
        mZoneSummaryChipWarning->Fill(-1, -1); // counter stored in the underflow bin!
        mZoneSummaryChipError->Fill(-1, -1);   // counter stored in the underflow bin!
        mZoneSummaryChipFault->Fill(-1, -1);   // counter stored in the underflow bin!
        uint64_t ddwLaneStatus = ddw->laneWord.laneBits.laneStatus;
        uint16_t rdhFeeIndex = feeID;
        int RUindex = (rdhFeeIndex & 127); // look only at the rightmost 7 bits
        // check the status of each lane
        for (int i = 0; i < nLanes; i++) {
          int idx = RUindex * nLanes + i;
          // check if it is a valide lane
          if (mChipIndex[idx] == -1) {
            continue;
          }
          // get the two bits corresponding to the lane i
          int MFTlaneStatus = ((ddwLaneStatus >> (i * 2)) & (3));
          // get zone for summary histos
          int xBin = mDisk[mChipIndex[idx]] * 2 + mFace[mChipIndex[idx]];
          int yBin = mZone[mChipIndex[idx]] + mHalf[mChipIndex[idx]] * 4;
          // fill the info
          if (MFTlaneStatus == 0) {
            mSummaryChipOk->Fill(mChipIndex[idx]);
          }
          if (MFTlaneStatus == 1) {
            mSummaryChipWarning->Fill(mChipIndex[idx]);
            mZoneSummaryChipWarning->Fill(xBin, yBin);
          }
          if (MFTlaneStatus == 2) {
            mSummaryChipError->Fill(mChipIndex[idx]);
            mZoneSummaryChipError->Fill(xBin, yBin);
          }
          if (MFTlaneStatus == 3) {
            mSummaryChipFault->Fill(mChipIndex[idx]);
            mZoneSummaryChipFault->Fill(xBin, yBin);
          }
        } // end loop over lanes
      } // end if is a DDW
    } // end if rdh->stop
  } // end loop over input
}

void QcMFTReadoutTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void QcMFTReadoutTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void QcMFTReadoutTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mSummaryChipError->Reset();
  mSummaryChipFault->Reset();
  mSummaryChipWarning->Reset();
  mSummaryChipOk->Reset();
  mZoneSummaryChipWarning->Reset();
  mZoneSummaryChipError->Reset();
  mZoneSummaryChipFault->Reset();
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

void QcMFTReadoutTask::getChipMapData()
{
  const o2::itsmft::ChipMappingMFT mapMFT;
  auto chipMapData = mapMFT.getChipMappingData();
  QcMFTUtilTables MFTTable;

  for (int i = 0; i < 936; i++) {
    mHalf[i] = chipMapData[i].half;
    mDisk[i] = chipMapData[i].disk;
    mLayer[i] = chipMapData[i].layer;
    mFace[i] = mLayer[i] % 2;
    mZone[i] = chipMapData[i].zone;
    mSensor[i] = chipMapData[i].localChipSWID;
    mTransID[i] = chipMapData[i].cable;
    mLadder[i] = MFTTable.mLadder[i];
    mX[i] = MFTTable.mX[i];
    mY[i] = MFTTable.mY[i];
  }
}

} // namespace o2::quality_control_modules::mft
