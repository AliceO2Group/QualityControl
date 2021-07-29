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
}

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize DigitsQcTask" << AliceO2::InfoLogger::InfoLogger::endm;
  //define histograms

  std::array<std::string, 2> triggers = { { "CAL", "PHYS" } };
  for (const auto& trg : triggers) {
    DigitsHistograms histos;
    histos.initForTrigger(trg.data());
    startPublishing(histos);
    mHistogramContainer[trg] = histos;
  } //trigger type

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void DigitsQcTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void DigitsQcTask::startOfCycle()
{
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
  std::vector<framework::InputSpec> cellInputs{ { "cellfilter", framework::ConcreteDataTypeMatcher(header::gDataOriginEMC, "CELLS") } },
    triggerRecordInputs{ { "triggerrecordfilter", framework::ConcreteDataTypeMatcher(header::gDataOriginEMC, "CELLSTRGR") } };
  std::unordered_map<int, gsl::span<const o2::emcal::Cell>> cellSubEvents;
  std::unordered_map<int, gsl::span<const o2::emcal::TriggerRecord>> triggerRecordSubevents;

  for (const auto& celldata : framework::InputRecordWalker(ctx.inputs(), cellInputs)) {
    int subspecification = framework::DataRefUtils::getHeader<header::DataHeader*>(celldata)->subSpecification;
    cellSubEvents[subspecification] = ctx.inputs().get<gsl::span<o2::emcal::Cell>>(celldata);
  }
  for (const auto& trgrecorddata : framework::InputRecordWalker(ctx.inputs(), triggerRecordInputs)) {
    int subspecification = framework::DataRefUtils::getHeader<header::DataHeader*>(trgrecorddata)->subSpecification;
    triggerRecordSubevents[subspecification] = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(trgrecorddata);
  }

  auto combinedEvents = buildCombinedEvents(triggerRecordSubevents);

  //  QcInfoLogger::GetInstance() << "Received " << digitcontainer.size() << " digits " << AliceO2::InfoLogger::InfoLogger::endm;
  int eventcounter = 0;
  for (auto trg : combinedEvents) {
    if (!trg.getNumberOfObjects()) {
      continue;
    }
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Next event " << eventcounter << " has " << trg.getNumberOfObjects() << " digits" << QcInfoLogger::endm;

    //trigger type
    auto triggertype = trg.mTriggerType;
    bool isPhysTrigger = triggertype & o2::trigger::PhT, isCalibTrigger = triggertype & o2::trigger::Cal;
    std::string trgClass;
    if (isPhysTrigger) {
      trgClass = "PHYS";
    } else if (isCalibTrigger) {
      trgClass = "CAL";
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
        gsl::span<const o2::emcal::Cell> eventdigits(cellsSubspec->second.data() + subev.mCellRange.getFirstEntry(), subev.mCellRange.getEntries());
        for (auto digit : eventdigits) {
          //int index = digit.getHighGain() ? 0 : (digit.getLowGain() ? 1 : -1);
          //if (index < 0)
          //  continue;
          auto cellindices = mGeometry->GetCellIndex(digit.getTower());
          histos.mDigitAmplitude->Fill(digit.getEnergy(), digit.getTower());
          //histos.mDigitAmplitude[index]->Fill(digit.getEnergy(), digit.getTower());

          auto timeoffset = mTimeCalib ? mTimeCalib->getTimeCalibParam(digit.getTower(), digit.getLowGain()) : 0.;

          if (!mBadChannelMap || (mBadChannelMap->getChannelStatus(digit.getTower()) == MaskType_t::GOOD_CELL)) {
            histos.mDigitAmplitudeCalib->Fill(digit.getEnergy(), digit.getTower());
            //histos.mDigitAmplitudeCalib[index]->Fill(digit.getEnergy(), digit.getTower());
            histos.mDigitTimeCalib->Fill(digit.getTimeStamp() - timeoffset, digit.getTower());
            //histos.mDigitTimeCalib[index]->Fill(digit.getTimeStamp() - timeoffset, digit.getTower());
          }
          histos.mDigitTime->Fill(digit.getTimeStamp(), digit.getTower());
          //histos.mDigitTime[index]->Fill(digit.getTimeStamp(), digit.getTower());

          // get the supermodule for filling EMCAL/DCAL spectra

          try {

            auto [row, col] = mGeometry->GlobalRowColFromIndex(digit.getTower());
            if (digit.getEnergy() > 0) {
              histos.mDigitOccupancy->Fill(col, row);
            }
            if (digit.getEnergy() > mCellThreshold) {
              histos.mDigitOccupancyThr->Fill(col, row);
            }
            histos.mIntegratedOccupancy->Fill(col, row, digit.getEnergy());

            if (std::get<0>(cellindices) < 12)
              histos.mDigitAmplitudeEMCAL->Fill(digit.getEnergy());

            else
              histos.mDigitAmplitudeDCAL->Fill(digit.getEnergy());
          } catch (o2::emcal::InvalidCellIDException& e) {
            QcInfoLogger::GetInstance() << "Invalid cell ID: " << e.getCellID() << AliceO2::InfoLogger::InfoLogger::endm;
          };
        }
      }
    }

    histos.mnumberEvents->Fill(1);
    eventcounter++;
  }
}

void DigitsQcTask::endOfCycle()
{
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
}

std::vector<DigitsQcTask::CombinedEvent> DigitsQcTask::buildCombinedEvents(const std::unordered_map<int, gsl::span<const o2::emcal::TriggerRecord>>& triggerrecords) const
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

void DigitsQcTask::startPublishing(DigitsHistograms& histos)
{
  //  for (auto h : histos.mDigitAmplitude) {
  //    getObjectsManager()->startPublishing(h);
  //  }

  //  for (auto h : histos.mDigitTime) {
  //    getObjectsManager()->startPublishing(h);
  //  }
  //  for (auto h : histos.mDigitAmplitudeCalib) {
  //    getObjectsManager()->startPublishing(h);
  //  }

  // for (auto h : histos.mDigitTimeCalib) {
  //   getObjectsManager()->startPublishing(h);
  //}

  getObjectsManager()->startPublishing(histos.mDigitTime);
  getObjectsManager()->startPublishing(histos.mDigitTimeCalib);
  getObjectsManager()->startPublishing(histos.mDigitAmplitude);
  getObjectsManager()->startPublishing(histos.mDigitAmplitudeCalib);

  getObjectsManager()->startPublishing(histos.mDigitAmplitudeEMCAL);
  getObjectsManager()->startPublishing(histos.mDigitAmplitudeDCAL);
  getObjectsManager()->startPublishing(histos.mDigitOccupancy);
  getObjectsManager()->startPublishing(histos.mDigitOccupancyThr);
  getObjectsManager()->startPublishing(histos.mIntegratedOccupancy);
  getObjectsManager()->startPublishing(histos.mnumberEvents);
}

void DigitsQcTask::DigitsHistograms::initForTrigger(const char* trigger)
{
  mTriggerClass = trigger;

  mDigitAmplitude = new TH2F(Form("digitAmplitudeHG_%s", mTriggerClass.data()), Form("Digit Amplitude (High gain) %s", mTriggerClass.data()), 100, 0, 100, 17664, -0.5, 17663.5);
  //mDigitAmplitude[1] = new TH2F(Form("digitAmplitudeLG_%s", mTriggerClass.data()), Form("Digit Amplitude (Low gain) %s", mTriggerClass.data()), 100, 0, 100, 17664, -0.5, 17663.5);

  mDigitAmplitudeCalib = new TH2F(Form("digitAmplitudeHGCalib_%s", mTriggerClass.data()), Form("Digit Amplitude (High gain) %s", mTriggerClass.data()), 100, 0, 100, 17664, -0.5, 17663.5);
  //mDigitAmplitudeCalib[1] = new TH2F(Form("digitAmplitudeLGCalib_%s", mTriggerClass.data()), Form("Digit Amplitude (Low gain) %s", mTriggerClass.data()), 100, 0, 100, 17664, -0.5, 17663.5);

  mDigitTime = new TH2F(Form("digitTimeHG_%s", mTriggerClass.data()), Form("Digit Time (High gain) %s", mTriggerClass.data()), 2000, -200, 200, 17664, -0.5, 17663.5);
  //  mDigitTime[1] = new TH2F(Form("digitTimeLG_%s", mTriggerClass.data()), Form("Digit Time (Low gain) %s", mTriggerClass.data()), 2000, -200, 200, 17664, -0.5, 17663.5);

  mDigitTimeCalib = new TH2F(Form("digitTimeHGCalib_%s", mTriggerClass.data()), Form("Digit Time Calib (High gain) %s", mTriggerClass.data()), 2000, -200, 200, 17664, -0.5, 17663.5);
  //  mDigitTimeCalib[1] = new TH2F(Form("digitTimeLGCalib_%s", mTriggerClass.data()), Form("Digit Time Calib (Low gain) %s", mTriggerClass.data()), 2000, -200, 200, 17664, -0.5, 17663.5);

  mDigitOccupancy = new TH2F(Form("digitOccupancyEMC_%s", mTriggerClass.data()), Form("Digit Occupancy EMCAL %s", mTriggerClass.data()), 96, -0.5, 95.5, 208, -0.5, 207.5);
  mDigitOccupancyThr = new TH2F(Form("digitOccupancyEMCwThr_%s", mTriggerClass.data()), Form("Digit Occupancy EMCAL with E>0.5 GeV/c %s", mTriggerClass.data()), 96, -0.5, 95.5, 208, -0.5, 207.5);

  mIntegratedOccupancy = new TProfile2D(Form("digitOccupancyInt_%s", mTriggerClass.data()), Form("Digit Occupancy Integrated %s", mTriggerClass.data()), 96, -0.5, 95.5, 208, -0.5, 207.5);
  mIntegratedOccupancy->GetXaxis()->SetTitle("col");
  mIntegratedOccupancy->GetYaxis()->SetTitle("row");
  // 1D histograms for showing the integrated spectrum

  mDigitAmplitudeEMCAL = new TH1F(Form("digitAmplitudeEMCAL_%s", mTriggerClass.data()), Form("Digit amplitude in EMCAL %s", mTriggerClass.data()), 500, 0., 500.);
  mDigitAmplitudeDCAL = new TH1F(Form("digitAmplitudeDCAL_%s", mTriggerClass.data()), Form("Digit amplitude in DCAL %s", mTriggerClass.data()), 500, 0., 500.);
  mnumberEvents = new TH1F(Form("NumberOfEvents_%s", mTriggerClass.data()), Form("Number Of Events %s", mTriggerClass.data()), 1, 0.5, 1.5);
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

  mDigitTime->Reset();
  mDigitTimeCalib->Reset();
  mDigitAmplitude->Reset();
  mDigitAmplitudeCalib->Reset();
  mDigitAmplitudeEMCAL->Reset();
  mDigitAmplitudeDCAL->Reset();
  mDigitOccupancy->Reset();
  mDigitOccupancyThr->Reset();
  mIntegratedOccupancy->Reset();
  mnumberEvents->Reset();
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

  if (mDigitTime)
    delete mDigitTime;

  if (mDigitTimeCalib)
    delete mDigitTimeCalib;

  if (mDigitAmplitude)
    delete mDigitAmplitude;

  if (mDigitAmplitudeCalib)
    delete mDigitAmplitudeCalib;

  if (mDigitAmplitudeEMCAL)
    delete mDigitAmplitudeEMCAL;

  if (mDigitAmplitudeDCAL)
    delete mDigitAmplitudeDCAL;

  if (mDigitOccupancy)
    delete mDigitOccupancy;

  if (mDigitOccupancyThr)
    delete mDigitOccupancyThr;

  if (mIntegratedOccupancy)
    delete mIntegratedOccupancy;

  if (mnumberEvents)
    delete mnumberEvents;
}

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2
