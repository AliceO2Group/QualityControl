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
/// \file   CellTask.cxx
/// \author Markus Fasel, Cristina Terrevoli
///

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iostream>

#include <TCanvas.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <DataFormatsEMCAL/TriggerRecord.h>
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/CellTask.h"
#include "CommonConstants/LHCConstants.h"
#include "DataFormatsEMCAL/Cell.h"
#include "EMCALBase/Geometry.h"
#include "EMCALCalib/CalibDB.h"
#include "EMCALCalib/BadChannelMap.h"
#include "EMCALCalib/GainCalibrationFactors.h"
#include "EMCALCalib/TimeCalibrationParams.h"
#include <Framework/ConcreteDataMatcher.h>
#include <Framework/DataRefUtils.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <CommonConstants/Triggers.h>
#include "EMCAL/DrawGridlines.h"
#include <set>

namespace o2::quality_control_modules::emcal
{

CellTask::~CellTask()
{
  auto cleanOptional = [](auto* hist) {
    if (hist) {
      delete hist;
    }
  };
  for (auto en : mHistogramContainer) {
    en.second.clean();
  }
  cleanOptional(mEvCounterTF);
  cleanOptional(mEvCounterTFPHYS);
  cleanOptional(mEvCounterTFCALIB);
  cleanOptional(mTFPerCycles);
  cleanOptional(mTFPerCyclesTOT);
  cleanOptional(mBCCounterPHYS);
  cleanOptional(mBCCounterCalib);
  cleanOptional(mCellsMaxSM);
  cleanOptional(mCells_ev_sm);
  cleanOptional(mCells_ev_sm_good);
  cleanOptional(mCells_ev_sm_bad);
  cleanOptional(mCells_ev_smThr);
  cleanOptional(mCells_ev);
  cleanOptional(mCells_ev_good);
  cleanOptional(mCells_ev_bad);
  cleanOptional(mCells_ev_Thres);
  cleanOptional(mCells_ev_EMCAL);
  cleanOptional(mCells_ev_EMCAL_good);
  cleanOptional(mCells_ev_EMCAL_bad);
  cleanOptional(mCells_ev_EMCAL_Thres);
  cleanOptional(mCells_ev_DCAL);
  cleanOptional(mCells_ev_DCAL_good);
  cleanOptional(mCells_ev_DCAL_bad);
  cleanOptional(mCells_ev_DCAL_Thres);
  cleanOptional(mFracGoodCellsEvent);
  cleanOptional(mFracGoodCellsSM);
  cleanOptional(mTotalEnergy);
  cleanOptional(mTotalEnergyCorr);
  cleanOptional(mTotalEnergySM);
}

void CellTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::setDetector("EMC");
  ILOG(Debug, Devel) << "initialize CellTask" << ENDM;
  // define histograms

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

  if (hasConfigValue("debuggerDelay")) {
    if (get_bool("debuggerDelay")) {
      // set delay in order to allow for attaching the debugger
      sleep(20);
    }
  }

  mTaskSettings.mHasAmpVsCellID = get_bool(getConfigValueLower("hasAmpVsCell")),
  mTaskSettings.mHasTimeVsCellID = get_bool(getConfigValueLower("hasTimeVsCell")),
  mTaskSettings.mHasHistosCalib = get_bool(getConfigValueLower("hasHistCalib"));
  mTaskSettings.mCalibrateEnergy = get_bool(getConfigValue("calibrateEnergy"));
  mTaskSettings.mIsHighMultiplicity = get_bool(getConfigValue("highMultiplicity"));
  if (hasConfigValue("thresholdTimePhys")) {
    mTaskSettings.mAmpThresholdTimePhys = get_double(getConfigValue("thresholdTimePhys"));
  }
  if (hasConfigValue("thresholdTimeCalib")) {
    mTaskSettings.mAmpThresholdTimeCalib = get_double(getConfigValue("thresholdTimeCalib"));
  }
  if (hasConfigValue("thresholdCAL")) {
    mTaskSettings.mThresholdCAL = get_double(getConfigValue("thresholdCAL"));
  }
  if (hasConfigValue("thresholdPHYS")) {
    mTaskSettings.mThresholdPHYS = get_double(getConfigValue("thresholdPHYS"));
  }
  if (hasConfigValue("thresholdTotalEnergy")) {
    mTaskSettings.mThresholdTotalEnergy = get_double(getConfigValue("thresholdTotalEnergy"));
  }
  if (hasConfigValue("ThresholdAvEnergy")) {
    mTaskSettings.mThresholdAvEnergy = get_double(getConfigValue("ThresholdAvEnergy"));
  }
  if (hasConfigValue("ThresholdAvTime")) {
    mTaskSettings.mThresholdAvTime = get_double(getConfigValue("ThresholdAvTime"));
  }
  ILOG(Info, Support) << "Apply energy calibration: " << (mTaskSettings.mCalibrateEnergy ? "yes" : "no") << ENDM;
  ILOG(Info, Support) << "Amplitude cut time histograms (PhysTrigger) " << mTaskSettings.mAmpThresholdTimePhys << ENDM;
  ILOG(Info, Support) << "Amplitude cut time histograms (CalibTrigger) " << mTaskSettings.mAmpThresholdTimeCalib << ENDM;
  ILOG(Info, Support) << "Amplitude cut occupancy histograms (PhysTrigger) " << mTaskSettings.mThresholdPHYS << ENDM;
  ILOG(Info, Support) << "Amplitude cut occupancy histograms (CalibTrigger) " << mTaskSettings.mThresholdCAL << ENDM;
  ILOG(Info, Support) << "Energy threshold av. energy histogram (constrained) " << mTaskSettings.mThresholdAvEnergy << " GeV/c" << ENDM;
  ILOG(Info, Support) << "Time threshold av. time histogram (constrained) " << mTaskSettings.mThresholdAvTime << " ns" << ENDM;
  ILOG(Info, Support) << "Multiplicity mode: " << (mTaskSettings.mIsHighMultiplicity ? "High multiplicity" : "Low multiplicity") << ENDM;

  mIgnoreTriggerTypes = get_bool(getConfigValue("ignoreTriggers"));

  if (mTaskSettings.mHasAmpVsCellID) {
    ILOG(Debug, Support) << "Enabling histograms : Amplitude vs. cellID" << ENDM;
  }
  if (mTaskSettings.mHasTimeVsCellID) {
    ILOG(Debug, Support) << "Enabling histograms : Time vs. cellID" << ENDM;
  }
  if (mTaskSettings.mHasHistosCalib) {
    ILOG(Debug, Support) << "Enabling calibrated histograms" << ENDM;
  }

  parseMultiplicityRanges();
  initDefaultMultiplicityRanges();

  // initialize geometry
  if (!mGeometry) {
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
  }

  std::array<std::string, 2> triggers = { { "CAL", "PHYS" } };
  for (const auto& trg : triggers) {
    CellHistograms histos;
    histos.mGeometry = mGeometry;
    histos.initForTrigger(trg.data(), mTaskSettings);
    histos.startPublishing(*getObjectsManager());
    mHistogramContainer[trg] = histos;
  } // trigger type
  // new histos
  mTFPerCyclesTOT = new TH1D("NumberOfTFperCycles_TOT", "NumberOfTFperCycles_TOT", 100, -0.5, 99.5); //
  mTFPerCyclesTOT->GetXaxis()->SetTitle("NumberOfTFperCyclesTOT");
  mTFPerCyclesTOT->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mTFPerCyclesTOT);

  mTFPerCycles = new TH1D("NumberOfTFperCycles_1", "NumberOfTFperCycles_1", 1, -0.5, 1.5);
  mTFPerCycles->GetXaxis()->SetTitle("NumberOfTFperCycles");
  mTFPerCycles->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mTFPerCycles);

  mBCCounterPHYS = new TH1D("NumberOfTriggerPerBC_PHYS", "Number of Triggers in bunch crossing (physics triggers)", o2::constants::lhc::LHCMaxBunches + 1, -0.5, o2::constants::lhc::LHCMaxBunches + 0.5);
  mBCCounterPHYS->GetXaxis()->SetTitle("Bunch crossing");
  mBCCounterPHYS->GetYaxis()->SetTitle("Number of triggers");
  getObjectsManager()->startPublishing(mBCCounterPHYS);

  mBCCounterCalib = new TH1D("NumberOfTriggerPerBC_CALIB", "Number of Triggers in bunch crossing (calibration triggers)", o2::constants::lhc::LHCMaxBunches + 1, -0.5, o2::constants::lhc::LHCMaxBunches + 0.5);
  mBCCounterCalib->GetXaxis()->SetTitle("Bunch crossing");
  mBCCounterCalib->GetYaxis()->SetTitle("Number of triggers");
  getObjectsManager()->startPublishing(mBCCounterCalib);

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

  mCellsMaxSM = new TH1D("SMMaxNumCells", "Supermodule with largest amount of cells", 20, -0.5, 19.5);
  mCellsMaxSM->GetXaxis()->SetTitle("Supermodule");
  mCellsMaxSM->GetYaxis()->SetTitle("counts");
  getObjectsManager()->startPublishing(mCellsMaxSM);

  mCells_ev_sm = new TH2D("ncellsPerEventSupermodule", "# of Cells per Events vs supermodule ID", mTaskSettings.mMultiplicityRangeSM, 0, mTaskSettings.mMultiplicityRangeSM, 20, -0.5, 19.5);
  mCells_ev_sm->GetYaxis()->SetTitle("Supermodule");
  mCells_ev_sm->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev_sm->SetStats(false);
  getObjectsManager()->startPublishing(mCells_ev_sm);

  mCells_ev_smThr = new TH2D("ncellsPerEventSupermoduleWThr", "# of Cells per Events vs supermodule ID Threshold", mTaskSettings.mMultiplicityRangeSMThreshold, 0, mTaskSettings.mMultiplicityRangeSMThreshold, 20, -0.5, 19.5);
  mCells_ev_smThr->GetYaxis()->SetTitle("Supermodule");
  mCells_ev_smThr->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev_smThr->SetStats(false);
  getObjectsManager()->startPublishing(mCells_ev_smThr);

  mCells_ev = new TH1D("ncellsPerEventTot", "# of Cells per event", mTaskSettings.mMultiplicityRange, 0, mTaskSettings.mMultiplicityRange);
  mCells_ev->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev->GetYaxis()->SetTitle("Events");
  mCells_ev->SetStats(false);
  getObjectsManager()->startPublishing(mCells_ev);

  mCells_ev_Thres = new TH1D("ncellPerEventTot_Thres", "# of Cells per event above threshold", mTaskSettings.mMultiplicityRangeThreshold, 0, mTaskSettings.mMultiplicityRangeThreshold);
  mCells_ev_Thres->SetStats(false);
  mCells_ev_Thres->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev_Thres->GetYaxis()->SetTitle("Events");
  getObjectsManager()->startPublishing(mCells_ev_Thres);

  mCells_ev_EMCAL = new TH1D("ncellsPerEventEMCALTot", "# of Cells per events in EMCAL", mTaskSettings.mMultiplicityRangeDetector, 0, mTaskSettings.mMultiplicityRangeDetector);
  mCells_ev_EMCAL->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev_EMCAL->GetYaxis()->SetTitle("Events");
  mCells_ev_EMCAL->SetStats(false);
  getObjectsManager()->startPublishing(mCells_ev_EMCAL);

  mCells_ev_EMCAL_Thres = new TH1D("ncellPerEventEMCALTot_Thres", "# of Cells per event in EMCAL above threshold", mTaskSettings.mMultiplicityRangeThreshold, 0, mTaskSettings.mMultiplicityRangeThreshold);
  mCells_ev_EMCAL_Thres->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev_EMCAL_Thres->GetYaxis()->SetTitle("Events");
  mCells_ev_EMCAL_Thres->SetStats(false);
  getObjectsManager()->startPublishing(mCells_ev_EMCAL_Thres);

  mCells_ev_DCAL = new TH1D("ncellsPerEventDCALTot", "# of Cells per event in DCAL", mTaskSettings.mMultiplicityRangeDetector, 0, mTaskSettings.mMultiplicityRangeDetector);
  mCells_ev_DCAL->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev_DCAL->GetYaxis()->SetTitle("Events");
  mCells_ev_DCAL->SetStats(false);
  getObjectsManager()->startPublishing(mCells_ev_DCAL);

  mCells_ev_DCAL_Thres = new TH1D("ncellPerEventDCALTot_Thres", "# of Cells per event in DCAL above threshold", mTaskSettings.mMultiplicityRangeThreshold, 0, mTaskSettings.mMultiplicityRangeThreshold);
  mCells_ev_DCAL_Thres->GetXaxis()->SetTitle("Cells/Event");
  mCells_ev_DCAL_Thres->GetYaxis()->SetTitle("Events");
  mCells_ev_DCAL_Thres->SetStats(false);
  getObjectsManager()->startPublishing(mCells_ev_DCAL_Thres);

  if (mTaskSettings.mHasHistosCalib) {
    mCells_ev_sm_good = new TH2D("ncellsGoodPerEventSupermodule", "# of good Cells per Events vs supermodule ID", mTaskSettings.mMultiplicityRangeSM, 0, mTaskSettings.mMultiplicityRangeSM, 20, -0.5, 19.5);
    mCells_ev_sm_good->GetYaxis()->SetTitle("Supermodule");
    mCells_ev_sm_good->GetXaxis()->SetTitle("Good cells/Event");
    mCells_ev_sm_good->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_sm_good);

    mCells_ev_sm_bad = new TH2D("ncellsBadPerEventSupermodule", "# of bad Cells per Events vs supermodule ID", mTaskSettings.mMultiplicityRangeSM, 0, mTaskSettings.mMultiplicityRangeSM, 20, -0.5, 19.5);
    mCells_ev_sm_bad->GetYaxis()->SetTitle("Supermodule");
    mCells_ev_sm_bad->GetXaxis()->SetTitle("Bad cells/Event");
    mCells_ev_sm_bad->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_sm_bad);

    mCells_ev_good = new TH1D("ncellsGoodPerEventTot", "# good of Cells per event", mTaskSettings.mMultiplicityRange, 0, mTaskSettings.mMultiplicityRange);
    mCells_ev_good->GetXaxis()->SetTitle("Good cells/Event");
    mCells_ev_good->GetYaxis()->SetTitle("Events");
    mCells_ev_good->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_good);

    mCells_ev_bad = new TH1D("ncellsBadPerEventTot", "# bad of Cells per event", mTaskSettings.mMultiplicityRange, 0, mTaskSettings.mMultiplicityRange);
    mCells_ev_bad->GetXaxis()->SetTitle("Bad cells/Event");
    mCells_ev_bad->GetYaxis()->SetTitle("Events");
    mCells_ev_bad->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_bad);

    mCells_ev_EMCAL_good = new TH1D("ncellsGoodPerEventEMCALTot", "# of good Cells per events in EMCAL", mTaskSettings.mMultiplicityRangeDetector, 0, mTaskSettings.mMultiplicityRangeDetector);
    mCells_ev_EMCAL_good->GetXaxis()->SetTitle("Good cells/Event");
    mCells_ev_EMCAL_good->GetYaxis()->SetTitle("Events");
    mCells_ev_EMCAL_good->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_EMCAL_good);

    mCells_ev_EMCAL_bad = new TH1D("ncellsBadPerEventEMCALTot", "# of bad Cells per events in EMCAL", mTaskSettings.mMultiplicityRangeDetector, 0, mTaskSettings.mMultiplicityRangeDetector);
    mCells_ev_EMCAL_bad->GetXaxis()->SetTitle("Bad cells/Event");
    mCells_ev_EMCAL_bad->GetYaxis()->SetTitle("Events");
    mCells_ev_EMCAL_bad->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_EMCAL_bad);

    mCells_ev_DCAL_good = new TH1D("ncellsGoodPerEventDCALTot", "# of good Cells per event in DCAL", mTaskSettings.mMultiplicityRangeDetector, 0, mTaskSettings.mMultiplicityRangeDetector);
    mCells_ev_DCAL_good->GetXaxis()->SetTitle("Good cells/Event");
    mCells_ev_DCAL_good->GetYaxis()->SetTitle("Events");
    mCells_ev_DCAL_good->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_DCAL_good);

    mCells_ev_DCAL_bad = new TH1D("ncellsBaddPerEventDCALTot", "# of bad Cells per event in DCAL", mTaskSettings.mMultiplicityRangeDetector, 0, mTaskSettings.mMultiplicityRangeDetector);
    mCells_ev_DCAL_bad->GetXaxis()->SetTitle("Badd cells/Event");
    mCells_ev_DCAL_bad->GetYaxis()->SetTitle("Events");
    mCells_ev_DCAL_bad->SetStats(false);
    getObjectsManager()->startPublishing(mCells_ev_DCAL_bad);

    mFracGoodCellsEvent = new TH2D("fractionGoodCellsEvent", "Fraction of good cells / event", 3, -0.5, 2.5, 11, 0., 1.1);
    mFracGoodCellsEvent->GetXaxis()->SetBinLabel(1, "All");
    mFracGoodCellsEvent->GetXaxis()->SetBinLabel(2, "EMCAL");
    mFracGoodCellsEvent->GetXaxis()->SetBinLabel(3, "DCAL");
    mFracGoodCellsEvent->GetYaxis()->SetTitle("Fraction good");
    mFracGoodCellsEvent->SetStats(false);
    getObjectsManager()->startPublishing(mFracGoodCellsEvent);

    mFracGoodCellsSM = new TH2D("fractionGoodCellsSupermodule", "Fraction of good cells / supermodule", 20, -0.5, 19.5, 11, 0., 1.1);
    mFracGoodCellsSM->GetXaxis()->SetTitle("Supermodule ID");
    mFracGoodCellsSM->GetYaxis()->SetTitle("Fraction good");
    mFracGoodCellsSM->SetStats(false);
    getObjectsManager()->startPublishing(mFracGoodCellsSM);
  }

  mTotalEnergy = new TH1D("totalEnergy", "Total energy / event", mTaskSettings.mTotalEnergyRange, 0., mTaskSettings.mTotalEnergyRange);
  mTotalEnergy->GetXaxis()->SetTitle("E_{tot} (GeV)");
  mTotalEnergy->GetYaxis()->SetTitle("Number of events");
  mTotalEnergy->SetStats(false);
  getObjectsManager()->startPublishing(mTotalEnergy);

  mTotalEnergyCorr = new TH2D("totalEnergyCorr", "Total energy EMCAL vs. DCAL / event", mTaskSettings.mTotalEnergyRangeDetector, 0., mTaskSettings.mTotalEnergyRangeDetector, mTaskSettings.mTotalEnergyRangeDetector, 0., mTaskSettings.mTotalEnergyRangeDetector);
  mTotalEnergyCorr->GetXaxis()->SetTitle("EMCAL E_{tot} (GeV)");
  mTotalEnergyCorr->GetYaxis()->SetTitle("DCAL E_{tot} (GeV)");
  getObjectsManager()->startPublishing(mTotalEnergyCorr);

  mTotalEnergySM = new TH2D("totalEnergySupermodule", "Total energy in supermodule / event", mTaskSettings.mTotalEnergyRangeSM, 0., mTaskSettings.mTotalEnergyRangeSM, 20, -0.5, 19.5);
  mTotalEnergySM->GetXaxis()->SetTitle("E_{tot} (GeV)");
  mTotalEnergySM->GetYaxis()->SetTitle("SupermoduleID");
  getObjectsManager()->startPublishing(mTotalEnergySM);
}

void CellTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void CellTask::startOfCycle()
{
  mTimeFramesPerCycles = 0;
  ILOG(Debug, Support) << "startOfCycle" << ENDM;
}

void CellTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mTFPerCycles->Fill(1); // number of timeframe process per cycle
  mTimeFramesPerCycles++;
  // check if we have payoad
  using MaskType_t = o2::emcal::BadChannelMap::MaskType_t;

  // Handling of inputs from multiple subevents (multiple FLPs)
  // Build maps of trigger records and cells according to the subspecification
  // and combine trigger records from different maps into a single map of range
  // references and subspecifications
  std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::Cell>> cellSubEvents;
  std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::TriggerRecord>> triggerRecordSubevents;

  loadCalibrationObjects(ctx);

  auto posCells = ctx.inputs().getPos("emcal-cells"),
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

  //  ILOG(Info, Support) <<"Received " << cellcontainer.size() << " cells " << ENDM;
  int eventcounter = 0;
  int eventcounterCALIB = 0;
  int eventcounterPHYS = 0;
  std::array<int, 20> numCellsSM;
  std::array<int, 20> numCellsSM_Thres;
  std::array<int, 20> numCellsGood;
  std::array<int, 20> numCellsBad;
  std::array<double, 20> totalEnergies;
  std::fill(numCellsSM.begin(), numCellsSM.end(), 0);
  std::fill(numCellsSM_Thres.begin(), numCellsSM_Thres.end(), 0);
  for (auto trg : combinedEvents) {
    if (!trg.getNumberOfObjects()) {
      continue;
    }
    ILOG(Debug, Support) << "Next event " << eventcounter << " has " << trg.getNumberOfObjects() << " cells from " << trg.getNumberOfSubevents() << " subevent(s)" << ENDM;

    // trigger type
    auto triggertype = trg.mTriggerType;
    bool isPhysTrigger = mIgnoreTriggerTypes || (triggertype & o2::trigger::PhT),
         isCalibTrigger = (!mIgnoreTriggerTypes) && (triggertype & o2::trigger::Cal);
    std::string trgClass;
    if (isPhysTrigger) {
      trgClass = "PHYS";
      eventcounterPHYS++;
      if (mBCCounterPHYS) {
        mBCCounterPHYS->Fill(trg.mInteractionRecord.bc);
      }
    } else if (isCalibTrigger) {
      trgClass = "CAL";
      eventcounterCALIB++;
      if (mBCCounterCalib) {
        mBCCounterCalib->Fill(trg.mInteractionRecord.bc);
      }
    } else {
      ILOG(Error, Support) << " Unmonitored trigger class requested " << ENDM;
      continue;
    }
    auto bcphase = trg.mInteractionRecord.bc % 4; // to be fixed:4 histos for EMCAL, 4 histos for DCAL
    // force BC phase for LED triggers to be 0
    if (isCalibTrigger) {
      bcphase = 0;
    }
    auto histos = mHistogramContainer[trgClass];
    std::fill(numCellsSM.begin(), numCellsSM.end(), 0);
    std::fill(numCellsSM_Thres.begin(), numCellsSM_Thres.end(), 0);
    std::fill(numCellsGood.begin(), numCellsGood.end(), 0);
    std::fill(numCellsBad.begin(), numCellsBad.end(), 0);
    std::fill(totalEnergies.begin(), totalEnergies.end(), 0.);

    // iterate over subevents
    for (auto& subev : trg.mSubevents) {
      auto cellsSubspec = cellSubEvents.find(subev.mSpecification);
      if (cellsSubspec == cellSubEvents.end()) {
        ILOG(Error, Support) << "No cell data found for subspecification " << subev.mSpecification << ENDM;
      } else {
        ILOG(Debug, Support) << subev.mCellRange.getEntries() << " cells in subevent from equipment " << subev.mSpecification << ENDM;
        gsl::span<const o2::emcal::Cell> eventcells(cellsSubspec->second.data() + subev.mCellRange.getFirstEntry(), subev.mCellRange.getEntries());
        for (auto cell : eventcells) {
          if (cell.getLEDMon()) {
            // Drop LEDMON cells
            continue;
          }
          // int index = cell.getHighGain() ? 0 : (cell.getLowGain() ? 1 : -1);
          // int index = cell.getHighGain() ? 0 : 1;
          auto timeoffset = mTimeCalib ? mTimeCalib->getTimeCalibParam(cell.getTower(), cell.getLowGain()) : 0.;
          auto energycalib = mEnergyCalib ? mEnergyCalib->getGainCalibFactors(cell.getTower()) : 1.;
          bool goodcell = true;
          if (mBadChannelMap) {
            goodcell = mBadChannelMap->getChannelStatus(cell.getTower()) == MaskType_t::GOOD_CELL;
          }
          histos.fillHistograms(cell, goodcell, timeoffset, energycalib, bcphase);
          if (isPhysTrigger) {
            auto [sm, mod, iphi, ieta] = mGeometry->GetCellIndex(cell.getTower());
            numCellsSM[sm]++;
            if (cell.getEnergy() > mTaskSettings.mThresholdPHYS) {
              numCellsSM_Thres[sm]++;
            }
            if (goodcell) {
              numCellsGood[sm]++;
              auto cellenergy = cell.getAmplitude() * energycalib;
              auto celltime = cell.getTimeStamp() - timeoffset;
              if (cellenergy > mTaskSettings.mThresholdTotalEnergy) {
                if (std::abs(celltime) < mTaskSettings.mMaxTimeTotalEnergy) {
                  totalEnergies[sm] += cellenergy;
                }
              }
            } else {
              numCellsBad[sm]++;
            }
          }
        }
      }
    }
    histos.countEvent();

    if (isPhysTrigger) {
      auto maxSM = std::max_element(numCellsSM.begin(), numCellsSM.end());
      auto indexMaxSM = maxSM - numCellsSM.begin();
      mCellsMaxSM->Fill(indexMaxSM);
      // fill histo 1)
      int mCell_all = 0, mCell_EMCAL = 0, mCell_DCAL = 0;
      int mCell_all_Thres = 0, mCell_EMCAL_Thres = 0, mCell_DCAL_Thres = 0;
      // make statistics good cells
      int nGoodAll = 0, nGoodEMCAL = 0, nGoodDCAL = 0, nBadAll = 0, nBadEMCAL = 0, nBadDCAL = 0;
      for (int ism = 0; ism < 20; ism++) {
        mCells_ev_sm->Fill(numCellsSM[ism], ism);          // for experts
        mCells_ev_smThr->Fill(numCellsSM_Thres[ism], ism); // for experts

        mCell_all += numCellsSM[ism];
        mCell_all_Thres += numCellsSM_Thres[ism];
        nGoodAll += numCellsGood[ism];
        nBadAll += numCellsBad[ism];
        if (ism < 12) {
          mCell_EMCAL += numCellsSM[ism];
          mCell_EMCAL_Thres += numCellsSM_Thres[ism];
          nGoodEMCAL += numCellsGood[ism];
          nBadEMCAL += numCellsBad[ism];
        } else {
          mCell_DCAL += numCellsSM[ism];
          mCell_DCAL_Thres += numCellsSM_Thres[ism];
          nGoodDCAL += numCellsGood[ism];
          nBadDCAL += numCellsBad[ism];
        }
        if (mTaskSettings.mHasHistosCalib) {
          mCells_ev_sm_good->Fill(numCellsGood[ism], ism);
          mCells_ev_sm_bad->Fill(numCellsBad[ism], ism);
          if (numCellsGood[ism] + numCellsBad[ism]) {
            mFracGoodCellsSM->Fill(ism, static_cast<double>(numCellsGood[ism]) / static_cast<double>(numCellsGood[ism] + numCellsBad[ism]));
          }
        }
      }
      mCells_ev->Fill(mCell_all);
      mCells_ev_EMCAL->Fill(mCell_EMCAL);
      mCells_ev_DCAL->Fill(mCell_DCAL);

      mCells_ev_Thres->Fill(mCell_all_Thres);
      mCells_ev_EMCAL_Thres->Fill(mCell_EMCAL_Thres);
      mCells_ev_DCAL_Thres->Fill(mCell_DCAL_Thres);

      if (mTaskSettings.mHasHistosCalib) {
        mCells_ev_good->Fill(nGoodAll);
        mCells_ev_EMCAL_good->Fill(nGoodEMCAL);
        mCells_ev_DCAL_good->Fill(nGoodDCAL);
        mCells_ev_bad->Fill(nBadAll);
        mCells_ev_EMCAL_bad->Fill(nBadEMCAL);
        mCells_ev_DCAL_bad->Fill(nBadDCAL);
        if (nGoodAll + nBadAll) {
          mFracGoodCellsEvent->Fill(0., static_cast<double>(nGoodAll) / static_cast<double>(nGoodAll + nBadAll));
        }
        if (nGoodEMCAL + nBadEMCAL) {
          mFracGoodCellsEvent->Fill(1., static_cast<double>(nGoodEMCAL) / static_cast<double>(nGoodEMCAL + nBadEMCAL));
        }
        if (nGoodDCAL + nBadDCAL) {
          mFracGoodCellsEvent->Fill(2., static_cast<double>(nGoodDCAL) / static_cast<double>(nGoodDCAL + nBadDCAL));
        }
      }

      double totalEnergySum = 0., totalEnergyEMCAL = 0., totalEnergyDCAL = 0.;
      for (std::size_t ism = 0; ism < 20; ism++) {
        mTotalEnergySM->Fill(totalEnergies[ism], ism);
        totalEnergySum += totalEnergies[ism];
        if (ism < 12) {
          totalEnergyEMCAL += totalEnergies[ism];
        } else {
          totalEnergyDCAL += totalEnergies[ism];
        }
      }
      mTotalEnergy->Fill(totalEnergySum);
      mTotalEnergyCorr->Fill(totalEnergyEMCAL, totalEnergyDCAL);
    }

    eventcounter++;
  }
  mEvCounterTF->Fill(eventcounter);
  mEvCounterTFPHYS->Fill(eventcounterPHYS);
  mEvCounterTFCALIB->Fill(eventcounterCALIB);
  // event counter per TimeFrame  (range 0-100) for the moment (parameter)
}

void CellTask::endOfCycle()
{
  mTFPerCyclesTOT->Fill(mTimeFramesPerCycles); // do not reset this histo
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void CellTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void CellTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Support) << "Resetting the histogram" << ENDM;
  for (auto cont : mHistogramContainer) {
    cont.second.reset();
  }
  auto resetOptional = [](auto* hist) {
    if (hist) {
      hist->Reset();
    }
  };
  resetOptional(mEvCounterTF);
  resetOptional(mEvCounterTFPHYS);
  resetOptional(mEvCounterTFCALIB);
  resetOptional(mTFPerCycles);
  resetOptional(mTFPerCyclesTOT);
  resetOptional(mBCCounterPHYS);
  resetOptional(mBCCounterCalib);
  resetOptional(mCellsMaxSM);
  resetOptional(mCells_ev_sm);
  resetOptional(mCells_ev_sm_good);
  resetOptional(mCells_ev_sm_bad);
  resetOptional(mCells_ev_smThr);
  resetOptional(mCells_ev);
  resetOptional(mCells_ev_good);
  resetOptional(mCells_ev_bad);
  resetOptional(mCells_ev_Thres);
  resetOptional(mCells_ev_EMCAL);
  resetOptional(mCells_ev_EMCAL_good);
  resetOptional(mCells_ev_EMCAL_bad);
  resetOptional(mCells_ev_EMCAL_Thres);
  resetOptional(mCells_ev_DCAL);
  resetOptional(mCells_ev_DCAL_good);
  resetOptional(mCells_ev_DCAL_bad);
  resetOptional(mCells_ev_DCAL_Thres);
  resetOptional(mFracGoodCellsEvent);
  resetOptional(mFracGoodCellsSM);
  resetOptional(mTotalEnergy);
  resetOptional(mTotalEnergyCorr);
  resetOptional(mTotalEnergySM);
}

void CellTask::finaliseCCDB(o2::framework::ConcreteDataMatcher& matcher, void* obj)
{
  if (matcher == o2::framework::ConcreteDataMatcher("EMC", "BADCHANNELMAP", 0)) {
    mBadChannelMap = reinterpret_cast<const o2::emcal::BadChannelMap*>(obj);
    if (mBadChannelMap) {
      ILOG(Info, Support) << "Updated EMCAL bad channel map " << ENDM;
    }
  }
  if (matcher == o2::framework::ConcreteDataMatcher("EMC", "TIMECALIBPARAM", 0)) {
    mTimeCalib = reinterpret_cast<const o2::emcal::TimeCalibrationParams*>(obj);
    if (mTimeCalib) {
      ILOG(Info, Support) << "Updated EMCAL time calibration" << ENDM;
    }
  }
  if (matcher == o2::framework::ConcreteDataMatcher("EMC", "GAINCALIBPARAM", 0)) {
    mEnergyCalib = reinterpret_cast<const o2::emcal::GainCalibrationFactors*>(obj);
    if (mEnergyCalib) {
      ILOG(Info, Support) << "Update EMCAL gain calibration" << ENDM;
    }
  }
}

void CellTask::loadCalibrationObjects(o2::framework::ProcessingContext& ctx)
{
  if (mTaskSettings.mHasHistosCalib) {
    ctx.inputs().get<o2::emcal::BadChannelMap*>("badchannelmap");
    ctx.inputs().get<o2::emcal::TimeCalibrationParams*>("timecalib");
  }
  if (mTaskSettings.mCalibrateEnergy) {
    ctx.inputs().get<o2::emcal::GainCalibrationFactors*>("energycalib");
  }
}

void CellTask::initDefaultMultiplicityRanges()
{
  if (!mTaskSettings.mMultiplicityRange) {
    mTaskSettings.mMultiplicityRange = mTaskSettings.mIsHighMultiplicity ? 10000 : 1000;
  }
  if (!mTaskSettings.mMultiplicityRangeDetector) {
    mTaskSettings.mMultiplicityRangeDetector = mTaskSettings.mIsHighMultiplicity ? 4000 : 300;
  }
  if (!mTaskSettings.mMultiplicityRangeThreshold) {
    mTaskSettings.mMultiplicityRangeThreshold = mTaskSettings.mIsHighMultiplicity ? 1000 : 100;
  }
  if (!mTaskSettings.mMultiplicityRangeSM) {
    mTaskSettings.mMultiplicityRangeSM = mTaskSettings.mIsHighMultiplicity ? 1000 : 100;
  }
  if (!mTaskSettings.mMultiplicityRangeSMThreshold) {
    mTaskSettings.mMultiplicityRangeSMThreshold = mTaskSettings.mIsHighMultiplicity ? 500 : 20;
  }
  if (std::abs(mTaskSettings.mTotalEnergyRange) < 1e-5) {
    mTaskSettings.mTotalEnergyRange = mTaskSettings.mIsHighMultiplicity ? 4000 : 200;
  }
  if (std::abs(mTaskSettings.mTotalEnergyRangeDetector) < 1e-5) {
    mTaskSettings.mTotalEnergyRangeDetector = mTaskSettings.mIsHighMultiplicity ? 4000 : 200;
  }
  if (std::abs(mTaskSettings.mTotalEnergyRangeSM) < 1e-5) {
    mTaskSettings.mTotalEnergyRangeSM = mTaskSettings.mIsHighMultiplicity ? 500 : 50;
  }
}

void CellTask::parseMultiplicityRanges()
{
  if (hasConfigValue("MultiplicityRange")) {
    mTaskSettings.mMultiplicityRange = std::stoi(getConfigValue("MultiplicityRange"));
  }
  if (hasConfigValue("MultiplicityRangeDetector")) {
    mTaskSettings.mMultiplicityRangeDetector = std::stoi(getConfigValue("MultiplicityRangeDetector"));
  }
  if (hasConfigValue("MultiplicityRangeThreshold")) {
    mTaskSettings.mMultiplicityRangeThreshold = std::stoi(getConfigValue("MultiplicityRangeThreshold"));
  }
  if (hasConfigValue("MultiplicityRangeSM")) {
    mTaskSettings.mMultiplicityRangeSM = std::stoi(getConfigValue("MultiplicityRangeSM"));
  }
  if (hasConfigValue("MultiplicityRangeSMThreshold")) {
    mTaskSettings.mMultiplicityRangeSMThreshold = std::stoi(getConfigValue("MultiplicityRangeSMThreshold"));
  }
  if (hasConfigValue("TotalEnergyRange")) {
    mTaskSettings.mTotalEnergyRange = std::stod(getConfigValue("TotalEnergyRange"));
  }
  if (hasConfigValue("TotalEnergyRangeDetector")) {
    mTaskSettings.mTotalEnergyRangeDetector = std::stod(getConfigValue("TotalEnergyRangeDetector"));
  }
  if (hasConfigValue("TotalEnergyRangeSM")) {
    mTaskSettings.mTotalEnergyRangeSM = std::stod(getConfigValue("TotalEnergyRangeSM"));
  }
  if (hasConfigValue("TotalEnergyMaxCellTime")) {
    mTaskSettings.mMaxTimeTotalEnergy = std::stod(getConfigValue("TotalEnergyMaxCellTime"));
  }
}

std::vector<CellTask::CombinedEvent> CellTask::buildCombinedEvents(const std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::TriggerRecord>>& triggerrecords) const
{
  std::vector<CellTask::CombinedEvent> events;

  // Search interaction records from all subevents
  std::set<o2::InteractionRecord> allInteractions;
  for (auto& [subspecification, trgrec] : triggerrecords) {
    for (auto rec : trgrec) {
      auto eventIR = rec.getBCData();
      if (allInteractions.find(eventIR) == allInteractions.end()) {
        allInteractions.insert(eventIR);
      }
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
    if (hasSubevent) {
      events.emplace_back(nextevent);
    }
  }
  return events;
}

bool CellTask::hasConfigValue(const std::string_view key)
{
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    return true;
  }
  return false;
}

std::string CellTask::getConfigValue(const std::string_view key)
{
  std::string result;
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    result = param->second;
  }
  return result;
}

std::string CellTask::getConfigValueLower(const std::string_view key)
{
  auto input = getConfigValue(key);
  std::string result;
  if (input.length()) {
    result = boost::algorithm::to_lower_copy(input);
  }
  return result;
}

void CellTask::CellHistograms::initForTrigger(const std::string trigger, const TaskSettings& settings)
{ // hasAmpVsCellID, bool hasTimeVsCellID, bool hasHistosCalib2D)
  if (trigger == "PYHS") {
    mCellThreshold = settings.mThresholdPHYS;
    mAmplitudeThresholdTime = settings.mAmpThresholdTimePhys;
  } else {
    mCellThreshold = settings.mThresholdCAL;
    mAmplitudeThresholdTime = settings.mAmpThresholdTimeCalib;
  }
  mThresholdAvTime = settings.mThresholdAvTime;
  mThresholdAvEnergy = settings.mThresholdAvEnergy;
  auto histBuilder1D = [trigger](const std::string name, const std::string title, int nbinsx, double xmin, double xmax) -> TH1* {
    std::string histname = name + "_" + trigger,
                histtitle = title + " " + trigger;
    return new TH1D(histname.data(), histtitle.data(), nbinsx, xmin, xmax);
  };
  auto histBuilder2D = [trigger](const std::string_view name, const std::string_view title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double ymax, bool profile) -> TH2* {
    std::string histname = std::string(name.data()) + "_" + trigger,
                histtitle = std::string(title.data()) + " " + trigger;
    if (profile) {
      return new TProfile2D(histname.data(), histtitle.data(), nbinsx, xmin, xmax, nbinsy, ymin, ymax);
    }
    return new TH2D(histname.data(), histtitle.data(), nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  };

  std::map<std::string, double> maxAmps = { { "PHYS", 50. }, { "CAL", 10. } };
  double maxAmp = maxAmps[trigger];

  bool isPhysTrigger = trigger == "PHYS";
  if (isPhysTrigger) {
    if (settings.mHasAmpVsCellID) {
      mCellAmplitude = histBuilder2D("cellAmplitudeHG", "Cell Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);
      mCellAmplitude->SetStats(false);
      // mCellAmplitude[1] = histBuilder2D("cellAmplitudeLG", "Cell Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      if (settings.mHasHistosCalib) {
        mCellAmplitudeCalib = histBuilder2D("cellAmplitudeHGCalib", "Cell Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);
        mCellAmplitudeCalib->SetStats(false);
        // mCellAmplitudeCalib[1] = histBuilder2D("cellAmplitudeLGCalib", "Cell Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      }
    }
    if (settings.mHasTimeVsCellID) {
      mCellTime = histBuilder2D("cellTimeHG", "Cell Time (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false); //
      mCellTime->SetStats(false);
      // mCellTime[1] = histBuilder2D("cellTimeLG", "Cell Time (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      if (settings.mHasHistosCalib) {
        mCellTimeCalib = histBuilder2D("cellTimeHGCalib", "Cell Time Calib (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
        mCellTimeCalib->SetStats(false);
        // mCellTimeCalib[1] = histBuilder2D("cellTimeLGCalib", "Cell Time Calib (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      }
    }

    if (settings.mHasHistosCalib) {
      mCellAmpSupermoduleCalib = histBuilder2D("cellAmplitudeSupermoduleCalib", "Cell amplitude (Calib) vs. supermodule ID ", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
      mCellAmpSupermoduleCalib->SetStats(false);
      mCellTimeSupermoduleCalib = histBuilder2D("cellTimeSupermoduleCalib", "Cell Time (Calib) vs. supermodule ID (High gain)", 600, -400, 800, 20, -0.5, 19.5, false);
      mCellTimeSupermoduleCalib->SetStats(false);
      mCellAmpSupermoduleBad = histBuilder2D("cellAmplitudeSupermoduleBad", "Cell amplitude (bad cells) vs. supermodule ID", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
      mCellAmpSupermoduleBad->SetStats(false);
      mCellOccupancyGood = histBuilder2D("cellOccupancyGood", "Cell occupancy good cells", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
      mCellOccupancyGood->SetStats(false);
      mCellOccupancyBad = histBuilder2D("cellOccupancyBad", "Cell occupancy bad cells", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
      mCellOccupancyBad->SetStats(false);

      mCellAmplitudeCalib_tot = histBuilder1D("cellAmplitudeCalib", "Cell amplitude Calib in EMCAL,DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
      mCellAmplitudeCalib_EMCAL = histBuilder1D("cellAmplitudeCalib_EMCAL", "Cell amplitude Calib in EMCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
      mCellAmplitudeCalib_DCAL = histBuilder1D("cellAmplitudeCalib_DCAL", "Cell amplitude Calib in DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);

      mCellTimeSupermoduleCalib_tot = histBuilder1D("cellTimeCalib", "Cell Time Calib EMCAL,DCAL", 600, -400, 800);
      mCellTimeSupermoduleCalib_EMCAL = histBuilder1D("cellTimeCalib_EMCAL", "Cell Time Calib EMCAL", 600, -400, 800);
      mCellTimeSupermoduleCalib_DCAL = histBuilder1D("cellTimeCalib_DCAL", "Cell Time Calib DCAL", 600, -400, 800);

      mCellAmplitudeTimeCalib = histBuilder2D("cellAmplitudeTimeCalib", "Cell amplitude vs. time (calibrated); E (GeV); t (ns)", 500, 0., 50., 800, -400., 400., false);
    }
  }
  mCellAmpSupermodule = histBuilder2D("cellAmplitudeSupermodule", "Cell amplitude vs. supermodule ID ", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
  mCellAmpSupermodule->SetStats(false);
  mCellTimeSupermodule = histBuilder2D("cellTimeSupermodule", "Cell Time vs. supermodule ID ", 600, -400, 800, 20, -0.5, 19.5, false);
  mCellTimeSupermodule->SetStats(false);

  mCellOccupancy = histBuilder2D("cellOccupancyEMC", "Cell Occupancy EMCAL", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mCellOccupancy->SetStats(false);
  mCellOccupancyThr = histBuilder2D("cellOccupancyEMCwThr", Form("Cell Occupancy EMCAL,DCAL with E>%.1f GeV/c", mCellThreshold), 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mCellOccupancyThr->SetStats(false);
  mCellOccupancyThrBelow = histBuilder2D("cellOccupancyEMCwThrBelow", Form("Cell Occupancy EMCAL,DCAL with E<%.1f GeV/c", mCellThreshold), 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mCellOccupancyThrBelow->SetStats(false);

  mAverageCellEnergy = histBuilder2D("averageCellEnergy", "Average cell energy", 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mAverageCellEnergy->GetXaxis()->SetTitle("col");
  mAverageCellEnergy->GetYaxis()->SetTitle("row");
  mAverageCellEnergy->SetStats(false);

  mAverageCellTime = histBuilder2D("averageCellTime", "Average cell time", 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mAverageCellTime->GetXaxis()->SetTitle("col");
  mAverageCellTime->GetYaxis()->SetTitle("row");
  mAverageCellTime->SetStats(false);

  mAverageCellEnergyConstrained = histBuilder2D("averageCellEnergyConstrained", Form("Average cell energy (E > %.1f GeV/c)", settings.mThresholdAvEnergy), 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mAverageCellEnergyConstrained->GetXaxis()->SetTitle("col");
  mAverageCellEnergyConstrained->GetYaxis()->SetTitle("row");
  mAverageCellEnergyConstrained->SetStats(false);

  mAverageCellTimeConstrained = histBuilder2D("averageCellTimeConstrained", Form("Average cell time (|t| < %.1f ns)", settings.mThresholdAvTime), 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mAverageCellTimeConstrained->GetXaxis()->SetTitle("col");
  mAverageCellTimeConstrained->GetYaxis()->SetTitle("row");
  mAverageCellTimeConstrained->SetStats(false);

  mIntegratedOccupancy = histBuilder2D("cellOccupancyInt", "Cell Occupancy Integrated", 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mIntegratedOccupancy->GetXaxis()->SetTitle("col");
  mIntegratedOccupancy->GetYaxis()->SetTitle("row");
  mIntegratedOccupancy->SetStats(false);
  // 1D histograms for showing the integrated spectrum

  mCellTimeSupermodule_tot = histBuilder1D("cellTime", "Cell Time EMCAL,DCAL", 600, -400, 800);
  mCellTimeSupermoduleEMCAL = histBuilder1D("cellTimeEMCAL", "Cell Time EMCAL", 600, -400, 800);
  mCellTimeSupermoduleDCAL = histBuilder1D("cellTimeDCAL", "Cell Time DCAL", 600, -400, 800);

  mCellTimeSupermoduleEMCAL_Gain[0] = histBuilder1D("cellTimeEMCAL_highGain", "Cell Time EMCAL highGain", 600, -400, 800);
  mCellTimeSupermoduleEMCAL_Gain[1] = histBuilder1D("cellTimeEMCAL_lowGain", "Cell Time EMCAL lowGain", 600, -400, 800);
  mCellTimeSupermoduleDCAL_Gain[0] = histBuilder1D("cellTimeDCAL_highGain", "Cell Time DCAL highGain", 600, -400, 800);
  mCellTimeSupermoduleDCAL_Gain[1] = histBuilder1D("cellTimeDCAL_lowGain", "Cell Time DCAL lowGain", 600, -400, 800);
  mCellAmplitude_tot = histBuilder1D("cellAmplitude", "Cell amplitude in EMCAL,DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mCellAmplitudeEMCAL = histBuilder1D("cellAmplitudeEMCAL", "Cell amplitude in EMCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mCellAmplitudeDCAL = histBuilder1D("cellAmplitudeDCAL", "Cell amplitude in DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);

  mCellAmplitudeTime = histBuilder2D("cellAmplitudeTime", "Cell amplitude vs. time; E (GeV); t (ns)", 500, 0., 50., 800, -400., 400., false);

  mnumberEvents = histBuilder1D("NumberOfEvents", "Number Of Events", 1, 0.5, 1.5);
  //
  std::fill(mCellTimeBC.begin(), mCellTimeBC.end(), nullptr);
  if (isPhysTrigger) {
    // Phys. trigger: monitor all bunch crossings
    for (auto bcID = 0; bcID < 4; bcID++) {
      mCellTimeBC[bcID] = histBuilder1D(Form("cellTimeBC%d", bcID), Form("Cell Time BC%d", bcID), 600, -400, 800);
    }
  } else {
    // Calib trigger: Only bc0;
    mCellTimeBC[0] = histBuilder1D("cellTimeBC0", "Cell Time BC0", 600, -400, 800);
  }
}

void CellTask::CellHistograms::fillHistograms(const o2::emcal::Cell& cell, bool goodCell, double timecalib, double energycalib, int bcphase)
{
  auto fillOptional1D = [](TH1* hist, double x, double weight = 1.) {
    if (hist) {
      hist->Fill(x, weight);
    }
  };
  auto fillOptional2D = [](TH2* hist, double x, double y, double weight = 1.) {
    if (hist) {
      hist->Fill(x, y, weight);
    }
  };

  double energy = cell.getEnergy() * energycalib;

  fillOptional2D(mCellAmplitude, energy, cell.getTower());
  // fillOptional2D(mCellAmplitude[index], cell.getEnergy(), cell.getTower());

  if (goodCell) {
    fillOptional2D(mCellAmplitudeCalib, energy, cell.getTower());
    fillOptional2D(mCellTimeCalib, cell.getTimeStamp() - timecalib, cell.getTower());
    // fillOptional2D(mCellAmplitudeCalib[index], cell.getEnergy(), cell.getTower());
    // fillOptional2D(mCellTimeCalib[index], cell.getTimeStamp() - timeoffset, cell.getTower());
  }
  fillOptional2D(mCellTime, cell.getTimeStamp(), cell.getTower());
  // fillOptional2D(mCellTime[index], cell.getTimeStamp(), cell.getTower());

  try {
    auto [row, col] = mGeometry->GlobalRowColFromIndex(cell.getTower());
    if (cell.getEnergy() > 0) {
      fillOptional2D(mCellOccupancy, col, row);
    }
    if (cell.getEnergy() > mCellThreshold) {
      fillOptional2D(mCellOccupancyThr, col, row);
    } else {
      fillOptional2D(mCellOccupancyThrBelow, col, row);
    }
    if (goodCell) {
      fillOptional2D(mCellOccupancyGood, col, row);
      fillOptional2D(mAverageCellEnergy, col, row, energy);
      if (energy > mThresholdAvEnergy) {
        fillOptional2D(mAverageCellEnergyConstrained, col, row, energy);
      };
      fillOptional2D(mAverageCellTime, col, row, cell.getTimeStamp() - timecalib);
      if (std::abs(cell.getTimeStamp()) < mThresholdAvTime) {
        fillOptional2D(mAverageCellTimeConstrained, col, row, cell.getTimeStamp() - timecalib);
      }
    } else {
      fillOptional2D(mCellOccupancyBad, col, row);
    }

    fillOptional2D(mIntegratedOccupancy, col, row, energy);

  } catch (o2::emcal::InvalidCellIDException& e) {
    ILOG(Error, Support) << "Invalid cell ID: " << e.getCellID() << ENDM;
  };

  try {
    auto cellindices = mGeometry->GetCellIndex(cell.getTower());
    auto supermoduleID = std::get<0>(cellindices);
    fillOptional2D(mCellAmpSupermodule, energy, supermoduleID);
    fillOptional2D(mCellAmplitudeTime, energy, cell.getTimeStamp());
    if (cell.getEnergy() > mAmplitudeThresholdTime) {
      fillOptional2D(mCellTimeSupermodule, cell.getTimeStamp(), supermoduleID);
    }
    if (goodCell) {
      fillOptional2D(mCellAmpSupermoduleCalib, energy, supermoduleID);
      if (energy > mAmplitudeThresholdTime) {
        fillOptional2D(mCellTimeSupermoduleCalib, cell.getTimeStamp() - timecalib, supermoduleID);
        fillOptional2D(mCellAmplitudeTimeCalib, energy, cell.getTimeStamp() - timecalib);
      }
    } else {
      fillOptional2D(mCellAmpSupermoduleBad, energy, supermoduleID);
    }
    fillOptional1D(mCellAmplitude_tot, energy);
    if (goodCell) {
      fillOptional1D(mCellAmplitudeCalib_tot, energy); // EMCAL+DCAL Calib, shifter
    }
    if (energy > mAmplitudeThresholdTime) {
      fillOptional1D(mCellTimeSupermodule_tot, cell.getTimeStamp()); // EMCAL+DCAL shifter
      if (goodCell) {
        fillOptional1D(mCellTimeSupermoduleCalib_tot, cell.getTimeStamp() - timecalib); // EMCAL+DCAL Calib shifter
      }
    }
    // check Gain
    int index = cell.getHighGain() ? 0 : 1; //(0=highGain, 1 = lowGain)
    if (supermoduleID < 12) {               // EMCAL
      if (energy > mAmplitudeThresholdTime) {
        fillOptional1D(mCellTimeSupermoduleEMCAL, cell.getTimeStamp());
        if (goodCell) {
          fillOptional1D(mCellTimeSupermoduleCalib_EMCAL, cell.getTimeStamp() - timecalib);
        }
        fillOptional1D(mCellTimeSupermoduleEMCAL_Gain[index], cell.getTimeStamp());
      }
      fillOptional1D(mCellAmplitudeEMCAL, energy);
      if (goodCell) {
        fillOptional1D(mCellAmplitudeCalib_EMCAL, energy);
      }
    } else {
      fillOptional1D(mCellAmplitudeDCAL, energy);
      if (goodCell) {
        fillOptional1D(mCellAmplitudeCalib_DCAL, energy);
      }
      if (energy > mAmplitudeThresholdTime) {
        fillOptional1D(mCellTimeSupermoduleDCAL, cell.getTimeStamp());
        if (goodCell) {
          fillOptional1D(mCellTimeSupermoduleCalib_DCAL, cell.getTimeStamp() - timecalib);
        }
        fillOptional1D(mCellTimeSupermoduleDCAL_Gain[index], cell.getTimeStamp());
      }
    }
    // bc phase histograms
    if (energy > mAmplitudeThresholdTime) {
      auto bchistos = mCellTimeBC[bcphase];
      auto histcontainer = bchistos;
      histcontainer->Fill(cell.getTimeStamp());
    }
  } catch (o2::emcal::InvalidCellIDException& e) {
    ILOG(Info, Support) << "Invalid cell ID: " << e.getCellID() << ENDM;
  }
}

void CellTask::CellHistograms::countEvent()
{
  mnumberEvents->Fill(1);
}

void CellTask::CellHistograms::startPublishing(o2::quality_control::core::ObjectsManager& manager)
{
  auto publishOptional = [&manager](TH1* hist) {
    if (hist) {
      manager.startPublishing(hist);
    }
  };

  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancy);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyThr);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyThrBelow);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyGood);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyBad);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mIntegratedOccupancy);

  publishOptional(mCellTime);
  publishOptional(mCellTimeCalib);
  publishOptional(mCellAmplitude);
  publishOptional(mCellAmplitudeCalib);
  publishOptional(mCellAmpSupermodule);
  publishOptional(mCellAmpSupermoduleCalib);
  publishOptional(mCellAmpSupermoduleBad);
  publishOptional(mCellTimeSupermodule);
  publishOptional(mCellTimeSupermodule_tot);
  publishOptional(mCellTimeSupermoduleEMCAL);
  publishOptional(mCellTimeSupermoduleDCAL);
  publishOptional(mCellTimeSupermoduleCalib_tot);
  publishOptional(mCellTimeSupermoduleCalib_EMCAL);
  publishOptional(mCellTimeSupermoduleCalib_DCAL);
  publishOptional(mCellTimeSupermoduleCalib);
  publishOptional(mCellAmplitudeTime);
  publishOptional(mCellAmplitudeTimeCalib);
  publishOptional(mCellAmplitude_tot);
  publishOptional(mCellAmplitudeEMCAL);
  publishOptional(mCellAmplitudeDCAL);
  publishOptional(mCellAmplitudeCalib_tot);
  publishOptional(mCellAmplitudeCalib_EMCAL);
  publishOptional(mCellAmplitudeCalib_DCAL);
  publishOptional(mCellOccupancy);
  publishOptional(mCellOccupancyThr);
  publishOptional(mCellOccupancyThrBelow);
  publishOptional(mCellOccupancyGood);
  publishOptional(mCellOccupancyBad);
  publishOptional(mIntegratedOccupancy);
  publishOptional(mAverageCellEnergy);
  publishOptional(mAverageCellTime);
  publishOptional(mAverageCellEnergyConstrained);
  publishOptional(mAverageCellTimeConstrained);
  publishOptional(mnumberEvents);

  for (auto histos : mCellTimeSupermoduleEMCAL_Gain) {
    publishOptional(histos);
  }
  for (auto histos : mCellTimeSupermoduleDCAL_Gain) {
    publishOptional(histos);
  }

  for (auto histos : mCellTimeBC) {
    publishOptional(histos);
  }
}

void CellTask::CellHistograms::reset()
{
  auto resetOptional = [](TH1* hist) {
    if (hist) {
      hist->Reset();
    }
  };

  resetOptional(mCellTime);
  resetOptional(mCellTimeCalib);
  resetOptional(mCellAmplitude);
  resetOptional(mCellAmplitudeCalib);
  resetOptional(mCellAmpSupermodule);
  resetOptional(mCellAmpSupermoduleCalib);
  resetOptional(mCellAmpSupermoduleBad);
  resetOptional(mCellTimeSupermodule);
  resetOptional(mCellTimeSupermodule_tot);
  resetOptional(mCellTimeSupermoduleEMCAL);
  resetOptional(mCellTimeSupermoduleDCAL);
  resetOptional(mCellTimeSupermoduleCalib_tot);
  resetOptional(mCellTimeSupermoduleCalib_EMCAL);
  resetOptional(mCellTimeSupermoduleCalib_DCAL);
  resetOptional(mCellTimeSupermoduleCalib);
  resetOptional(mCellAmplitude_tot);
  resetOptional(mCellAmplitudeEMCAL);
  resetOptional(mCellAmplitudeDCAL);
  resetOptional(mCellAmplitudeCalib_tot);
  resetOptional(mCellAmplitudeCalib_EMCAL);
  resetOptional(mCellAmplitudeCalib_DCAL);
  resetOptional(mCellAmplitudeTime);
  resetOptional(mCellAmplitudeTimeCalib);
  resetOptional(mCellOccupancy);
  resetOptional(mCellOccupancyThr);
  resetOptional(mCellOccupancyThrBelow);
  resetOptional(mCellOccupancyGood);
  resetOptional(mCellOccupancyBad);
  resetOptional(mIntegratedOccupancy);
  resetOptional(mAverageCellEnergy);
  resetOptional(mAverageCellTime);
  resetOptional(mAverageCellEnergyConstrained);
  resetOptional(mAverageCellTimeConstrained);
  resetOptional(mnumberEvents);

  for (auto histos : mCellTimeSupermoduleEMCAL_Gain) {
    resetOptional(histos);
  }
  for (auto histos : mCellTimeSupermoduleDCAL_Gain) {
    resetOptional(histos);
  }

  for (auto histos : mCellTimeBC) {
    resetOptional(histos);
  }

  // Draw Grid Lines
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancy);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyThr);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyThrBelow);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyGood);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mCellOccupancyBad);
  o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mIntegratedOccupancy);
}

void CellTask::CellHistograms::clean()
{
  delete mCellTime;
  delete mCellTimeCalib;
  delete mCellAmplitude;
  delete mCellAmplitudeCalib;
  delete mCellAmpSupermodule;
  delete mCellAmpSupermoduleCalib;
  delete mCellAmpSupermoduleBad;
  delete mCellTimeSupermodule;
  delete mCellTimeSupermodule_tot;
  delete mCellTimeSupermoduleEMCAL;
  delete mCellTimeSupermoduleDCAL;
  delete mCellTimeSupermoduleCalib_tot;
  delete mCellTimeSupermoduleCalib_EMCAL;
  delete mCellTimeSupermoduleCalib_DCAL;
  delete mCellTimeSupermoduleCalib;
  delete mCellAmplitude_tot;
  delete mCellAmplitudeEMCAL;
  delete mCellAmplitudeDCAL;
  delete mCellAmplitudeCalib_tot;
  delete mCellAmplitudeCalib_EMCAL;
  delete mCellAmplitudeCalib_DCAL;
  delete mCellAmplitudeTime;
  delete mCellAmplitudeTimeCalib;
  delete mCellOccupancy;
  delete mCellOccupancyThr;
  delete mCellOccupancyThrBelow;
  delete mCellOccupancyGood;
  delete mCellOccupancyBad;
  delete mIntegratedOccupancy;
  delete mAverageCellEnergy;
  delete mAverageCellTime;
  delete mAverageCellEnergyConstrained;
  delete mAverageCellTimeConstrained;
  delete mnumberEvents;

  mCellTime = nullptr;
  mCellTimeCalib = nullptr;
  mCellAmplitude = nullptr;
  mCellAmplitudeCalib = nullptr;
  mCellAmpSupermodule = nullptr;
  mCellAmpSupermoduleCalib = nullptr;
  mCellAmpSupermoduleBad = nullptr;
  mCellTimeSupermodule = nullptr;
  mCellTimeSupermodule_tot = nullptr;
  mCellTimeSupermoduleEMCAL = nullptr;
  mCellTimeSupermoduleDCAL = nullptr;
  mCellTimeSupermoduleCalib_tot = nullptr;
  mCellTimeSupermoduleCalib_EMCAL = nullptr;
  mCellTimeSupermoduleCalib_DCAL = nullptr;
  mCellTimeSupermoduleCalib = nullptr;
  mCellAmplitude_tot = nullptr;
  mCellAmplitudeEMCAL = nullptr;
  mCellAmplitudeDCAL = nullptr;
  mCellAmplitudeCalib_tot = nullptr;
  mCellAmplitudeCalib_EMCAL = nullptr;
  mCellAmplitudeCalib_DCAL = nullptr;
  mCellAmplitudeTime = nullptr;
  mCellAmplitudeTimeCalib = nullptr;
  mCellOccupancy = nullptr;
  mCellOccupancyThr = nullptr;
  mCellOccupancyThrBelow = nullptr;
  mCellOccupancyGood = nullptr;
  mCellOccupancyBad = nullptr;
  mIntegratedOccupancy = nullptr;
  mAverageCellEnergy = nullptr;
  mAverageCellTime = nullptr;
  mAverageCellEnergyConstrained = nullptr;
  mAverageCellTimeConstrained = nullptr;
  mnumberEvents = nullptr;

  for (auto*& histos : mCellTimeSupermoduleEMCAL_Gain) {
    delete histos;
    histos = nullptr;
  }
  for (auto*& histos : mCellTimeSupermoduleDCAL_Gain) {
    delete histos;
    histos = nullptr;
  }
  for (auto*& histos : mCellTimeBC) {
    delete histos;
    histos = nullptr;
  }
}

} // namespace o2::quality_control_modules::emcal
