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
/// \file   RawQcTask.cxx
/// \author Bogdan Vulpescu
/// \author Xavier Lopez
/// \author Diego Stocco
/// \author Guillaume Taillepied
/// \author Valerie Ramillien

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

#include "QualityControl/QcInfoLogger.h"
#include "MID/RawQcTask.h"
#include <Framework/InputRecord.h>

#include "Framework/DataRefUtils.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROBoard.h"
#include "DataFormatsMID/ROFRecord.h"
#include "DPLUtils/DPLRawParser.h"
#include "MIDQC/RawDataChecker.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDRaw/CrateParameters.h"
#include "Headers/RAWDataHeader.h"

namespace o2::quality_control_modules::mid
{

RawQcTask::~RawQcTask()
{
  if (mRawDataChecker)
    delete mRawDataChecker;
}

void RawQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Devel) << "initialize RawQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  // printf(" =================== > test initialise RAW \n");

  // Retrieve task parameters from the config file
  if (auto param = mCustomParameters.find("feeId-config-file"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - FEE Id config file: " << param->second << ENDM;
    if (!param->second.empty()) {
      mFeeIdConfig = o2::mid::FEEIdConfig(param->second.c_str());
    }
  }

  if (auto param = mCustomParameters.find("crate-masks-file"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - Crate masks file: " << param->second << ENDM;
    if (!param->second.empty()) {
      mCrateMasks = o2::mid::CrateMasks(param->second.c_str());
    }
  }

  if (auto param = mCustomParameters.find("electronics-delays-file"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - Electronics delays file: " << param->second << ENDM;
    if (!param->second.empty()) {
      mElectronicsDelay = o2::mid::readElectronicsDelay(param->second.c_str());
    }
  }

  mChecker.init(mCrateMasks);

  // Histograms to be published
  mRawDataChecker = new TH1F("mRawDataChecker", "Raw Data Checker", 2, 0, 2);
  mRawDataChecker->GetXaxis()->SetBinLabel(1, "Processed");
  mRawDataChecker->GetXaxis()->SetBinLabel(2, "Faulty");
  getObjectsManager()->startPublishing(mRawDataChecker);

  mRawLocalBoardsMap = new TH2F("RawLocalBoardsMap", "Raw Local boards Occupancy Map", 16, 0, 16, 16, 0, 16);
  getObjectsManager()->startPublishing(mRawLocalBoardsMap);
  mRawLocalBoardsMap->GetXaxis()->SetTitle("CrateID");
  mRawLocalBoardsMap->GetYaxis()->SetTitle("LocID in Crate");
  mRawLocalBoardsMap->SetOption("colz");
  mRawLocalBoardsMap->SetStats(0);

  mBusyRawLocalBoards = new TH2F("BusyRawLocalBoards", "Busy Raw Local boards", 16, 0, 16, 16, 0, 16); // crateId X: locId Y
  getObjectsManager()->startPublishing(mBusyRawLocalBoards);
  mBusyRawLocalBoards->GetXaxis()->SetTitle("CrateID");
  mBusyRawLocalBoards->GetYaxis()->SetTitle("LocID in Crate");
  mBusyRawLocalBoards->SetOption("colz");
  mBusyRawLocalBoards->SetStats(0);

  mRawBCCounts = new TH1F("RawBCCounts", "Raw Bunch Crossing Counts", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches);
  // mRawBCCounts = new TProfile("RawBCCounts", "Mean Raw Bunch Crossing Counts", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches);
  getObjectsManager()->startPublishing(mRawBCCounts);
  mRawBCCounts->GetXaxis()->SetTitle("BC");
  mRawBCCounts->GetYaxis()->SetTitle("Entry (Ko)");
}

static int Pattern(uint16_t pattern)
{
  int nHits = 0;
  int mask = 32768; // 1000000000000000
  for (int j = 0; j < 16; j++) {
    if ((pattern & mask) != 0) {
      nHits += 1;
    }
    mask >>= 1;
  }
  return nHits;
}

static int BPPattern(const o2::mid::ROBoard& board, uint8_t station)
{
  if (board.patternsBP[station] != 0) {
    return Pattern(board.patternsBP[station]);
  } else
    return 0;
}

static int NBPPattern(const o2::mid::ROBoard& board, uint8_t station)
{
  if (board.patternsNBP[station] != 0) {
    return Pattern(board.patternsNBP[station]);
  } else
    return 0;
}

void PatternMultiplicity(const o2::mid::ROBoard& board, int multB[], int multNB[])
{
  for (int i = 0; i < 4; i++) {
    multB[i] += BPPattern(board, i);
    multNB[i] += NBPPattern(board, i);
  }
}

bool isBoardEmpty(const o2::mid::ROBoard& board)
{
  bool rep = 0;
  for (int i = 0; i < 4; i++) {
    if ((board.patternsBP[i] != 0) || (board.patternsNBP[i] != 0))
      rep = 0;
    else
      rep = 1;
  }
  return rep;
}

void RawQcTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Info, Devel) << "startOfActivity" << ENDM;
  // printf(" =================== > test startOfActivity RAW \n");
}

void RawQcTask::startOfCycle()
{
  // ILOG(Info, Devel) << "startOfCycle" << ENDM;
  // printf(" =================== > test startOfCycle RAW \n");
}

void RawQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  // ILOG(Info, Devel) << "startOfDataMonitoring" << ENDM;
  // printf(" =================== > test monitorData RAW \n");

  o2::framework::DPLRawParser parser(ctx.inputs());
  o2::InteractionRecord IntRecord;

  std::vector<o2::mid::ROFRecord> dummy;

  if (!mDecoder) {
    auto const* rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(parser.begin().raw());
    mDecoder = createDecoder(*rdhPtr, true, mElectronicsDelay, mCrateMasks, mFeeIdConfig);
    ILOG(Info, Devel) << "Created decoder" << ENDM;
  }

  mDecoder->clear();
  InitMultiplicity();

  int count = 0;
  int plcount = 0;
  int itparsercount = 0;
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(it.raw());
    itparsercount++;
    gsl::span<const uint8_t> payload(it.data(), it.size());
    if (!payload.empty()) {
      mDecoder->process(payload, *rdhPtr);
      ++count;
      plcount = 0;
      for (auto pl = payload.begin(), end = payload.end(); pl != end; ++pl) {
        plcount++;
      }
    }
  }

  nROF = 0;
  int nBoardTot = 0;

  for (auto& rof : mDecoder->getROFRecords()) { // boucle sur les ROFRecords //
    // printf("========================================================== \n");
    // printf("Raw :: %05d ROF with first entry %05zu and nentries %02zu , BC %05d, ORB %05d , EventType %02d\n", nROF, rof.firstEntry, rof.nEntries, rof.interactionRecord.bc, rof.interactionRecord.orbit,rof.eventType);
    // eventType::  Standard = 0, Calib = 1, FET = 2

    nROF++;
    nBoard = 0;
    nEntriesROF = 0;
    mRawBCCounts->Fill(rof.interactionRecord.bc, float(rof.nEntries) * 20. / 1000.); // board = 20 octets

    for (auto board = mDecoder->getData().begin() + rof.firstEntry; board != mDecoder->getData().begin() + rof.firstEntry + rof.nEntries; ++board) {

      nBoard++;
      nBoardTot++;
      int boardId = (*board).boardId;
      int crateId = o2::mid::raw::getCrateId(boardId);
      int locId = o2::mid::raw::getLocId(boardId);
      int linkId = o2::mid::crateparams::getGBTIdFromBoardInCrate(locId);
      int feeId = o2::mid::crateparams::makeGBTUniqueId(crateId, linkId);
      int statusWord = (*board).statusWord;
      int triggerWord = (*board).triggerWord;
      int isLoc = (statusWord >> 6) & 1;
      int busyLoc = (statusWord >> 5) & 1;
      int decisionLoc = (statusWord >> 4) & 1;

      int isPhys = (triggerWord >> 2) & 1;
      int isCalib = (triggerWord >> 3) & 1;
      int sOrb = triggerWord & 1;

      if (isLoc && busyLoc)
        mBusyRawLocalBoards->Fill(crateId, locId, 1);

      PatternMultiplicity(*board, multHitB, multHitNB);
    }
  }

  // mChecker.clear();
  // if (!mChecker.process(mDecoder->getData(), mDecoder->getROFRecords(), dummy)) {
  //  // ILOG(Info, Devel) << mChecker.getDebugMessage() << ENDM;
  //  mRawDataChecker->Fill("Faulty", mChecker.getNEventsFaulty());
  //}

  // ILOG(Info, Devel) << "Number of busy raised: " << mChecker.getNBusyRaised() << ENDM;
  // ILOG(Info, Devel) << "Fraction of faulty events: " << mChecker.getNEventsFaulty() << " / " << mChecker.getNEventsProcessed() << ENDM;
  // ILOG(Info, Devel) << "Counts: " << count << ENDM;

  // mRawDataChecker->Fill("Processed", mChecker.getNEventsProcessed());
}

void RawQcTask::endOfCycle()
{
  // ILOG(Info, Devel) << "endOfCycle" << ENDM;
  // printf(" =================== > test endOfCycle RAW \n");
}

void RawQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Info, Devel) << "endOfActivity" << ENDM;
  // printf(" =================== > test endOfActivity RAW \n");
}

void RawQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Devel) << "Resetting the histogram" << ENDM;
  // printf(" =================== > test reset RAW \n");
  mRawDataChecker->Reset();
  mRawBCCounts->Reset();
  mRawLocalBoardsMap->Reset();
  mBusyRawLocalBoards->Reset();
}

} // namespace o2::quality_control_modules::mid
