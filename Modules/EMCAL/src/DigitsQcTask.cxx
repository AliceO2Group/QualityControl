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
  double threshold = hasConfigValue("threshold") ? get_double(getConfigValue("threshold")) : 0.5;

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
    histos.mCellThreshold = threshold;
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

  mEvCounterTF = new TH1D("NEventsPerTF", "NEventsPerTF", 100, -0.5, 99.5);
  mEvCounterTF->GetXaxis()->SetTitle("NEventsPerTimeFrame");
  mEvCounterTF->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTF);

  mEvCounterTFPHYS = new TH1D("NEventsPerTFPHYS", "NEventsPerTFPHYS", 100, -0.5, 99.5);
  mEvCounterTFPHYS->GetXaxis()->SetTitle("NEventsPerTimeFrame_PHYS");
  mEvCounterTFPHYS->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTFPHYS);

  mEvCounterTFCALIB = new TH1D("NEventsPerTFCALIB", "NEventsPerTFCALIB", 100, -0.5, 99.5);
  mEvCounterTFCALIB->GetXaxis()->SetTitle("NEventsPerTimeFrame_CALIB");
  mEvCounterTFCALIB->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTFCALIB);
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
  for (auto trg : combinedEvents) {
    if (!trg.getNumberOfObjects()) {
      continue;
    }
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Next event " << eventcounter << " has " << trg.getNumberOfObjects() << " digits from " << trg.getNumberOfSubevents() << " subevent(s)" << QcInfoLogger::endm;

    //trigger type
    auto triggertype = trg.mTriggerType;
    bool isPhysTrigger = triggertype & o2::trigger::PhT, isCalibTrigger = triggertype & o2::trigger::Cal;
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
          histos.fillHistograms(digit, goodcell, timeoffset);
          ndigit++;
          ndigitGlobal++;
        }
      }
    }
    histos.countEvent();

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

  bool isPhysTrigger = trigger == "PHYS";
  if (isPhysTrigger) {
    if (hasAmpVsCellID) {
      mDigitAmplitude = histBuilder2D("digitAmplitudeHG", "Digit Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);

      //mDigitAmplitude[1] = histBuilder2D("digitAmplitudeLG", "Digit Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      if (hasHistosCalib2D) {
        mDigitAmplitudeCalib = histBuilder2D("digitAmplitudeHGCalib", "Digit Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);
        //mDigitAmplitudeCalib[1] = histBuilder2D("digitAmplitudeLGCalib", "Digit Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      }
    }
    if (hasTimeVsCellID) {
      mDigitTime = histBuilder2D("digitTimeHG", "Digit Time (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      // mDigitTime[1] = histBuilder2D("digitTimeLG", "Digit Time (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      if (hasHistosCalib2D) {
        mDigitTimeCalib = histBuilder2D("digitTimeHGCalib", "Digit Time Calib (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
        // mDigitTimeCalib[1] = histBuilder2D("digitTimeLGCalib", "Digit Time Calib (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      }
    }

    mDigitAmpSupermodule = histBuilder2D("digitAmplitudeSupermodule", "Digit amplitude vs. supermodule ID ", 400, 0., 100, 20, -0.5, 19.5, false);
    mDigitTimeSupermodule = histBuilder2D("digitTimeSupermodule", "Digit Time vs. supermodule ID (High gain)", 400, -200, 200, 20, -0.5, 19.5, false);
    if (hasHistosCalib2D) {
      mDigitAmpSupermoduleCalib = histBuilder2D("digitAmplitudeSupermoduleCalib", "Digit amplitude (Calib) vs. supermodule ID ", 400, 0., 100, 20, -0.5, 19.5, false);
      mDigitTimeSupermoduleCalib = histBuilder2D("digitTimeSupermoduleCalib", "Digit Time (Calib) vs. supermodule ID (High gain)", 400, -200, 200, 20, -0.5, 19.5, false);
    }
  }

  mDigitOccupancy = histBuilder2D("digitOccupancyEMC", "Digit Occupancy EMCAL", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mDigitOccupancyThr = histBuilder2D("digitOccupancyEMCwThr", Form("Digit Occupancy EMCAL with E>%.1f GeV/c", mCellThreshold), 96, -0.5, 95.5, 208, -0.5, 207.5, false);

  mIntegratedOccupancy = histBuilder2D("digitOccupancyInt", "Digit Occupancy Integrated", 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mIntegratedOccupancy->GetXaxis()->SetTitle("col");
  mIntegratedOccupancy->GetYaxis()->SetTitle("row");
  // 1D histograms for showing the integrated spectrum

  mDigitAmplitudeEMCAL = histBuilder1D("digitAmplitudeEMCAL", "Digit amplitude in EMCAL", 400, 0., 100.);
  mDigitAmplitudeDCAL = histBuilder1D("digitAmplitudeDCAL", "Digit amplitude in DCAL", 400, 0., 100.);
  mnumberEvents = histBuilder1D("NumberOfEvents", "Number Of Events", 1, 0.5, 1.5);
}

void DigitsQcTask::DigitsHistograms::fillHistograms(const o2::emcal::Cell& digit, bool goodCell, double timecalib)
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
    }
    fillOptional2D(mIntegratedOccupancy, col, row, digit.getEnergy());

  } catch (o2::emcal::InvalidCellIDException& e) {
    QcInfoLogger::GetInstance() << "Invalid cell ID: " << e.getCellID() << QcInfoLogger::endm;
  };

  try {
    auto cellindices = mGeometry->GetCellIndex(digit.getTower());
    auto supermoduleID = std::get<0>(cellindices);
    fillOptional2D(mDigitAmpSupermodule, digit.getEnergy(), supermoduleID);
    fillOptional2D(mDigitTimeSupermodule, digit.getTimeStamp(), supermoduleID);
    if (goodCell) {
      fillOptional2D(mDigitAmpSupermoduleCalib, digit.getEnergy(), supermoduleID);
      fillOptional2D(mDigitTimeSupermoduleCalib, digit.getTimeStamp() - timecalib, supermoduleID);
    }
    if (supermoduleID) {
      fillOptional1D(mDigitAmplitudeEMCAL, digit.getEnergy());
    } else {
      fillOptional1D(mDigitAmplitudeDCAL, digit.getEnergy());
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
  publishOptional(mDigitTimeSupermoduleCalib);
  publishOptional(mDigitAmplitudeEMCAL);
  publishOptional(mDigitAmplitudeDCAL);
  publishOptional(mDigitOccupancy);
  publishOptional(mDigitOccupancyThr);
  publishOptional(mIntegratedOccupancy);
  publishOptional(mnumberEvents);
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
  resetOptional(mDigitTimeSupermoduleCalib);
  resetOptional(mDigitAmplitudeEMCAL);
  resetOptional(mDigitAmplitudeDCAL);
  resetOptional(mDigitOccupancy);
  resetOptional(mDigitOccupancyThr);
  resetOptional(mIntegratedOccupancy);
  resetOptional(mnumberEvents);
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
  cleanOptional(mDigitTimeSupermoduleCalib);
  cleanOptional(mDigitAmplitudeEMCAL);
  cleanOptional(mDigitAmplitudeDCAL);
  cleanOptional(mDigitOccupancy);
  cleanOptional(mDigitOccupancyThr);
  cleanOptional(mIntegratedOccupancy);
  cleanOptional(mnumberEvents);
  //  cleanOptional(mEvCounterTF);
  //  cleanOptional(mEvCounterTFPHYS);
  //  cleanOptional(mEvCounterTFCALIB);
  //  cleanOptional(mTFPerCycles);
  //
}

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2
