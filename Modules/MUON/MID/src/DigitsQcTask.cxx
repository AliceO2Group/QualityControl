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
#include "MIDBase/Mapping.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"
#include "MIDWorkflow/ColumnDataSpecsUtils.h"
#include "MIDGlobalMapping/GlobalMapper.h"
#include "MIDRaw/CrateMapper.h"
#include "MIDRaw/CrateParameters.h"
#include "MIDRaw/Decoder.h"
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

  mNbLBEmpty = std::make_unique<TH1F>("NbLBEmpty", "NbLocalBoardEmpty", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbLBEmpty.get());

  mNbLBHighRate = std::make_unique<TH1F>("NbLBHighRate", "NbLocalBoardHighRate", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbLBHighRate.get());

  mLBHighRate = std::make_unique<TH1F>("LBHighRate", "LocalBoardHigherRate", 1, 0, 1.);
  getObjectsManager()->startPublishing(mLBHighRate.get());

  mGBTRate = std::make_unique<TH1F>("GBTRate", "GBTRate", 32, 0, 32.);
  getObjectsManager()->startPublishing(mGBTRate.get());

  mCRURate = std::make_unique<TH1F>("CRURate", "CRURate", 2, 0, 2.);
  getObjectsManager()->startPublishing(mCRURate.get());

  mEPRate = std::make_unique<TH1F>("EPRate", "EPRate", 4, 0, 4.);
  getObjectsManager()->startPublishing(mEPRate.get());

  std::array<std::string, 4> chId{ "11", "12", "21", "22" };

  for (size_t i = 0; i < 36; ++i) {
    std::string nline = "";
    nline = std::to_string(i + 1);
    mLineLocalResp[i] = std::make_unique<TH1F>(fmt::format("RespBoardLine{}", nline).c_str(), fmt::format("RespBoardLine{}", nline).c_str(), 1, 0, 1);
    getObjectsManager()->startPublishing(mLineLocalResp[i].get());
  }

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

  mLocalBoardsMap = mDigitsHelper.makeBoardMapHistos("LocalBoardsMap", "Local boards Occupancy Map");

  for (int ich = 0; ich < 4; ++ich) {
    getObjectsManager()->startPublishing(mLocalBoardsMap[ich].get());
    getObjectsManager()->setDefaultDrawOptions(mLocalBoardsMap[ich].get(), "COLZ");
  }

  mLocalBoardsMapTot = std::make_unique<TH2F>(mDigitsHelper.makeBoardMapHisto("LocalBoardsMap", "Local boards Occupancy Map"));
  getObjectsManager()->startPublishing(mLocalBoardsMapTot.get());
  getObjectsManager()->setDefaultDrawOptions(mLocalBoardsMapTot.get(), "COLZ");

  mHits = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("Hits", "Fired strips"));
  getObjectsManager()->startPublishing(mHits.get());

  mBendHitsMap = mDigitsHelper.makeStripMapHistos("BendHitsMap", "Bending Hits Map", 0);
  for (int ich = 0; ich < 4; ++ich) {
    getObjectsManager()->startPublishing(mBendHitsMap[ich].get());
    getObjectsManager()->setDefaultDrawOptions(mBendHitsMap[ich].get(), "COLZ");
  }

  mNBendHitsMap = mDigitsHelper.makeStripMapHistos("NBendHitsMap", "Non-Bending Hits Map", 1);
  for (int ich = 0; ich < 4; ++ich) {
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
  reset();
}

void DigitsQcTask::startOfCycle()
{
}

void DigitsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mNbDigitTF->Fill(0.5, 1.);

  auto digits = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("digits");
  auto rofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("digits_rof");

  // auto digits = o2::mid::specs::getData(ctx, "digits", o2::mid::EventType::Standard);
  // auto rofs = o2::mid::specs::getRofs(ctx, "digits", o2::mid::EventType::Standard);

  std::array<unsigned long int, 4> evtSizeB{};
  std::array<unsigned long int, 4> evtSizeNB{};

  unsigned long int prevSize = 0;
  o2::InteractionRecord prevIr;
  bool isFirst = true;

  o2::mid::CrateMapper cm;

  for (auto& rof : rofs) {
    auto eventDigits = digits.subspan(rof.firstEntry, rof.nEntries);
    evtSizeB.fill(0);
    evtSizeNB.fill(0);

    for (auto& col : eventDigits) {

      auto ich = o2::mid::detparams::getChamber(col.deId);
      evtSizeB[ich] += mDigitsHelper.countDigits(col, 0);
      evtSizeNB[ich] += mDigitsHelper.countDigits(col, 1);
      mDigitsHelper.fillStripHisto(col, mHits.get());

      for (int lineId = 0; lineId < 4; lineId++) {
        if (col.getBendPattern(lineId)) {
          auto locId = cm.deLocalBoardToRO(col.deId, col.columnId, lineId);

          int crateId = o2::mid::raw::getCrateId(locId);
          int cruId = o2::mid::crateparams::isRightSide(crateId) ? 0 : 1;
          int epId = 0;
          switch (crateId % 8) {
            case 0:
            case 1:
            case 4:
            case 5:
              epId = 0;
              break;
            case 2:
            case 3:
            case 6:
            case 7:
              epId = 1;
              break;
          }
          auto locIdInCrate = o2::mid::raw::getLocId(locId);
          auto gbtId = o2::mid::crateparams::getGBTIdFromBoardInCrate(locIdInCrate);
          auto gbtUniqueId = o2::mid::crateparams::makeGBTUniqueId(crateId, gbtId);
          mGBTRate->Fill(gbtUniqueId);
          mCRURate->Fill(cruId);
          int epIndex = 2 * cruId + epId;
          mEPRate->Fill(epIndex);
        }
      }
    }

    unsigned long int sizeTot = 0;
    for (int ich = 0; ich < 4; ++ich) {
      sizeTot += evtSizeB[ich] + evtSizeNB[ich];
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
  mDigitsHelper.fillStripMapHistos(mHits.get(), mBendHitsMap, mNBendHitsMap);
  mDigitsHelper.fillBoardMapHistosFromStrips(mHits.get(), mLocalBoardsMap, mLocalBoardsMap);

  for (auto& histo : mLocalBoardsMap) {
    mLocalBoardsMapTot->Add(histo.get());
  }
}

void DigitsQcTask::endOfActivity(const Activity& /*activity*/)
{
  reset();
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
  mLocalBoardsMapTot->Reset();
}

void DigitsQcTask::reset()
{
  // clean all the monitor objects here

  mNbDigitTF->Reset();
  mROFTimeDiff->Reset();
  mNbLBEmpty->Reset();
  mNbLBHighRate->Reset();
  mLBHighRate->Reset();

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

  mGBTRate->Reset();
  mCRURate->Reset();
  mEPRate->Reset();
}

} // namespace o2::quality_control_modules::mid
