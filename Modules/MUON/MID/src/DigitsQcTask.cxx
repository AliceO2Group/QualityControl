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

#include "MID/DigitsQcTask.h"

#include <string>
#include <fmt/format.h>

#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"
#include "MIDWorkflow/ColumnDataSpecsUtils.h"
#include "MID/DigitsHelper.h"

namespace o2::quality_control_modules::mid
{

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Devel) << "initialize DigitsQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will

  mNbDigitTF = std::make_unique<TH1F>("NbDigitTF", "NbTimeFrame", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbDigitTF.get());

  mROFTimeDiff = std::make_unique<TH2F>("ROFTimeDiff", "ROF time difference vs. min. ROF size", 100, 0, 100, 100, 0, 100);
  mROFTimeDiff->SetOption("colz");
  getObjectsManager()->startPublishing(mROFTimeDiff.get());

  std::array<string, 4> chId{ "11", "12", "21", "22" };

  for (size_t ich = 0; ich < 5; ++ich) {
    std::string chName = "";
    if (ich < 4) {
      chName = "MT" + chId[ich];
    }
    mMultHitB[ich] = std::make_unique<TH1F>(fmt::format("MultHit{}B", chName).c_str(), fmt::format("Multiplicity Hits - {} bending plane", chName).c_str(), 300, 0, 300);
    getObjectsManager()->startPublishing(mMultHitB[ich].get());
    mMultHitNB[ich] = std::make_unique<TH1F>(fmt::format("MultHit{}NB", chName).c_str(), fmt::format("Multiplicity Hits - {} non-bending plane", chName).c_str(), 300, 0, 300);
    getObjectsManager()->startPublishing(mMultHitNB[ich].get());
  }

  mMeanMultiHits = std::make_unique<TH1F>("MeanMultiHits", "Min Hits Multiplicity", 8, 0, 8.);
  for (int icath = 0; icath < 2; ++icath) {
    std::string cathName = (icath == 0) ? "B" : "NB";
    for (int ich = 0; ich < 4; ++ich) {
      int ibin = 1 + 4 * icath + ich;
      mMeanMultiHits->GetXaxis()->SetBinLabel(ibin, fmt::format("MT{}{}", chId[ich], cathName).c_str());
    }
  }
  getObjectsManager()->startPublishing(mMeanMultiHits.get());

  for (int ich = 0; ich < 5; ++ich) {
    std::string name = "LocalBoardsMap";
    std::string title = "Local boards Occupancy Map";
    if (ich < 4) {
      name += chId[ich];
      title += "MT" + chId[ich];
    }

    mLocalBoardsMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeBoardMapHisto(name, title));
    getObjectsManager()->startPublishing(mLocalBoardsMap[ich].get());
    getObjectsManager()->setDefaultDrawOptions(mLocalBoardsMap[ich].get(), "COLZ");
  }

  mHits = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("Hits", "Fired strips"));
  getObjectsManager()->startPublishing(mHits.get());

  for (int ich = 0; ich < 4; ++ich) {
    mBendHitsMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("BendHitsMap{}", chId[ich]), fmt::format("Bending Hits Map MT{}", chId[ich]), 0));
    getObjectsManager()->startPublishing(mBendHitsMap[ich].get());
    getObjectsManager()->setDefaultDrawOptions(mBendHitsMap[ich].get(), "COLZ");
  }

  for (int ich = 0; ich < 4; ++ich) {
    mNBendHitsMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("NBendHitsMap{}", chId[ich]), fmt::format("Non-Bending Hits Map MT{}", chId[ich]), 1));
    getObjectsManager()->startPublishing(mNBendHitsMap[ich].get());
    getObjectsManager()->setDefaultDrawOptions(mNBendHitsMap[ich].get(), "COLZ");
  }

  mDigitBCCounts = std::make_unique<TH1F>("DigitBCCounts", "Digits Bunch Crossing Counts", o2::constants::lhc::LHCMaxBunches, 0., o2::constants::lhc::LHCMaxBunches);
  getObjectsManager()->startPublishing(mDigitBCCounts.get());
  mDigitBCCounts->GetXaxis()->SetTitle("BC");
  mDigitBCCounts->GetYaxis()->SetTitle("Number of digits");
}

void DigitsQcTask::startOfActivity(const Activity& /*activity*/)
{
}

void DigitsQcTask::startOfCycle()
{
}

void DigitsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mNbDigitTF->Fill(0.5, 1.);
  auto digits = o2::mid::specs::getData(ctx, "digits", o2::mid::EventType::Standard);
  auto rofs = o2::mid::specs::getRofs(ctx, "digits", o2::mid::EventType::Standard);

  std::array<unsigned long int, 4> evtSizeB{};
  std::array<unsigned long int, 4> evtSizeNB{};

  unsigned long int prevSize = 0;
  o2::InteractionRecord prevIr;
  bool isFirst = true;
  for (auto& rof : rofs) {
    auto eventDigits = digits.subspan(rof.firstEntry, rof.nEntries);
    evtSizeB.fill(0);
    evtSizeNB.fill(0);
    for (auto& col : eventDigits) {
      auto ich = o2::mid::detparams::getChamber(col.deId);
      evtSizeB[ich] = mDigitsHelper.countDigits(col, 0);
      evtSizeNB[ich] = mDigitsHelper.countDigits(col, 1);
      mDigitsHelper.fillStripHisto(col, mHits.get());
    }

    unsigned long int sizeTot = 0;
    for (int ich = 0; ich < 4; ++ich) {
      sizeTot += evtSizeB[ich];
      sizeTot += evtSizeNB[ich];
      mMultHitB[ich]->Fill(evtSizeB[ich]);
      mMultHitB[4]->Fill(evtSizeB[ich]);
      mMultHitNB[ich]->Fill(evtSizeNB[ich]);
      mMultHitNB[4]->Fill(evtSizeNB[ich]);
    }

    if (!isFirst) {
      unsigned long int sizeMin = (sizeTot < prevSize) ? sizeTot : prevSize;
      auto timeDiff = rof.interactionRecord.differenceInBC(prevIr);
      mROFTimeDiff->Fill(timeDiff, sizeMin);
    }

    isFirst = false;
    prevSize = sizeTot;
    prevIr = rof.interactionRecord;

    mDigitBCCounts->Fill(rof.interactionRecord.bc, sizeTot);
  }
}

void DigitsQcTask::endOfCycle()
{
  // Fill here the 2D representation of the fired strips/boards
  // First reset the old histograms
  resetDisplayHistos();

  // Then fill from the strip histogram
  mDigitsHelper.fillMapHistos(mHits.get(), mBendHitsMap, mNBendHitsMap, mLocalBoardsMap);
}

void DigitsQcTask::endOfActivity(const Activity& /*activity*/)
{
}

void DigitsQcTask::resetDisplayHistos()
{
  for (auto& histo : mBendHitsMap) {
    histo->Reset();
  }
  for (auto& histo : mNBendHitsMap) {
    histo->Reset();
  }
  for (auto& histo : mLocalBoardsMap) {
    histo->Reset();
  }
}

void DigitsQcTask::reset()
{
  // clean all the monitor objects here

  mNbDigitTF->Reset();
  mROFTimeDiff->Reset();

  for (auto& histo : mMultHitB) {
    histo->Reset();
  }
  for (auto& histo : mMultHitNB) {
    histo->Reset();
  }
  mMeanMultiHits->Reset();

  mHits->Reset();
  resetDisplayHistos();

  mDigitBCCounts->Reset();
}

} // namespace o2::quality_control_modules::mid
