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
/// \file   CellTask.h
/// \author Markus Fasel, Cristina Terrevoli
///

#ifndef QC_MODULE_EMCAL_CELLTASK_H
#define QC_MODULE_EMCAL_CELLTASK_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include <unordered_map>
#include <string_view>
#include <gsl/span>
#include <CCDB/TObjectWrapper.h>
#include <TProfile2D.h>
#include "CommonDataFormat/InteractionRecord.h"
#include "CommonDataFormat/RangeReference.h"
#include "Headers/DataHeader.h"
#include "DataFormatsEMCAL/TriggerRecord.h"

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2
{
namespace emcal
{
class Geometry;
class BadChannelMap;
class TimeCalibrationParams;
class Cell;
} // namespace emcal

namespace quality_control_modules
{
namespace emcal
{

/// \brief QC Task for EMCAL cells
/// \author Markus Fasel
///
/// The main monitoring component for EMCAL cell (energy and time measurement in tower).
/// Monitoring observables:
/// - Amplitude for different towers
/// - Time for different towers
class CellTask final : public TaskInterface
{
 public:
  struct CellHistograms {
    o2::emcal::Geometry* mGeometry;
    double mCellThreshold = 0; //
    // std::array<TH2*, 2> mCellAmplitude;      ///< Cell amplitude
    TH2* mCellAmplitude = nullptr; ///< Cell amplitude
                                   //    std::array<TH2*, 2> mCellTime;           ///< Cell time
    TH2* mCellTime = nullptr;      ///< Cell time
    // std::array<TH2*, 2> mCellAmplitudeCalib; ///< Cell amplitude calibrated
    TH2* mCellAmplitudeCalib = nullptr; ///< Cell amplitude calibrated
                                        //  std::array<TH2*, 2> mCellTimeCalib;      ///< Cell time calibrated
    TH2* mCellTimeCalib = nullptr;      ///< Cell time calibrated

    TH2* mCellAmpSupermodule = nullptr;
    TH2* mCellAmpSupermoduleCalib = nullptr;
    TH2* mCellTimeSupermodule = nullptr;
    TH2* mCellTimeSupermoduleCalib = nullptr;

    TH2* mCellOccupancy = nullptr;                             ///< Cell occupancy EMCAL and DCAL
    TH2* mCellOccupancyThr = nullptr;                          ///< Cell occupancy EMCAL and DCAL with Energy trheshold
    TH2* mCellOccupancyThrBelow = nullptr;                     ///< Cell occupancy EMCAL and DCAL with Energy trheshold
    TH2* mIntegratedOccupancy = nullptr;                       ///< Cell integrated occupancy
    TH1* mCellAmplitude_tot = nullptr;                         ///< Cell amplitude in EMCAL,DCAL
    TH1* mCellAmplitudeEMCAL = nullptr;                        ///< Cell amplitude in EMCAL
    std::unordered_map<int, std::array<TH1*, 20>> mCellTimeBC; ///< Cell amplitude in EMCAL if bc==0
    TH1* mCellAmplitudeDCAL = nullptr;                         ///< Cell amplitude in DCAL
    TH1* mCellTimeSupermodule_tot = nullptr;                   ///< Cell time in EMCAL,DCAL per SuperModule
    TH1* mCellTimeSupermoduleEMCAL = nullptr;                  ///< Cell time in EMCAL per SuperModule
    TH1* mCellTimeSupermoduleDCAL = nullptr;                   ///< Cell time in DCAL per SuperModule
    TH1* mnumberEvents = nullptr;                              ///< Number of Events for normalization

    void initForTrigger(const std::string trigger, bool hasAmpVsCellID, bool hasTimeVsCellID, bool hasHistosCalib2D);
    void startPublishing(o2::quality_control::core::ObjectsManager& manager);
    void reset();
    void clean();

    void fillHistograms(const o2::emcal::Cell& cell, bool isGood, double timeoffset, int bcphase);
    void countEvent();
  };

  TH1* mEvCounterTF = nullptr;      ///< Number of Events per timeframe
  TH1* mEvCounterTFPHYS = nullptr;  ///< Number of Events per timeframe per PHYS
  TH1* mEvCounterTFCALIB = nullptr; ///< Number of Events per timeframe per CALIB
  TH1* mTFPerCyclesTOT = nullptr;   ///< Number of Time Frame per cycles TOT
  TH1* mTFPerCycles = nullptr;      ///< Number of Time Frame per cycles per MonitorData
  TH1* mCellsMaxSM = nullptr;       ///< Supermodule with the largest amount of cells

  /// \brief Constructor
  CellTask() = default;
  /// Destructor
  ~CellTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  bool hasConfigValue(const std::string_view key);
  std::string getConfigValue(const std::string_view key);
  std::string getConfigValueLower(const std::string_view key);

 private:
  struct SubEvent {
    header::DataHeader::SubSpecificationType mSpecification;
    dataformats::RangeReference<int, int> mCellRange;
  };

  struct CombinedEvent {
    InteractionRecord mInteractionRecord;
    uint32_t mTriggerType;
    std::vector<SubEvent> mSubevents;

    int getNumberOfObjects() const
    {
      int nObjects = 0;
      for (auto ev : mSubevents) {
        nObjects += ev.mCellRange.getEntries();
      }
      return nObjects;
    }

    int getNumberOfSubevents()
    {
      return mSubevents.size();
    };
  };
  std::vector<CombinedEvent> buildCombinedEvents(const std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::TriggerRecord>>& triggerrecords) const;
  Bool_t mIgnoreTriggerTypes = false;                        ///< Do not differenciate between trigger types, treat all triggers as phys. triggers
  std::map<std::string, CellHistograms> mHistogramContainer; ///< Container with histograms per trigger class
  o2::emcal::Geometry* mGeometry = nullptr;                  ///< EMCAL geometry
  o2::emcal::BadChannelMap* mBadChannelMap;                  ///< EMCAL channel map
  o2::emcal::TimeCalibrationParams* mTimeCalib;              ///< EMCAL time calib
  int mTimeFramesPerCycles = 0;                              ///< TF per cycles
};

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_EMCAL_CELLTASK_H
