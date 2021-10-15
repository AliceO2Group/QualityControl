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
///

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

#define MID_NDE 72
#define MID_NCOL 7

namespace o2::quality_control_modules::mid
{

DigitsQcTask::~DigitsQcTask()
{
}

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize DigitsQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // Histograms to be published
  mHitsMapB = new TH2F("HitsMapB", "Hits Map - bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  mHitsMapB->GetXaxis()->SetTitle("DE ID");
  mHitsMapB->GetYaxis()->SetTitle("Column ID");
  mHitsMapB->SetOption("colz");

  mHitsMapNB = new TH2F("HitsMapNB", "Hits Map - non-bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  mHitsMapNB->GetXaxis()->SetTitle("DE ID");
  mHitsMapNB->GetYaxis()->SetTitle("Column ID");
  mHitsMapNB->SetOption("colz");

  getObjectsManager()->startPublishing(mHitsMapB);
  getObjectsManager()->startPublishing(mHitsMapNB);

  mOrbitsMapB = new TH2F("OrbitsMapB", "Orbits Map - bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  mOrbitsMapB->GetXaxis()->SetTitle("DE ID");
  mOrbitsMapB->GetYaxis()->SetTitle("Column ID");
  mOrbitsMapB->SetOption("colz");

  mOrbitsMapNB = new TH2F("OrbitsMapNB", "Orbits Map - non-bending plane", MID_NDE, 0, MID_NDE, MID_NCOL, 0, MID_NCOL);
  mOrbitsMapNB->GetXaxis()->SetTitle("DE ID");
  mOrbitsMapNB->GetYaxis()->SetTitle("Column ID");
  mOrbitsMapNB->SetOption("colz");

  getObjectsManager()->startPublishing(mOrbitsMapB);
  getObjectsManager()->startPublishing(mOrbitsMapNB);

  mOccupancyMapB = new MergeableTH2Ratio("OccupancyMapB", "Occupancy - bending (MHz)", mHitsMapB, mOrbitsMapB);
  mOccupancyMapB->SetOption("colz");

  mOccupancyMapNB = new MergeableTH2Ratio("OccupancyMapNB", "Occupancy - non-bending (MHz)", mHitsMapNB, mOrbitsMapNB);
  mOccupancyMapNB->SetOption("colz");

  getObjectsManager()->startPublishing(mOccupancyMapB);
  getObjectsManager()->startPublishing(mOccupancyMapNB);

  mROFSizeB = new TH1F("ROFSizeB", "ROF size distribution - bending plane", 100, 0, 100);
  mROFSizeNB = new TH1F("ROFSizeNB", "ROF size distribution - non-bending plane", 100, 0, 100);

  getObjectsManager()->startPublishing(mROFSizeB);
  getObjectsManager()->startPublishing(mROFSizeNB);

  mROFTimeDiff = new TH2F("ROFTimeDiff", "ROF time difference vs. min. ROF size", 100, 0, 100, 100, 0, 100);
  mROFTimeDiff->SetOption("colz");

  getObjectsManager()->startPublishing(mROFTimeDiff);
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
  int mask = 1;
  for (int j = 0; j < 16; j++) {
    if ((digit.patterns[id] & mask) != 0) {
      nHits += 1;
    }
    mask <<= 1;
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
  for (size_t i = 0; i < rofs.size(); i++) {
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
}

void DigitsQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;

  mOccupancyMapB->update();
  mOccupancyMapNB->update();
}

void DigitsQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void DigitsQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
}

} // namespace o2::quality_control_modules::mid
