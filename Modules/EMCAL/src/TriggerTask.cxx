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
/// \file   TriggerTask.cxx
/// \author Markus Fasel
///

#include <algorithm>

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>

#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/TriggerTask.h"
#include "DataFormatsEMCAL/CompressedTriggerData.h"
#include "DataFormatsEMCAL/TriggerRecord.h"
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

namespace o2::quality_control_modules::emcal
{

TriggerTask::~TriggerTask()
{
  delete mTRUFired;
  delete mFastORFired;
  delete mPositionFasORFired;
  delete mNumberOfTRUsPerEvent;
  delete mNumberOfPatchesPerEvent;
  delete mPatchEnergySpectrumPerEvent;
  delete mLeadingPatchEnergySpectrumPerEvent;
  delete mPatchEnergyTRU;
  delete mLeadingPatchEnergyTRU;
  delete mNumberOfPatchesPerTRU;
  delete mPatchIndexFired;
  delete mPatchIndexLeading;
  delete mTRUTime;
  delete mPatchTime;
  delete mNumberTimesumsEvent;
  delete mL0Timesums;
  delete mL0TimesumsTRU;
  delete mADCMaxTimesum;
  delete mFastORIndexMaxTimesum;
  delete mPositionMaxTimesum;
  delete mIntegratedTimesums;
  delete mAverageTimesum;
}

void TriggerTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  mTRUFired = new TH1F("NumberOfFiredTRUs", "Number of triggers per TRU; TRU index; Number of triggers", o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mTRUFired);
  mFastORFired = new TH1F("NumberOfPatchesWithFastOR", "Number of patches per FastOR; FastOR abs. ID; Number of patches", o2::emcal::TriggerMappingV2::ALLFASTORS, -0.5, o2::emcal::TriggerMappingV2::ALLFASTORS - 0.5);
  getObjectsManager()->startPublishing(mFastORFired);
  mPositionFasORFired = new TH2F("PositionFastOrInPatch", "Position of FastORs in patches; column; row", o2::emcal::TriggerMappingV2::FASTORSETA, -0.5, o2::emcal::TriggerMappingV2::FASTORSETA - 0.5, o2::emcal::TriggerMappingV2::FASTORSPHI, -0.5, o2::emcal::TriggerMappingV2::FASTORSPHI - 0.5);
  getObjectsManager()->startPublishing(mPositionFasORFired);
  mNumberOfTRUsPerEvent = new TH1F("NumberOfTRUsPerEvent", "Number of fired TRUs per event; Number of TRUs; Number of events", 20, -0.5, 19.5);
  getObjectsManager()->startPublishing(mNumberOfTRUsPerEvent);
  mNumberOfPatchesPerEvent = new TH1F("NumberOfPatchesPerEvent", "Number of patches per event; Number of L0 patches; Number of events", 100, -0.5, 99.5);
  getObjectsManager()->startPublishing(mNumberOfPatchesPerEvent);
  mPatchEnergySpectrumPerEvent = new TH1F("PatchEnergySpectrum", "Patch energy spectrum; Patch ADC; Yield", 2000, 0., 2000);
  getObjectsManager()->startPublishing(mPatchEnergySpectrumPerEvent);
  mLeadingPatchEnergySpectrumPerEvent = new TH1F("LeadingPatchEnergySpectrum", "Leading patch energy spectrum; Patch ADC; Yield", 2000, 0., 2000);
  getObjectsManager()->startPublishing(mLeadingPatchEnergySpectrumPerEvent);
  mPatchEnergyTRU = new TH2F("PatchEnergySpectrumTRU", "Patch energy spectrum; Patch ADC; TRU index", 2000, 0., 2000, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mPatchEnergyTRU);
  mLeadingPatchEnergyTRU = new TH2F("LeadingPatchEnergySpectrumTRU", "Leading patch energy spectrum; Patch ADC; TRU index", 2000, 0., 2000, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mLeadingPatchEnergyTRU);
  mNumberOfPatchesPerTRU = new TH2F("NumberOfPatchesPerTRU", "Number of trigger patches per TRU and event; Patch ADC; TRU index", 20, -0.5, 19.5, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mNumberOfPatchesPerTRU);
  mPatchIndexFired = new TH2F("PatchIndexFired", "Fired trigger patches; Patch index; TRU index", o2::emcal::TriggerMappingV2::PATCHESINTRU, -0.5, o2::emcal::TriggerMappingV2::PATCHESINTRU - 0.5, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mPatchIndexFired);
  mPatchIndexLeading = new TH2F("LeadingPatchIndexFired", "Leading trigger patches; Patch index; TRU index", o2::emcal::TriggerMappingV2::PATCHESINTRU, -0.5, o2::emcal::TriggerMappingV2::PATCHESINTRU - 0.5, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mPatchIndexLeading);
  mTRUTime = new TH2F("TRUTime", "TRU time; TRU time sample; TRU index", 12, -0.5, 11.5, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mTRUTime);
  mPatchTime = new TH2F("PatchTime", "Patch time; Patch time sample; TRU index", 12, -0.5, 11.5, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mPatchTime);
  mLeadingPatchTime = new TH2F("LeadingPatchTime", "Leading patch time; Patch time sample; TRU index", 12, -0.5, 11.5, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mLeadingPatchTime);
  mNumberTimesumsEvent = new TH1F("NumberL0TimesumsEvent", "Number of L0 timesums per event; Number of L0 timesums; Number of events", 1000, 0., 1000.);
  getObjectsManager()->startPublishing(mNumberTimesumsEvent);
  mL0Timesums = new TH1F("L0Timesums", "L0 timesums; L0 timesum (ADC counts); yield", 2048, 0., 2048);
  getObjectsManager()->startPublishing(mL0Timesums);
  mL0TimesumsTRU = new TH2F("L0TimesumsTRU", "L0 timesums per TRU; L0 timesum (ADC counts); TRU index", 2048, 0., 2048, o2::emcal::TriggerMappingV2::ALLTRUS, -0.5, o2::emcal::TriggerMappingV2::ALLTRUS - 0.5);
  getObjectsManager()->startPublishing(mL0TimesumsTRU);
  mADCMaxTimesum = new TH1F("MaxL0Timesum", "Max. L0 timesum per event; L0 timesum (ADC counts); Yield", 2048, 0., 2048);
  getObjectsManager()->startPublishing(mADCMaxTimesum);
  mFastORIndexMaxTimesum = new TH1F("FastORMaxTimesum", "FastOR ID of the max. L0 timesum; FastOR abs. ID; Yield", o2::emcal::TriggerMappingV2::ALLFASTORS, -0.5, o2::emcal::TriggerMappingV2::ALLFASTORS - 0.5);
  getObjectsManager()->startPublishing(mFastORIndexMaxTimesum);
  mPositionMaxTimesum = new TH2F("PositionMaxTimesum", "FastOR position of the max. L0 timesum; column; row", o2::emcal::TriggerMappingV2::FASTORSETA, -0.5, o2::emcal::TriggerMappingV2::FASTORSETA - 0.5, o2::emcal::TriggerMappingV2::FASTORSPHI, -0.5, o2::emcal::TriggerMappingV2::FASTORSPHI - 0.5);
  getObjectsManager()->startPublishing(mPositionMaxTimesum);
  mIntegratedTimesums = new TH2F("IntegratedTimesums", "Integrated L0 timesums per FastOR; column; row", o2::emcal::TriggerMappingV2::FASTORSETA, -0.5, o2::emcal::TriggerMappingV2::FASTORSETA - 0.5, o2::emcal::TriggerMappingV2::FASTORSPHI, -0.5, o2::emcal::TriggerMappingV2::FASTORSPHI - 0.5);
  getObjectsManager()->startPublishing(mIntegratedTimesums);
  mAverageTimesum = new TProfile2D("AverageTimesums", "Average L0 timesums per FastOR; column; row", o2::emcal::TriggerMappingV2::FASTORSETA, -0.5, o2::emcal::TriggerMappingV2::FASTORSETA - 0.5, o2::emcal::TriggerMappingV2::FASTORSPHI, -0.5, o2::emcal::TriggerMappingV2::FASTORSPHI - 0.5);
  getObjectsManager()->startPublishing(mAverageTimesum);

  mTriggerMapping = std::make_unique<o2::emcal::TriggerMappingV2>();
}

void TriggerTask::startOfActivity(const Activity& activity)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void TriggerTask::startOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TriggerTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto trus = ctx.inputs().get<gsl::span<o2::emcal::CompressedTRU>>("truinfo");
  auto trurecords = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>("trurecords");
  auto patches = ctx.inputs().get<gsl::span<o2::emcal::CompressedTriggerPatch>>("patchinfos");
  auto patchrecords = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>("patchrecords");
  auto timesums = ctx.inputs().get<gsl::span<o2::emcal::CompressedL0TimeSum>>("timesums");
  auto timesumrecords = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>("timesumrecords");
  ILOG(Debug, Devel) << "Found " << trurecords.size() << " TRU trigger records for " << trus.size() << " TRU info objects ..." << ENDM;
  ILOG(Debug, Devel) << "Found " << patchrecords.size() << " patch trigger records for " << patches.size() << " patch objects ..." << ENDM;
  ILOG(Debug, Devel) << "Found " << timesumrecords.size() << " timesum trigger records for " << timesums.size() << " L0 timesums ..." << ENDM;

  for (const auto& bc : getAllBCs(trurecords, patchrecords, timesumrecords)) {
    gsl::span<const o2::emcal::CompressedTRU> eventTRUs;
    gsl::span<const o2::emcal::CompressedTriggerPatch> eventPatches;
    gsl::span<const o2::emcal::CompressedL0TimeSum> eventTimesums;
    auto recordfinder = [&bc](const o2::emcal::TriggerRecord& rec) {
      return bc == rec.getBCData();
    };
    auto trurecfound = std::find_if(trurecords.begin(), trurecords.end(), recordfinder);
    auto patchrecfound = std::find_if(patchrecords.begin(), patchrecords.end(), recordfinder);
    auto timesumrecfound = std::find_if(timesumrecords.begin(), timesumrecords.end(), recordfinder);
    if (trurecfound != trurecords.end()) {
      eventTRUs = trus.subspan(trurecfound->getFirstEntry(), trurecfound->getNumberOfObjects());
    }
    if (patchrecfound != patchrecords.end()) {
      eventPatches = patches.subspan(patchrecfound->getFirstEntry(), patchrecfound->getNumberOfObjects());
    }
    if (timesumrecfound != timesumrecords.end()) {
      eventTimesums = timesums.subspan(timesumrecfound->getFirstEntry(), timesumrecfound->getNumberOfObjects());
    }
    processEvent(eventTRUs, eventPatches, eventTimesums);
  }
}

void TriggerTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TriggerTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TriggerTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mTRUFired->Reset();
  mFastORFired->Reset();
  mPositionFasORFired->Reset();
  mNumberOfTRUsPerEvent->Reset();
  mNumberOfPatchesPerEvent->Reset();
  mPatchEnergySpectrumPerEvent->Reset();
  mLeadingPatchEnergySpectrumPerEvent->Reset();
  mPatchEnergyTRU->Reset();
  mLeadingPatchEnergyTRU->Reset();
  mNumberOfPatchesPerTRU->Reset();
  mPatchIndexFired->Reset();
  mPatchIndexLeading->Reset();
  mTRUTime->Reset();
  mPatchTime->Reset();
  mLeadingPatchTime->Reset();
  mNumberTimesumsEvent->Reset();
  mL0Timesums->Reset();
  mL0TimesumsTRU->Reset();
  mADCMaxTimesum->Reset();
  mFastORIndexMaxTimesum->Reset();
  mPositionMaxTimesum->Reset();
  mIntegratedTimesums->Reset();
  mAverageTimesum->Reset();
}

std::vector<o2::InteractionRecord> TriggerTask::getAllBCs(const gsl::span<const o2::emcal::TriggerRecord> trurecords, const gsl::span<const o2::emcal::TriggerRecord> patchrecords, const gsl::span<const o2::emcal::TriggerRecord> timesumrecords) const
{
  std::vector<o2::InteractionRecord> result;
  auto add_to_result = [&result](const o2::emcal::TriggerRecord& rec) {
    if (std::find(result.begin(), result.end(), rec.getBCData()) == result.end()) {
      result.emplace_back(rec.getBCData());
    }
  };
  for (const auto& trurecord : trurecords) {
    result.emplace_back(trurecord.getBCData());
  }
  for (const auto& patchrecord : patchrecords) {
    add_to_result(patchrecord);
  }
  for (const auto& timesumrecord : timesumrecords) {
    add_to_result(timesumrecord);
  }

  std::sort(result.begin(), result.end(), std::less<>());
  return result;
}

void TriggerTask::processEvent(const gsl::span<const o2::emcal::CompressedTRU> trudata, const gsl::span<const o2::emcal::CompressedTriggerPatch> triggerpatches, const gsl::span<const o2::emcal::CompressedL0TimeSum> timesums)
{
  uint8_t numFiredTRU = 0;
  for (const auto& tru : trudata) {
    if (tru.mFired) {
      mTRUFired->Fill(tru.mTRUIndex);
      mTRUTime->Fill(tru.mTriggerTime, tru.mTRUIndex);
      numFiredTRU++;
    }
  }
  mNumberOfTRUsPerEvent->Fill(numFiredTRU);
  mNumberOfPatchesPerEvent->Fill(triggerpatches.size());
  uint16_t leadingadc = 0;
  uint8_t timeleading = UCHAR_MAX,
          indexleading = UCHAR_MAX,
          indexTRUleading = UCHAR_MAX;
  std::array<uint8_t, 52> numpatches;
  std::fill(numpatches.begin(), numpatches.end(), 0);
  for (auto& patch : triggerpatches) {
    numpatches[patch.mTRUIndex]++;
    mPatchIndexFired->Fill(patch.mPatchIndexInTRU, patch.mTRUIndex);
    mPatchEnergySpectrumPerEvent->Fill(patch.mADC);
    mPatchEnergyTRU->Fill(patch.mADC, patch.mTRUIndex);
    mPatchTime->Fill(patch.mTime, patch.mTRUIndex);
    auto fastORs = mTriggerMapping->getFastORIndexFromL0Index(patch.mTRUIndex, patch.mPatchIndexInTRU, 4);
    for (auto fastor : fastORs) {
      mFastORFired->Fill(fastor);
      auto [col, row] = mTriggerMapping->getPositionInEMCALFromAbsFastORIndex(fastor);
      mPositionFasORFired->Fill(col, row);
    }
    if (patch.mADC > leadingadc) {
      leadingadc = patch.mADC;
      indexleading = patch.mPatchIndexInTRU;
      indexTRUleading = patch.mTRUIndex;
      timeleading = patch.mTime;
    }
  }
  if (indexleading < UCHAR_MAX) {
    mLeadingPatchEnergySpectrumPerEvent->Fill(leadingadc);
    mLeadingPatchEnergyTRU->Fill(leadingadc, indexTRUleading);
    mPatchIndexLeading->Fill(indexleading, indexTRUleading);
    mLeadingPatchTime->Fill(timeleading, indexTRUleading);
  }
  for (auto itru = 0; itru < 52; itru++) {
    if (numpatches[itru]) {
      mNumberOfPatchesPerTRU->Fill(numpatches[itru], itru);
    }
  }

  uint16_t maxtimesum = 0,
           indexMaxTimesum = USHRT_MAX;
  mNumberTimesumsEvent->Fill(timesums.size());
  for (const auto& timesum : timesums) {
    mL0Timesums->Fill(timesum.mTimesum);
    auto [indexTRU, indexInTRU] = mTriggerMapping->getTRUFromAbsFastORIndex(timesum.mIndex);
    mL0TimesumsTRU->Fill(timesum.mTimesum, indexTRU);
    auto [col, row] = mTriggerMapping->getPositionInEMCALFromAbsFastORIndex(timesum.mIndex);
    mIntegratedTimesums->Fill(col, row, timesum.mTimesum);
    mAverageTimesum->Fill(col, row, timesum.mTimesum);
    if (timesum.mTimesum > maxtimesum) {
      maxtimesum = timesum.mTimesum;
      indexMaxTimesum = timesum.mIndex;
    }
  }
  if (indexMaxTimesum < USHRT_MAX) {
    mADCMaxTimesum->Fill(maxtimesum);
    mFastORIndexMaxTimesum->Fill(indexMaxTimesum);
    auto [col, row] = mTriggerMapping->getPositionInEMCALFromAbsFastORIndex(indexMaxTimesum);
    mPositionMaxTimesum->Fill(col, row);
  }
}

} // namespace o2::quality_control_modules::emcal
