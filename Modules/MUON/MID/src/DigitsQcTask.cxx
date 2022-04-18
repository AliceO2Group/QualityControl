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
/// \file   DigitsQcTask.cxx
/// \author Diego Stocco
/// \author Andrea Ferrero
/// \author Valerie Ramillien

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "QualityControl/QcInfoLogger.h"
#include "MID/DigitsQcTask.h"
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
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"

#define MID_NDE 72
#define MID_NCOL 7

using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::mid
{

DigitsQcTask::~DigitsQcTask()
{
}

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize DigitsQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mHitsMapB = std::make_shared<TH2F>("HitsMapB", "Hits Map - bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  getObjectsManager()->startPublishing(mHitsMapB.get());
  mHitsMapB->SetOption("colz");
  mHitsMapB->SetStats(0);

  mHitsMapNB = std::make_shared<TH2F>("HitsMapNB", "Hits Map - non-bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  getObjectsManager()->startPublishing(mHitsMapNB.get());
  mHitsMapNB->SetOption("colz");
  mHitsMapNB->SetStats(0);

  mOrbitsMapB = std::make_shared<TH2F>("OrbitsMapB", "Orbits Map - bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  getObjectsManager()->startPublishing(mOrbitsMapB.get());
  mOrbitsMapB->SetOption("colz");
  mOrbitsMapB->SetStats(0);
  mOrbitsMapNB = std::make_shared<TH2F>("OrbitsMapNB", "Orbits Map - bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  getObjectsManager()->startPublishing(mOrbitsMapNB.get());
  mOrbitsMapNB->SetOption("colz");
  mOrbitsMapNB->SetStats(0);

  mROFSizeB = std::make_shared<TH1F>("ROFSizeB", "ROF size distribution - bending plane", 300, 0, 300);
  getObjectsManager()->startPublishing(mROFSizeB.get());
  mROFSizeNB = std::make_shared<TH1F>("ROFSizeNB", "ROF size distribution - non-bending plane", 300, 0, 300);
  getObjectsManager()->startPublishing(mROFSizeNB.get());

  mROFTimeDiff = std::make_shared<TH2F>("ROFTimeDiff", "ROF time difference vs. min. ROF size", 100, 0, 100, 100, 0, 100);
  mROFTimeDiff->SetOption("colz");
  getObjectsManager()->startPublishing(mROFTimeDiff.get());

  /////////////////

  mMultHitMT11B = std::make_shared<TH1F>("MultHitMT11B", "Multiplicity Hits - MT11 bending plane", 300, 0, 300);
  mMultHitMT11NB = std::make_shared<TH1F>("MultHitMT11NB", "Multiplicity Hits - MT11 non-bending plane", 300, 0, 300);
  mMultHitMT12B = std::make_shared<TH1F>("MultHitMT12B", "Multiplicity Hits - MT12 bending plane", 300, 0, 300);
  mMultHitMT12NB = std::make_shared<TH1F>("MultHitMT12NB", "Multiplicity Hits - MT12 non-bending plane", 300, 0, 300);
  mMultHitMT21B = std::make_shared<TH1F>("MultHitMT21B", "Multiplicity Hits - MT21 bending plane", 300, 0, 300);
  mMultHitMT21NB = std::make_shared<TH1F>("MultHitMT21NB", "Multiplicity Hits - MT21 non-bending plane", 300, 0, 300);
  mMultHitMT22B = std::make_shared<TH1F>("MultHitMT22B", "Multiplicity Hits - MT22 bending plane", 300, 0, 300);
  mMultHitMT22NB = std::make_shared<TH1F>("MultHitMT22NB", "Multiplicity Hits - MT22 non-bending plane", 300, 0, 300);
  getObjectsManager()->startPublishing(mMultHitMT11B.get());
  getObjectsManager()->startPublishing(mMultHitMT11NB.get());
  getObjectsManager()->startPublishing(mMultHitMT12B.get());
  getObjectsManager()->startPublishing(mMultHitMT12NB.get());
  getObjectsManager()->startPublishing(mMultHitMT21B.get());
  getObjectsManager()->startPublishing(mMultHitMT21NB.get());
  getObjectsManager()->startPublishing(mMultHitMT22B.get());
  getObjectsManager()->startPublishing(mMultHitMT22NB.get());

  mLocalBoardsMap = std::make_shared<TH2F>("LocalBoardsMap", "Local boards Occupancy Map", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mLocalBoardsMap.get());
  mLocalBoardsMap->SetOption("colz");
  mLocalBoardsMap->SetStats(0);

  mLocalBoardsMap11 = std::make_shared<TH2F>("LocalBoardsMap11", "Local boards Occupancy Map MT11", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mLocalBoardsMap11.get());
  mLocalBoardsMap11->SetOption("colz");
  mLocalBoardsMap11->SetStats(0);

  mLocalBoardsMap12 = std::make_shared<TH2F>("LocalBoardsMap12", "Local boards Occupancy Map MT12", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mLocalBoardsMap12.get());
  mLocalBoardsMap12->SetOption("colz");
  mLocalBoardsMap12->SetStats(0);

  mLocalBoardsMap21 = std::make_shared<TH2F>("LocalBoardsMap21", "Local boards Occupancy Map MT21", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mLocalBoardsMap21.get());
  mLocalBoardsMap21->SetOption("colz");
  mLocalBoardsMap21->SetStats(0);

  mLocalBoardsMap22 = std::make_shared<TH2F>("LocalBoardsMap22", "Local boards Occupancy Map MT22", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mLocalBoardsMap22.get());
  mLocalBoardsMap22->SetOption("colz");
  mLocalBoardsMap22->SetStats(0);

  mBendHitsMap11 = std::make_shared<TH2F>("BendHitsMap11", "Bending Hits Map MT11", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendHitsMap11.get());
  mBendHitsMap11->SetOption("colz");
  mBendHitsMap11->SetStats(0);
  mBendHitsMap12 = std::make_shared<TH2F>("BendHitsMap12", "Bending Hits Map MT12", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendHitsMap12.get());
  mBendHitsMap12->SetOption("colz");
  mBendHitsMap12->SetStats(0);
  mBendHitsMap21 = std::make_shared<TH2F>("BendHitsMap21", "Bending Hits Map MT21", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendHitsMap21.get());
  mBendHitsMap21->SetOption("colz");
  mBendHitsMap21->SetStats(0);
  mBendHitsMap22 = std::make_shared<TH2F>("BendHitsMap22", "Bending Hits Map MT22", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendHitsMap22.get());
  mBendHitsMap22->SetOption("colz");
  mBendHitsMap22->SetStats(0);

  mNBendHitsMap11 = std::make_shared<TH2F>("NBendHitsMap11", "Non-Bending Hits Map MT11", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendHitsMap11.get());
  mNBendHitsMap11->SetOption("colz");
  mNBendHitsMap11->SetStats(0);

  mNBendHitsMap12 = std::make_shared<TH2F>("NBendHitsMap12", "Non-Bending Hits Map MT12", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendHitsMap12.get());
  mNBendHitsMap12->SetOption("colz");
  mNBendHitsMap12->SetStats(0);

  mNBendHitsMap21 = std::make_shared<TH2F>("NBendHitsMap21", "Non-Bending Hits Map MT21", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendHitsMap21.get());
  mNBendHitsMap21->SetOption("colz");
  mNBendHitsMap21->SetStats(0);

  mNBendHitsMap22 = std::make_shared<TH2F>("NBendHitsMap22", "Non-Bending Hits Map MT22", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendHitsMap22.get());
  mNBendHitsMap22->SetOption("colz");
  mNBendHitsMap22->SetStats(0);
}

void DigitsQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void DigitsQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

static int countColumnDataHits(const o2::mid::ColumnData& digit, int id)
{
  int nHits = 0;
  int mask = 32768; // 1000000000000000
  if (digit.patterns[id] != 0) {
    for (int j = 0; j < 16; j++) {
      if ((digit.patterns[id] & mask) != 0) {
        nHits += 1;
      }
      mask >>= 1;
    }
  }
  return nHits;
}

static int getBendingHits(const o2::mid::ColumnData& digit)
{
  int nHits = 0;
  for (int i = 0; i < 4; i++) {
    nHits += countColumnDataHits(digit, i);
  }
  return nHits;
}

static int getNonBendingHits(const o2::mid::ColumnData& digit)
{
  return countColumnDataHits(digit, 4);
}

bool isDigitEmpty(const o2::mid::ColumnData& digit)
{
  bool rep = 0;
  for (int i = 0; i < 5; i++) {
    if ((digit.patterns[i] != 0))
      rep = 0;
    else
      rep = 1;
  }
  return rep;
}

static std::pair<uint32_t, uint32_t> getROFSize(const o2::mid::ROFRecord& rof, gsl::span<const o2::mid::ColumnData> digits)
{
  uint32_t nHitsB{ 0 };
  uint32_t nHitsNB{ 0 };

  auto lastEntry = rof.getEndIndex();
  for (auto i = rof.firstEntry; i < lastEntry; i++) {
    const auto& digit = digits[i];
    nHitsB += getBendingHits(digit);
    nHitsNB += getNonBendingHits(digit);
  }

  return std::make_pair(nHitsB, nHitsNB);
}

void DigitsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto digits = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("digits");
  auto rofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("digitrofs");

  int multHitMT11B = 0;
  int multHitMT12B = 0;
  int multHitMT21B = 0;
  int multHitMT22B = 0;
  int multHitMT11NB = 0;
  int multHitMT12NB = 0;
  int multHitMT21NB = 0;
  int multHitMT22NB = 0;

  for (int i = 0; i < MID_NDE; i++) {
    for (int j = 0; j < MID_NCOL; j++) {
      mOrbitsMapB->Fill(i, j, 128);
      mOrbitsMapNB->Fill(i, j, 128);
    }
  }

  for (const auto& digit : digits) {
    // total number of bending plane hits
    int nHitsB = getBendingHits(digit);
    mHitsMapB->Fill(digit.deId, digit.columnId, nHitsB);
    // total number of non-bending plane hits
    int nHitsNB = getNonBendingHits(digit);
    mHitsMapNB->Fill(digit.deId, digit.columnId, nHitsNB);
  }

  std::pair<uint32_t, uint32_t> prevSize;
  o2::InteractionRecord prevIr;
  for (size_t i = 0; i < rofs.size(); i++) { // rofs.size() = Number of Events
    const auto& rof = rofs[i];
    auto rofSize = getROFSize(rof, digits);
    mROFSizeB->Fill(rofSize.first);
    mROFSizeNB->Fill(rofSize.second);

    if (i > 0) {
      uint32_t sizeTot = rofSize.first + rofSize.second;
      uint32_t prevSizeTot = prevSize.first + prevSize.second;
      uint32_t sizeMin = (sizeTot < prevSizeTot) ? sizeTot : prevSizeTot;

      auto timeDiff = rof.interactionRecord.differenceInBC(prevIr);

      mROFTimeDiff->Fill(timeDiff, sizeMin);
    }

    prevSize = rofSize;
    prevIr = rof.interactionRecord;
  }

  for (const auto& rofRecord : rofs) { // loop ROFRecords //
    // printf("========================================================== \n");
    // printf("%05d ROF with first entry %05zu and nentries %02zu , BC %05d, ORB %05d , EventType %02d\n", nROF, rofRecord.firstEntry, rofRecord.nEntries, rofRecord.interactionRecord.bc, rofRecord.interactionRecord.orbit,rofRecord.eventType);
    //  eventType::  Standard = 0, Calib = 1, FET = 2
    nROF++;
    multHitMT11B = 0;
    multHitMT12B = 0;
    multHitMT21B = 0;
    multHitMT22B = 0;
    multHitMT11NB = 0;
    multHitMT12NB = 0;
    multHitMT21NB = 0;
    multHitMT22NB = 0;

    // loadStripPatterns (ColumnData)
    for (auto& digit : digits.subspan(rofRecord.firstEntry, rofRecord.nEntries)) { // loop DE //
      int deIndex = digit.deId;
      int colId = digit.columnId;
      int rpcLine = o2::mid::detparams::getRPCLine(deIndex);
      int ichamber = o2::mid::detparams::getChamber(deIndex);
      auto isRightSide = o2::mid::detparams::isRightSide(deIndex);
      auto detId = o2::mid::detparams::getDEId(isRightSide, ichamber, colId);

      /* if(isDigitEmpty(digit) == 0){
  std::cout << "  =>>  " << digit << std::endl;
  for (int i = 0; i < 4; i++) {
    if(digit.patterns[i]!=0){
      std::cout <<"=== i = "<<i<< " :  BoardId  = " << mMapping.getBoardId(i,colId,deIndex,true) << std::endl;
    }
  }
      }*/

      int nZoneHistoX = 1;
      int nZoneHistoY = 4;
      double stripYSize = 1. / 16;
      double stripXSize = 0.25 / 16;
      if (mMapping.getLastBoardBP(colId, deIndex) == 1) {
        nZoneHistoX = 2;
        stripXSize = 0.5 / 16;
      } else if (mMapping.getLastBoardBP(colId, deIndex) == 0) {
        nZoneHistoX = 4;
        stripXSize = 1. / 16;
      }

      // std::cout << "colId  =>>  " << colId << "  deIndex = "<< deIndex <<" getNStripsNBP(deIndex)  = "<< mMapping.getNStripsNBP(colId,deIndex) << "  detId = "<< detId <<" getNStripsNBP(detId)  = "<< mMapping.getNStripsNBP(colId,detId) << std::endl;
      for (int board = mMapping.getFirstBoardBP(colId, deIndex), lastBoard = mMapping.getLastBoardBP(colId, deIndex); board <= lastBoard + 1; board++) {
        // These are the existing bend boards for this column Id (board = n°board in the column) + 1 (for non-bend)
        if ((lastBoard < 4) && (board == lastBoard + 1))
          board = 4;                      // for Non-Bend digits
        if (digit.patterns[board] != 0) { //  Bend + Non-Bend plane
          //// Bend Local Boards Display ::
          double linePos0 = rpcLine;
          if (board < 4) {
            linePos0 = rpcLine + 0.25 * board;
            if ((nZoneHistoX == 2) && (board == 1))
              linePos0 += 0.25;
            for (int ib = 0; ib < nZoneHistoX; ib++) {
              double linePos = linePos0 + (0.25 * ib);
              if (isRightSide) {
                mLocalBoardsMap->Fill(colId + 0.5, linePos, 1);
                if (ichamber == 0)
                  mLocalBoardsMap11->Fill(colId + 0.5, linePos, 1);
                else if (ichamber == 1)
                  mLocalBoardsMap12->Fill(colId + 0.5, linePos, 1);
                else if (ichamber == 2)
                  mLocalBoardsMap21->Fill(colId + 0.5, linePos, 1);
                else if (ichamber == 3)
                  mLocalBoardsMap22->Fill(colId + 0.5, linePos, 1);
              } else {
                mLocalBoardsMap->Fill(-colId - 0.5, linePos, 1);
                if (ichamber == 0)
                  mLocalBoardsMap11->Fill(-colId - 0.5, linePos, 1);
                else if (ichamber == 1)
                  mLocalBoardsMap12->Fill(-colId - 0.5, linePos, 1);
                else if (ichamber == 2)
                  mLocalBoardsMap21->Fill(-colId - 0.5, linePos, 1);
                else if (ichamber == 3)
                  mLocalBoardsMap22->Fill(-colId - 0.5, linePos, 1);
              }
            }    // board in line loop
          }      // if board<4 :: Bend
          else { // nen-Bend
            double shift = 0;
            if ((colId == 0) && ((rpcLine == 3) || (rpcLine == 5)))
              nZoneHistoY = 3;
            if ((colId == 0) && (rpcLine == 5))
              shift = 0.25;
            for (int ib = 0; ib < nZoneHistoY; ib++) {
              if (isRightSide) {
                mLocalBoardsMap->Fill(colId + 0.5, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                if (ichamber == 0)
                  mLocalBoardsMap11->Fill(colId + 0.5, rpcLine + shift + (0.25 * ib), 1);
                else if (ichamber == 1)
                  mLocalBoardsMap12->Fill(colId + 0.5, rpcLine + shift + (0.25 * ib), 1);
                else if (ichamber == 2)
                  mLocalBoardsMap21->Fill(colId + 0.5, rpcLine + shift + (0.25 * ib), 1);
                else if (ichamber == 3)
                  mLocalBoardsMap22->Fill(colId + 0.5, rpcLine + shift + (0.25 * ib), 1);
              } else {
                mLocalBoardsMap->Fill(-colId - 0.5, rpcLine + shift + (0.25 * ib), 1);
                if (ichamber == 0)
                  mLocalBoardsMap11->Fill(-colId - 0.5, rpcLine + shift + (0.25 * ib), 1);
                else if (ichamber == 1)
                  mLocalBoardsMap12->Fill(-colId - 0.5, rpcLine + shift + (0.25 * ib), 1);
                else if (ichamber == 2)
                  mLocalBoardsMap21->Fill(-colId - 0.5, rpcLine + shift + (0.25 * ib), 1);
                else if (ichamber == 3)
                  mLocalBoardsMap22->Fill(-colId - 0.5, rpcLine + shift + (0.25 * ib), 1);
              }
            }
          }
          //// Strips Display :: // pb zone with 3 local boards
          int mask = 1;

          double shift = 0;
          if ((colId == 0) && ((rpcLine == 3) || (rpcLine == 5)))
            nZoneHistoY = 3;
          if ((colId == 0) && (rpcLine == 5))
            shift = 0.25; // for central zone with only 3 loc boards

          for (int j = 0; j < 16; j++) {
            if ((digit.patterns[board] & mask) != 0) {
              double lineHitPos = linePos0 + (j * stripXSize);
              int colj = j;
              if (mMapping.getNStripsNBP(colId, deIndex) == 8)
                colj = j * 2; // Bend with only 8 stripY
              double colHitPos = colId + (colj * stripYSize);
              // std::cout << " bit (j) =>>  " << j << "  stripXPos = "<< stripXPos <<"  lineHitPos = "<< lineHitPos << std::endl;
              if (isRightSide) {
                if (ichamber == 0) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap11->Fill(colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap11->Fill(colId + 0.5, lineHitPos, 1); // Bend
                } else if (ichamber == 1) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap12->Fill(colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap12->Fill(colId + 0.5, lineHitPos, 1); // Bend
                } else if (ichamber == 2) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap21->Fill(colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap21->Fill(colId + 0.5, lineHitPos, 1); // Bend
                } else if (ichamber == 3) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap22->Fill(colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap22->Fill(colId + 0.5, lineHitPos, 1); // Bend
                }
              } else {
                if (ichamber == 0) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap11->Fill(-colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap11->Fill(-colId - 0.5, lineHitPos, 1); // Bend
                } else if (ichamber == 1) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap12->Fill(-colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap12->Fill(-colId - 0.5, lineHitPos, 1); // Bend
                } else if (ichamber == 2) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap21->Fill(-colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap21->Fill(-colId - 0.5, lineHitPos, 1); // Bend
                } else if (ichamber == 3) {
                  if (board == 4) {
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendHitsMap22->Fill(-colHitPos, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else
                    mBendHitsMap22->Fill(-colId - 0.5, lineHitPos, 1); // Bend
                }
              }
            }
            mask <<= 1;
          }
        } // digit.patterns[board] Bend or Non-Bend not empty
      }

      //  Hits Multiplicity ::
      if (ichamber == 0) {
        multHitMT11B += getBendingHits(digit);
        multHitMT11NB += getNonBendingHits(digit);
      } else if (ichamber == 1) {
        multHitMT12B += getBendingHits(digit);
        multHitMT12NB += getNonBendingHits(digit);
      } else if (ichamber == 2) {
        multHitMT21B += getBendingHits(digit);
        multHitMT21NB += getNonBendingHits(digit);
      } else if (ichamber == 3) {
        multHitMT22B += getBendingHits(digit);
        multHitMT22NB += getNonBendingHits(digit);
      }

    } // Histograms Hits Multiplicity ::
    mMultHitMT11B->Fill(multHitMT11B);
    mMultHitMT12B->Fill(multHitMT12B);
    mMultHitMT21B->Fill(multHitMT21B);
    mMultHitMT22B->Fill(multHitMT22B);
    mMultHitMT11NB->Fill(multHitMT11NB);
    mMultHitMT12NB->Fill(multHitMT12NB);
    mMultHitMT21NB->Fill(multHitMT21NB);
    mMultHitMT22NB->Fill(multHitMT22NB);
  } //  ROFRecords //
}

void DigitsQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void DigitsQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void DigitsQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mHitsMapB->Reset();
  mHitsMapNB->Reset();
  mOrbitsMapB->Reset();
  mOrbitsMapNB->Reset();
  mROFSizeB->Reset();
  mROFSizeNB->Reset();

  mROFTimeDiff->Reset();

  mMultHitMT11B->Reset();
  mMultHitMT11NB->Reset();
  mMultHitMT12B->Reset();
  mMultHitMT12NB->Reset();
  mMultHitMT21B->Reset();
  mMultHitMT21NB->Reset();
  mMultHitMT22B->Reset();
  mMultHitMT22NB->Reset();

  mLocalBoardsMap->Reset();
}

} // namespace o2::quality_control_modules::mid
