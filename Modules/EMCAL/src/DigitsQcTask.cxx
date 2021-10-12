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
/// \author Markus Fasel, Cristina Terrevoli
///

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iostream>

#include <TCanvas.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <DataFormatsEMCAL/EMCALBlockHeader.h>
#include <DataFormatsEMCAL/TriggerRecord.h>
#include <DataFormatsEMCAL/Digit.h>
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/DigitsQcTask.h"
#include "DataFormatsEMCAL/Cell.h"
#include "EMCALBase/Geometry.h"
#include "EMCALCalib/BadChannelMap.h"
#include "EMCALCalib/TimeCalibrationParams.h"
#include <Framework/ConcreteDataMatcher.h>
#include <Framework/DataRefUtils.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <CommonConstants/Triggers.h>

namespace o2
{
namespace quality_control_modules
{
namespace emcal
{

DigitsQcTask::~DigitsQcTask()
{
  for (auto en : mHistogramContainer) {
    en.second.clean();
  }
  if (mEvCounterTF)
    delete mEvCounterTF;
  if (mEvCounterTFPHYS)
    delete mEvCounterTFPHYS;
  if (mEvCounterTFCALIB)
    delete mEvCounterTFCALIB;
  if (mTFPerCycles)
    delete mTFPerCycles;
  if (mTFPerCyclesTOT)
    delete mTFPerCyclesTOT;
  if (mDigitsMaxSM)
    delete mDigitsMaxSM;
}

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance().setDetector("EMC");
  QcInfoLogger::GetInstance() << "initialize DigitsQcTask" << AliceO2::InfoLogger::InfoLogger::endm;
  //define histograms

  auto get_bool = [](const std::string_view input) -> bool {
    return input == "true";
  };

  auto get_double = [](const std::string_view input) -> double {
    double result = 0.;
    if (input.length()) {
      try {
        result = std::stof(input.data());
      } catch (...) {
      }
    }
    return result;
  };

  auto hasAmpVsCell = get_bool(getConfigValueLower("hasAmpVsCell")),
       hasTimeVsCell = get_bool(getConfigValueLower("hasTimeVsCell")),
       hasCalib2D = get_bool(getConfigValueLower("hasHistValibVsCell"));

  mIgnoreTriggerTypes = get_bool(getConfigValue("ignoreTriggers"));
  //add a second threshold
  double thresholdCAL = hasConfigValue("threshold") ? get_double(getConfigValue("threshold")) : 0.5;
  double thresholdPHYS = hasConfigValue("threshold") ? get_double(getConfigValue("threshold")) : 0.2;

  if (hasAmpVsCell) {
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Enabling histograms : Amplitude vs. cellID" << QcInfoLogger::endm;
  }
  if (hasTimeVsCell) {
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Enabling histograms : Time vs. cellID" << QcInfoLogger::endm;
  }
  if (hasCalib2D) {
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Enabling calibrated histograms" << QcInfoLogger::endm;
  }

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);

  std::array<std::string, 2> triggers = { { "CAL", "PHYS" } };
  for (const auto& trg : triggers) {
    DigitsHistograms histos;
    histos.mCellThreshold = (trg == "CAL") ? thresholdCAL : thresholdPHYS;
    histos.mGeometry = mGeometry;
    histos.initForTrigger(trg.data(), hasAmpVsCell, hasTimeVsCell, hasCalib2D);
    histos.startPublishing(*getObjectsManager());
    mHistogramContainer[trg] = histos;
  } //trigger type
  //new histos`
  mTFPerCyclesTOT = new TH1D("NumberOfTFperCycles_TOT", "NumberOfTFperCycles_TOT", 100, -0.5, 99.5); //
  mTFPerCyclesTOT->GetXaxis()->SetTitle("NumberOfTFperCyclesTOT");
  mTFPerCyclesTOT->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mTFPerCyclesTOT);

  mTFPerCycles = new TH1D("NumberOfTFperCycles_1", "NumberOfTFperCycles_1", 1, -0.5, 1.5);
  mTFPerCycles->GetXaxis()->SetTitle("NumberOfTFperCycles");
  mTFPerCycles->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mTFPerCycles);

  mEvCounterTF = new TH1D("NEventsPerTF", "NEventsPerTF", 401, -0.5, 400.5);
  mEvCounterTF->GetXaxis()->SetTitle("NEventsPerTimeFrame");
  mEvCounterTF->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTF);

  mEvCounterTFPHYS = new TH1D("NEventsPerTFPHYS", "NEventsPerTFPHYS", 401, -0.5, 400.5);
  mEvCounterTFPHYS->GetXaxis()->SetTitle("NEventsPerTimeFrame_PHYS");
  mEvCounterTFPHYS->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTFPHYS);

  mEvCounterTFCALIB = new TH1D("NEventsPerTFCALIB", "NEventsPerTFCALIB", 6, -0.5, 5.5);
  mEvCounterTFCALIB->GetXaxis()->SetTitle("NEventsPerTimeFrame_CALIB");
  mEvCounterTFCALIB->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTFCALIB);

  mDigitsMaxSM = new TH1D("SMMaxNumDigits", "Supermodule with largest amount of digits", 20, -0.5, 19.5);
  mDigitsMaxSM->GetXaxis()->SetTitle("Supermodule");
  mDigitsMaxSM->GetYaxis()->SetTitle("counts");
  getObjectsManager()->startPublishing(mDigitsMaxSM);
}

void DigitsQcTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void DigitsQcTask::startOfCycle()
{
  mTimeFramesPerCycles = 0;
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
  std::map<std::string, std::string> metadata;
  mBadChannelMap = retrieveConditionAny<o2::emcal::BadChannelMap>("EMC/Calib/BadChannels", metadata);
  //it was EMC/BadChannelMap
  if (!mBadChannelMap)
    QcInfoLogger::GetInstance() << "No Bad Channel Map object " << AliceO2::InfoLogger::InfoLogger::endm;

  mTimeCalib = retrieveConditionAny<o2::emcal::TimeCalibrationParams>("EMC/Calib/Time", metadata);
  //"EMC/TimeCalibrationParams
  if (!mTimeCalib)
    QcInfoLogger::GetInstance() << " No Time Calib object " << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mTFPerCycles->Fill(1); //number of timeframe process per cycle
  mTimeFramesPerCycles++;
  // check if we have payoad
  using MaskType_t = o2::emcal::BadChannelMap::MaskType_t;

  if (mDoEndOfPayloadCheck) {
    auto dataref = ctx.inputs().get("emcal-digits");
    auto const* emcheader = o2::framework::DataRefUtils::getHeader<o2::emcal::EMCALBlockHeader*>(dataref);
    if (!emcheader->mHasPayload) {
      QcInfoLogger::GetInstance() << "No more digits" << AliceO2::InfoLogger::InfoLogger::endm;
      return;
    }
  }

  // Handling of inputs from multiple subevents (multiple FLPs)
  // Build maps of trigger records and cells according to the subspecification
  // and combine trigger records from different maps into a single map of range
  // references and subspecifications
  std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::Cell>> cellSubEvents;
  std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::TriggerRecord>> triggerRecordSubevents;

  auto posCells = ctx.inputs().getPos("emcal-digits"),
       posTriggerRecords = ctx.inputs().getPos("emcal-triggerecords");
  auto numSlotsCells = ctx.inputs().getNofParts(posCells),
       numSlotsTriggerRecords = ctx.inputs().getNofParts(posTriggerRecords);
  for (decltype(numSlotsCells) islot = 0; islot < numSlotsCells; islot++) {
    auto celldata = ctx.inputs().getByPos(posCells, islot);
    auto subspecification = framework::DataRefUtils::getHeader<header::DataHeader*>(celldata)->subSpecification;
    // Discard message if it is a deadbeaf message (empty timeframe)
    if (subspecification == 0xDEADBEEF) {
      continue;
    }
    cellSubEvents[subspecification] = ctx.inputs().get<gsl::span<o2::emcal::Cell>>(celldata);
  }
  for (decltype(numSlotsTriggerRecords) islot = 0; islot < numSlotsTriggerRecords; islot++) {
    auto trgrecorddata = ctx.inputs().getByPos(posTriggerRecords, islot);
    auto subspecification = framework::DataRefUtils::getHeader<header::DataHeader*>(trgrecorddata)->subSpecification;
    // Discard message if it is a deadbeaf message (empty timeframe)
    if (subspecification == 0xDEADBEEF) {
      continue;
    }
    triggerRecordSubevents[subspecification] = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(trgrecorddata);
  }

  auto combinedEvents = buildCombinedEvents(triggerRecordSubevents);

  //  QcInfoLogger::GetInstance() << "Received " << digitcontainer.size() << " digits " << AliceO2::InfoLogger::InfoLogger::endm;
  int eventcounter = 0;
  int eventcounterCALIB = 0;
  int eventcounterPHYS = 0;
  std::array<int, 20> numDigitsSM;
  std::fill(numDigitsSM.begin(), numDigitsSM.end(), 0);
  for (auto trg : combinedEvents) {
    if (!trg.getNumberOfObjects()) {
      continue;
    }
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Next event " << eventcounter << " has " << trg.getNumberOfObjects() << " digits from " << trg.getNumberOfSubevents() << " subevent(s)" << QcInfoLogger::endm;

    //trigger type
    auto triggertype = trg.mTriggerType;
    bool isPhysTrigger = mIgnoreTriggerTypes || (triggertype & o2::trigger::PhT),
         isCalibTrigger = (!mIgnoreTriggerTypes) && (triggertype & o2::trigger::Cal);
    std::string trgClass;
    if (isPhysTrigger) {
      trgClass = "PHYS";
      eventcounterPHYS++;
    } else if (isCalibTrigger) {
      trgClass = "CAL";
      eventcounterCALIB++;
    } else {
      QcInfoLogger::GetInstance() << QcInfoLogger::Error << " Unmonitored trigger class requested " << AliceO2::InfoLogger::InfoLogger::endm;
      continue;
    }

    auto bcphase = trg.mInteractionRecord.bc % 4; // to be fixed:4 histos for EMCAL, 4 histos for DCAL
    auto histos = mHistogramContainer[trgClass];

    // iterate over subevents
    for (auto& subev : trg.mSubevents) {
      auto cellsSubspec = cellSubEvents.find(subev.mSpecification);
      if (cellsSubspec == cellSubEvents.end()) {
        QcInfoLogger::GetInstance() << QcInfoLogger::Error << "No cell data found for subspecification " << subev.mSpecification << QcInfoLogger::endm;
      } else {
        QcInfoLogger::GetInstance() << QcInfoLogger::Debug << subev.mCellRange.getEntries() << " digits in subevent from equipment " << subev.mSpecification << QcInfoLogger::endm;
        gsl::span<const o2::emcal::Cell> eventdigits(cellsSubspec->second.data() + subev.mCellRange.getFirstEntry(), subev.mCellRange.getEntries());
        int ndigit = 0, ndigitGlobal = subev.mCellRange.getFirstEntry();
        for (auto digit : eventdigits) {
          //int index = digit.getHighGain() ? 0 : (digit.getLowGain() ? 1 : -1);
          //if (index < 0)
          //  continue;
          auto timeoffset = mTimeCalib ? mTimeCalib->getTimeCalibParam(digit.getTower(), digit.getLowGain()) : 0.;
          bool goodcell = true;
          if (mBadChannelMap) {
            goodcell = mBadChannelMap->getChannelStatus(digit.getTower()) != MaskType_t::GOOD_CELL;
          }
          histos.fillHistograms(digit, goodcell, timeoffset, bcphase);
          if (isPhysTrigger) {
            auto [sm, mod, iphi, ieta] = mGeometry->GetCellIndex(digit.getTower());
            numDigitsSM[sm]++;
          }
          ndigit++;
          ndigitGlobal++;
        }
      }
    }
    histos.countEvent();

    if (isPhysTrigger) {
      auto maxSM = std::max_element(numDigitsSM.begin(), numDigitsSM.end());
      auto indexMaxSM = maxSM - numDigitsSM.begin();
      mDigitsMaxSM->Fill(indexMaxSM);
    }

    eventcounter++;
  }
  mEvCounterTF->Fill(eventcounter);
  mEvCounterTFPHYS->Fill(eventcounterPHYS);
  mEvCounterTFCALIB->Fill(eventcounterCALIB);
  //event counter per TimeFrame  (range 0-100) for the moment (parameter)
}

void DigitsQcTask::endOfCycle()
{
  mTFPerCyclesTOT->Fill(mTimeFramesPerCycles); // do not reset this histo
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto cont : mHistogramContainer) {
    cont.second.reset();
  }
  if (mEvCounterTF)
    mEvCounterTF->Reset();
  if (mEvCounterTFPHYS)
    mEvCounterTFPHYS->Reset();
  if (mEvCounterTFCALIB)
    mEvCounterTFCALIB->Reset();
  if (mTFPerCycles)
    mTFPerCycles->Reset();
  if (mDigitsMaxSM)
    mDigitsMaxSM->Reset();
}

std::vector<DigitsQcTask::CombinedEvent> DigitsQcTask::buildCombinedEvents(const std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::TriggerRecord>>& triggerrecords) const
{
  std::vector<DigitsQcTask::CombinedEvent> events;

  // Search interaction records from all subevents
  std::set<o2::InteractionRecord> allInteractions;
  for (auto& [subspecification, trgrec] : triggerrecords) {
    for (auto rec : trgrec) {
      auto eventIR = rec.getBCData();
      if (allInteractions.find(eventIR) == allInteractions.end())
        allInteractions.insert(eventIR);
    }
  }

  // iterate over all subevents for all bunch crossings
  for (auto collision : allInteractions) {
    CombinedEvent nextevent;
    nextevent.mInteractionRecord = collision;
    bool first = true,
         hasSubevent = false;
    for (auto [subspecification, records] : triggerrecords) {
      auto found = std::find_if(records.begin(), records.end(), [&collision](const o2::emcal::TriggerRecord& rec) { return rec.getBCData() == collision; });
      if (found != records.end()) {
        hasSubevent = true;
        if (first) {
          nextevent.mTriggerType = found->getTriggerBits();
          first = false;
        }
        nextevent.mSubevents.push_back({ subspecification, o2::dataformats::RangeReference(found->getFirstEntry(), found->getNumberOfObjects()) });
      }
    }
    if (hasSubevent)
      events.emplace_back(nextevent);
  }
  return events;
}

bool DigitsQcTask::hasConfigValue(const std::string_view key)
{
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    return true;
  }
  return false;
}

std::string DigitsQcTask::getConfigValue(const std::string_view key)
{
  std::string result;
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    result = param->second;
  }
  return result;
}

std::string DigitsQcTask::getConfigValueLower(const std::string_view key)
{
  auto input = getConfigValue(key);
  std::string result;
  if (input.length()) {
    result = boost::algorithm::to_lower_copy(input);
  }
  return result;
}

void DigitsQcTask::DigitsHistograms::initForTrigger(const std::string trigger, bool hasAmpVsCellID, bool hasTimeVsCellID, bool hasHistosCalib2D)
{
  auto histBuilder1D = [trigger](const std::string name, const std::string title, int nbinsx, double xmin, double xmax) -> TH1* {
    std::string histname = name + "_" + trigger,
                histtitle = title + " " + trigger;
    return new TH1D(histname.data(), histtitle.data(), nbinsx, xmin, xmax);
  };
  auto histBuilder2D = [trigger](const std::string_view name, const std::string_view title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double ymax, bool profile) -> TH2* {
    std::string histname = std::string(name.data()) + "_" + trigger,
                histtitle = std::string(title.data()) + " " + trigger;
    if (profile)
      return new TProfile2D(histname.data(), histtitle.data(), nbinsx, xmin, xmax, nbinsy, ymin, ymax);
    return new TH2D(histname.data(), histtitle.data(), nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  };

  std::map<std::string, double> maxAmps = { { "PHYS", 50. }, { "CAL", 10. } };
  double maxAmp = maxAmps[trigger];

  bool isPhysTrigger = trigger == "PHYS";
  if (isPhysTrigger) {
    if (hasAmpVsCellID) {
      mDigitAmplitude = histBuilder2D("digitAmplitudeHG", "Digit Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);
      mDigitAmplitude->SetStats(0);
      //mDigitAmplitude[1] = histBuilder2D("digitAmplitudeLG", "Digit Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      if (hasHistosCalib2D) {
        mDigitAmplitudeCalib = histBuilder2D("digitAmplitudeHGCalib", "Digit Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);
        mDigitAmplitudeCalib->SetStats(0);
        //mDigitAmplitudeCalib[1] = histBuilder2D("digitAmplitudeLGCalib", "Digit Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      }
    }
    if (hasTimeVsCellID) {
      mDigitTime = histBuilder2D("digitTimeHG", "Digit Time (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      mDigitTime->SetStats(0);
      // mDigitTime[1] = histBuilder2D("digitTimeLG", "Digit Time (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      if (hasHistosCalib2D) {
        mDigitTimeCalib = histBuilder2D("digitTimeHGCalib", "Digit Time Calib (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
        mDigitTimeCalib->SetStats(0);
        // mDigitTimeCalib[1] = histBuilder2D("digitTimeLGCalib", "Digit Time Calib (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      }
    }

    mDigitAmpSupermodule = histBuilder2D("digitAmplitudeSupermodule", "Digit amplitude vs. supermodule ID ", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
    mDigitAmpSupermodule->SetStats(0);
    mDigitTimeSupermodule = histBuilder2D("digitTimeSupermodule", "Digit Time vs. supermodule ID ", 600, -400, 800, 20, -0.5, 19.5, false);
    mDigitTimeSupermodule->SetStats(0);
    if (hasHistosCalib2D) {
      mDigitAmpSupermoduleCalib = histBuilder2D("digitAmplitudeSupermoduleCalib", "Digit amplitude (Calib) vs. supermodule ID ", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
      mDigitAmpSupermoduleCalib->SetStats(0);
      mDigitTimeSupermoduleCalib = histBuilder2D("digitTimeSupermoduleCalib", "Digit Time (Calib) vs. supermodule ID (High gain)", 600, -400, 800, 20, -0.5, 19.5, false);
      mDigitTimeSupermoduleCalib->SetStats(0);
    }
  }

  mDigitOccupancy = histBuilder2D("digitOccupancyEMC", "Digit Occupancy EMCAL", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mDigitOccupancy->SetStats(0);
  mDigitOccupancyThr = histBuilder2D("digitOccupancyEMCwThr", Form("Digit Occupancy EMCAL,DCAL with E>%.1f GeV/c", mCellThreshold), 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mDigitOccupancyThr->SetStats(0);
  mDigitOccupancyThrBelow = histBuilder2D("digitOccupancyEMCwThrBelow", Form("Digit Occupancy EMCAL,DCAL with E<%.1f GeV/c", mCellThreshold), 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mDigitOccupancyThrBelow->SetStats(0);

  mIntegratedOccupancy = histBuilder2D("digitOccupancyInt", "Digit Occupancy Integrated", 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mIntegratedOccupancy->GetXaxis()->SetTitle("col");
  mIntegratedOccupancy->GetYaxis()->SetTitle("row");
  mIntegratedOccupancy->SetStats(0);
  // 1D histograms for showing the integrated spectrum

  mDigitTimeSupermodule_tot = histBuilder1D("digitTime", "Digit Time EMCAL,DCAL", 600, -400, 800);
  mDigitTimeSupermoduleEMCAL = histBuilder1D("digitTimeEMCAL", "Digit Time EMCAL", 600, -400, 800);
  mDigitTimeSupermoduleDCAL = histBuilder1D("digitTimeDCAL", "Digit Time DCAL", 600, -400, 800);
  mDigitAmplitude_tot = histBuilder1D("digitAmplitude", "Digit amplitude in EMCAL,DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mDigitAmplitudeEMCAL = histBuilder1D("digitAmplitudeEMCAL", "Digit amplitude in EMCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mDigitAmplitudeDCAL = histBuilder1D("digitAmplitudeDCAL", "Digit amplitude in DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mnumberEvents = histBuilder1D("NumberOfEvents", "Number Of Events", 1, 0.5, 1.5);
  if (isPhysTrigger) {
    // Phys. trigger: monitor all bunch crossings
    for (auto bcID = 0; bcID < 4; bcID++) {
      std::array<TH1*, 20> digitTimeBCSM;
      for (auto smID = 0; smID < 20; smID++) {
        digitTimeBCSM[smID] = histBuilder1D(Form("digitTimeBC%dSM%d", bcID, smID), Form("Digit Time BC%d SM%d", bcID, smID), 600, -400, 800);
      }
      mDigitTimeBC[bcID] = digitTimeBCSM;
    }
  } else {
    // Calib trigger: Only bc0;
    std::array<TH1*, 20> digitTimeBCSM;
    for (auto smID = 0; smID < 20; smID++) {
      digitTimeBCSM[smID] = histBuilder1D(Form("digitTimeBC0SM%d", smID), Form("Digit Time BC0 SM%d", smID), 600, -400, 800);
    }
    mDigitTimeBC[0] = digitTimeBCSM;
  }
}

void DigitsQcTask::DigitsHistograms::fillHistograms(const o2::emcal::Cell& digit, bool goodCell, double timecalib, int bcphase)
{
  auto fillOptional1D = [](TH1* hist, double x, double weight = 1.) {
    if (hist)
      hist->Fill(x, weight);
  };
  auto fillOptional2D = [](TH2* hist, double x, double y, double weight = 1.) {
    if (hist)
      hist->Fill(x, y, weight);
  };

  fillOptional2D(mDigitAmplitude, digit.getEnergy(), digit.getTower());
  //fillOptional2D(mDigitAmplitude[index], digit.getEnergy(), digit.getTower());

  if (goodCell) {
    fillOptional2D(mDigitAmplitudeCalib, digit.getEnergy(), digit.getTower());
    fillOptional2D(mDigitTimeCalib, digit.getTimeStamp() - timecalib, digit.getTower());
    //fillOptional2D(mDigitAmplitudeCalib[index], digit.getEnergy(), digit.getTower());
    //fillOptional2D(mDigitTimeCalib[index], digit.getTimeStamp() - timeoffset, digit.getTower());
  }
  fillOptional2D(mDigitTime, digit.getTimeStamp(), digit.getTower());
  //fillOptional2D(mDigitTime[index], digit.getTimeStamp(), digit.getTower());

  try {
    auto [row, col] = mGeometry->GlobalRowColFromIndex(digit.getTower());
    if (digit.getEnergy() > 0) {
      fillOptional2D(mDigitOccupancy, col, row);
    }
    if (digit.getEnergy() > mCellThreshold) {
      fillOptional2D(mDigitOccupancyThr, col, row);
    } else {
      fillOptional2D(mDigitOccupancyThrBelow, col, row);
    }

    fillOptional2D(mIntegratedOccupancy, col, row, digit.getEnergy());

  } catch (o2::emcal::InvalidCellIDException& e) {
    QcInfoLogger::GetInstance() << QcInfoLogger::Error << "Invalid cell ID: " << e.getCellID() << QcInfoLogger::endm;
  };

  try {
    auto cellindices = mGeometry->GetCellIndex(digit.getTower());
    auto supermoduleID = std::get<0>(cellindices);
    fillOptional2D(mDigitAmpSupermodule, digit.getEnergy(), supermoduleID);
    if (digit.getEnergy() > 0.3)
      fillOptional2D(mDigitTimeSupermodule, digit.getTimeStamp(), supermoduleID);
    if (goodCell) {
      fillOptional2D(mDigitAmpSupermoduleCalib, digit.getEnergy(), supermoduleID);
      if (digit.getEnergy() > 0.3)
        fillOptional2D(mDigitTimeSupermoduleCalib, digit.getTimeStamp() - timecalib, supermoduleID);
    }
    fillOptional1D(mDigitAmplitude_tot, digit.getEnergy()); //EMCAL+DCAL, shifter
    if (digit.getEnergy() > 0.15)
      fillOptional1D(mDigitTimeSupermodule_tot, digit.getTimeStamp()); //EMCAL+DCAL shifter
    if (supermoduleID < 12) {

      if (digit.getEnergy() > 0.15)
        fillOptional1D(mDigitTimeSupermoduleEMCAL, digit.getTimeStamp());
      fillOptional1D(mDigitAmplitudeEMCAL, digit.getEnergy()); //EMCAL
    } else {
      fillOptional1D(mDigitAmplitudeDCAL, digit.getEnergy());
      if (digit.getEnergy() > 0.15)
        fillOptional1D(mDigitTimeSupermoduleDCAL, digit.getTimeStamp());
    }
    // bc phase histograms
    if (digit.getEnergy() > 0.15) {
      auto bchistos = mDigitTimeBC.find(bcphase);
      if (bchistos != mDigitTimeBC.end()) {
        auto histcontainer = bchistos->second;
        histcontainer[supermoduleID]->Fill(digit.getTimeStamp());
      }
    }
  } catch (o2::emcal::InvalidCellIDException& e) {
    QcInfoLogger::GetInstance() << "Invalid cell ID: " << e.getCellID() << QcInfoLogger::endm;
  }
}

void DigitsQcTask::DigitsHistograms::countEvent()
{
  mnumberEvents->Fill(1);
}

void DigitsQcTask::DigitsHistograms::startPublishing(o2::quality_control::core::ObjectsManager& manager)
{
  auto publishOptional = [&manager](TH1* hist) {
    if (hist)
      manager.startPublishing(hist);
  };

  publishOptional(mDigitTime);
  publishOptional(mDigitTimeCalib);
  publishOptional(mDigitAmplitude);
  publishOptional(mDigitAmplitudeCalib);
  publishOptional(mDigitAmpSupermodule);
  publishOptional(mDigitAmpSupermoduleCalib);
  publishOptional(mDigitTimeSupermodule);
  publishOptional(mDigitTimeSupermodule_tot);
  publishOptional(mDigitTimeSupermoduleEMCAL);
  publishOptional(mDigitTimeSupermoduleDCAL);
  publishOptional(mDigitTimeSupermoduleCalib);
  publishOptional(mDigitAmplitude_tot);
  publishOptional(mDigitAmplitudeEMCAL);
  publishOptional(mDigitAmplitudeDCAL);
  publishOptional(mDigitOccupancy);
  publishOptional(mDigitOccupancyThr);
  publishOptional(mDigitOccupancyThrBelow);
  publishOptional(mIntegratedOccupancy);
  publishOptional(mnumberEvents);
  for (auto& [bcID, histos] : mDigitTimeBC) {
    for (auto hist : histos)
      publishOptional(hist);
  }
  //  publishOptional(mEvCounterTF);
  //  publishOptional(mEvCounterTFPHYS);
  //  publishOptional(mEvCounterTFCALIB);
  //  publishOptional(mTFPerCycles);
}

void DigitsQcTask::DigitsHistograms::reset()
{

  //  for (auto h : mDigitAmplitude) {
  //    h->Reset();
  //  }
  //  for (auto h : mDigitTime) {
  //    h->Reset();
  //  }
  //  for (auto h : mDigitAmplitudeCalib) {
  //    h->Reset();
  //  }
  //  for (auto h : mDigitTimeCalib) {
  //    h->Reset();
  //  }

  auto resetOptional = [](TH1* hist) {
    if (hist)
      hist->Reset();
  };

  resetOptional(mDigitTime);
  resetOptional(mDigitTimeCalib);
  resetOptional(mDigitAmplitude);
  resetOptional(mDigitAmplitudeCalib);
  resetOptional(mDigitAmpSupermodule);
  resetOptional(mDigitAmpSupermoduleCalib);
  resetOptional(mDigitTimeSupermodule);
  resetOptional(mDigitTimeSupermodule_tot);
  resetOptional(mDigitTimeSupermoduleEMCAL);
  resetOptional(mDigitTimeSupermoduleDCAL);
  resetOptional(mDigitTimeSupermoduleCalib);
  resetOptional(mDigitAmplitude_tot);
  resetOptional(mDigitAmplitudeEMCAL);
  resetOptional(mDigitAmplitudeDCAL);
  resetOptional(mDigitOccupancy);
  resetOptional(mDigitOccupancyThr);
  resetOptional(mDigitOccupancyThrBelow);
  resetOptional(mIntegratedOccupancy);
  resetOptional(mnumberEvents);
  for (auto& [bcID, histos] : mDigitTimeBC) {
    for (auto hist : histos)
      resetOptional(hist);
  }
  //  resetOptional(mEvCounterTF);
  //  resetOptional(mEvCounterTFPHYS);
  //  resetOptional(mEvCounterTFCALIB);
  //  resetOptional(mTFPerCycles);
}

void DigitsQcTask::DigitsHistograms::clean()
{
  //for (auto h : mDigitAmplitude) {
  //  delete h;
  //}
  // for (auto h : mDigitTime) {
  //  delete h;
  // }
  //for (auto h : mDigitAmplitudeCalib) {
  //  delete h;
  //}
  //for (auto h : mDigitTimeCalib) {
  //  delete h;
  // }

  auto cleanOptional = [](TObject* hist) {
    if (hist)
      delete hist;
  };

  cleanOptional(mDigitTime);
  cleanOptional(mDigitTimeCalib);
  cleanOptional(mDigitAmplitude);
  cleanOptional(mDigitAmplitudeCalib);
  cleanOptional(mDigitAmpSupermodule);
  cleanOptional(mDigitAmpSupermoduleCalib);
  cleanOptional(mDigitTimeSupermodule);
  cleanOptional(mDigitTimeSupermodule_tot);
  cleanOptional(mDigitTimeSupermoduleEMCAL);
  cleanOptional(mDigitTimeSupermoduleDCAL);
  cleanOptional(mDigitTimeSupermoduleCalib);
  cleanOptional(mDigitAmplitude_tot);
  cleanOptional(mDigitAmplitudeEMCAL);
  cleanOptional(mDigitAmplitudeDCAL);
  cleanOptional(mDigitOccupancy);
  cleanOptional(mDigitOccupancyThr);
  cleanOptional(mDigitOccupancyThrBelow);
  cleanOptional(mIntegratedOccupancy);
  cleanOptional(mnumberEvents);
  for (auto& [bcID, histos] : mDigitTimeBC) {
    for (auto hist : histos)
      cleanOptional(hist);
  }
  //  cleanOptional(mEvCounterTF);
  //  cleanOptional(mEvCounterTFPHYS);
  //  cleanOptional(mEvCounterTFCALIB);
  //  cleanOptional(mTFPerCycles);
  //
}

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2
