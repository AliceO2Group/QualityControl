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
/// \file   RecoFITQcTask.cxx
/// \author Artur Furs afurs@cern.ch

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TROOT.h>

#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"
#include <FIT/RecoFITQcTask.h>
#include <FITCommon/HelperHist.h>
#include <DataFormatsFDD/RecPoint.h>
#include <DataFormatsFT0/RecPoints.h>
#include <DataFormatsFV0/RecPoints.h>

#include "Framework/ProcessingContext.h"
#include "Framework/InputRecord.h"
#include "Framework/ConcreteDataMatcher.h"
#include "Framework/InitContext.h"
#include "Framework/DeviceSpec.h"
#include <vector>
#include <set>
#include <chrono>
namespace o2::quality_control_modules::fit
{

RecoFITQcTask::~RecoFITQcTask()
{
}

void RecoFITQcTask::initialize(o2::framework::InitContext& ctx)
{
  const auto& inputRouts = ctx.services().get<const o2::framework::DeviceSpec>().inputs;
  std::set<std::string> setOfBindings{};
  for (const auto& route : inputRouts) {
    setOfBindings.insert(route.matcher.binding);
  }
  if (setOfBindings.find("recPointsFDD") != setOfBindings.end() && setOfBindings.find("channelsFDD") != setOfBindings.end()) {
    mIsFDD = true;
    LOG(debug) << "FDD DPL channel is detected";
  }
  if (setOfBindings.find("recPointsFT0") != setOfBindings.end() && setOfBindings.find("channelsFT0") != setOfBindings.end()) {
    mIsFT0 = true;
    LOG(debug) << "FT0 DPL channel is detected";
  }
  if (setOfBindings.find("recPointsFV0") != setOfBindings.end() && setOfBindings.find("channelsFV0") != setOfBindings.end()) {
    mIsFV0 = true;
    LOG(debug) << "FV0 DPL channel is detected";
  }
  const auto mapFDD_FT0 = helper::multiplyMaps({ { "FDD ", HelperTrgFIT::sMapBasicTrgBitsFDD, " && " }, { "FT0 ", HelperTrgFIT::sMapBasicTrgBitsFT0, "" } });
  const auto mapFDD_FV0 = helper::multiplyMaps({ { "FDD ", HelperTrgFIT::sMapBasicTrgBitsFDD, " && " }, { "FV0 ", HelperTrgFIT::sMapBasicTrgBitsFV0, "" } });
  const auto mapFT0_FV0 = helper::multiplyMaps({ { "FT0 ", HelperTrgFIT::sMapBasicTrgBitsFT0, " && " }, { "FV0 ", HelperTrgFIT::sMapBasicTrgBitsFV0, "" } });

  const auto mapFDD_FT0_FV0 = helper::multiplyMaps({ { "FDD ", HelperTrgFIT::sMapBasicTrgBitsFDD, " && " }, { "FT0 ", HelperTrgFIT::sMapBasicTrgBitsFT0, " && " }, { "FV0 ", HelperTrgFIT::sMapBasicTrgBitsFV0, "" } });
  mHistTrgCorrelationFDD_FT0 = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", "TrgCorrelationFDD_FT0", "Correlation between trigger signals: FDD & FT0;BC;Triggers", sNBC, 0, sNBC, mapFDD_FT0);
  mHistTrgCorrelationFDD_FV0 = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", "TrgCorrelationFDD_FV0", "Correlation between trigger signals: FDD & FV0;BC;Triggers", sNBC, 0, sNBC, mapFDD_FV0);
  mHistTrgCorrelationFT0_FV0 = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", "TrgCorrelationFT0_FV0", "Correlation between trigger signals: FT0 & FV0;BC;Triggers", sNBC, 0, sNBC, mapFT0_FV0);

  //  mHistTrgCorrelationFDD_FT0_FV0 = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", "TrgCorrelationFDD_FT0_FV0","Correlation between trigger signals: FDD & FT0 & FV0;BC;Triggers",sNBC,0,sNBC,mapFDD_FT0_FV0);
  for (const auto& enTrgFT0 : HelperTrgFIT::sMapBasicTrgBitsFT0) {
    const std::string histName = "TrgCorrelation_FT0" + enTrgFT0.second + "_FDD_FV0";
    const std::string histTitle = "Correlation between trigger signals (FT0 " + enTrgFT0.second + "): FDD & FV0;BC;Triggers";
    mHistTrgCorrelationFT0_FDD_FV0[enTrgFT0.first] = helper::registerHist<TH2F>(getObjectsManager(), "COLZ", histName, histTitle, sNBC, 0, sNBC, mapFDD_FV0);
  }
}

void RecoFITQcTask::startOfActivity(const Activity& activity)
{
}

void RecoFITQcTask::startOfCycle()
{
}

void RecoFITQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  const auto& vecChannelsFDD = mIsFDD ? ctx.inputs().get<gsl::span<o2::fdd::ChannelDataFloat>>("channelsFDD") : gsl::span<o2::fdd::ChannelDataFloat>{};
  const auto& vecRecPointsFDD = mIsFDD ? ctx.inputs().get<gsl::span<o2::fdd::RecPoint>>("recPointsFDD") : gsl::span<o2::fdd::RecPoint>{};

  const auto& vecChannelsFT0 = mIsFT0 ? ctx.inputs().get<gsl::span<o2::ft0::ChannelDataFloat>>("channelsFT0") : gsl::span<o2::ft0::ChannelDataFloat>{};
  const auto& vecRecPointsFT0 = mIsFT0 ? ctx.inputs().get<gsl::span<o2::ft0::RecPoints>>("recPointsFT0") : gsl::span<o2::ft0::RecPoints>{};

  const auto& vecChannelsFV0 = mIsFV0 ? ctx.inputs().get<gsl::span<o2::fv0::ChannelDataFloat>>("channelsFV0") : gsl::span<o2::fv0::ChannelDataFloat>{};
  const auto& vecRecPointsFV0 = mIsFV0 ? ctx.inputs().get<gsl::span<o2::fv0::RecPoints>>("recPointsFV0") : gsl::span<o2::fv0::RecPoints>{};

  const auto& mapDigitSync = DigitSyncFIT::makeSyncMap(vecRecPointsFDD, vecRecPointsFT0, vecRecPointsFV0);
  const auto start = std::chrono::high_resolution_clock::now();
  for (const auto& entry : mapDigitSync) {
    const o2::InteractionRecord& ir = entry.first;
    const auto& digitSync = entry.second;
    const auto& recPointsFDD = digitSync.getDigitFDD(vecRecPointsFDD); // o2::fdd::RecPoint
    const auto& recPointsFT0 = digitSync.getDigitFT0(vecRecPointsFT0); // o2::ft0::RecPoints
    const auto& recPointsFV0 = digitSync.getDigitFV0(vecRecPointsFV0); // o2::fv0::RecPoints
    constexpr uint8_t allowedTrgBits = 0b11111;                        // to suppress tech trg bits(laset, dataIsValid, etc)
    const auto trgFDD = recPointsFDD.getTrigger().getTriggersignals() & allowedTrgBits;
    const auto trgFT0 = recPointsFT0.getTrigger().getTriggersignals() & allowedTrgBits;
    const auto trgFV0 = recPointsFV0.getTrigger().getTriggersignals() & allowedTrgBits;
    // Too many small nested loops(max size = 5) , better to find another way for correlation calc?
    for (const auto trgBitFDD : HelperTrgFIT::sArrDecomposed1Byte[trgFDD]) {
      for (const auto trgBitFT0 : HelperTrgFIT::sArrDecomposed1Byte[trgFT0]) {
        const auto trgCorrStatusFDD_FT0 = mFuncTrgStatusFDD_FT0(trgBitFDD, trgBitFT0);
        mHistTrgCorrelationFDD_FT0->Fill(ir.bc, trgCorrStatusFDD_FT0);
      }
      for (const auto trgBitFV0 : HelperTrgFIT::sArrDecomposed1Byte[trgFV0]) {
        const auto trgCorrStatusFDD_FV0 = mFuncTrgStatusFDD_FV0(trgBitFDD, trgBitFV0);
        mHistTrgCorrelationFDD_FV0->Fill(ir.bc, trgCorrStatusFDD_FV0);
        for (const auto trgBitFT0 : HelperTrgFIT::sArrDecomposed1Byte[trgFT0]) {
          mHistTrgCorrelationFT0_FDD_FV0[trgBitFT0]->Fill(ir.bc, trgCorrStatusFDD_FV0);
        }
      }
    }
    for (const auto trgBitFT0 : HelperTrgFIT::sArrDecomposed1Byte[trgFT0]) {
      for (const auto trgBitFV0 : HelperTrgFIT::sArrDecomposed1Byte[trgFV0]) {
        const auto trgCorrStatusFT0_FV0 = mFuncTrgStatusFT0_FV0(trgBitFT0, trgBitFV0);
        mHistTrgCorrelationFT0_FV0->Fill(ir.bc, trgCorrStatusFT0_FV0);
      }
    }
  }
  const auto stop = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  LOG(debug) << "FIT reco QC delay: " << duration.count();
}

void RecoFITQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // one has to set num. of entries manually because
  // default TH1Reductor gets only mean,stddev and entries (no integral)
}

void RecoFITQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void RecoFITQcTask::reset()
{
  // clean all the monitor objects here
  // mHistTrgCorrelationFDD_FT0_FV0->Reset();
  mHistTrgCorrelationFDD_FT0->Reset();
  mHistTrgCorrelationFDD_FV0->Reset();
  mHistTrgCorrelationFT0_FV0->Reset();
  for (int iTrgBit = 0; iTrgBit < 5; iTrgBit++) {
    mHistTrgCorrelationFT0_FDD_FV0[iTrgBit]->Reset();
  }
}

} // namespace o2::quality_control_modules::fit
