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

#ifndef QUALITYCONTROL_CALIBMONITORINGTASK_H
#define QUALITYCONTROL_CALIBMONITORINGTASK_H

// QC includes
#include "QualityControl/PostProcessingInterface.h"
#include "EMCALCalib/CalibDB.h"
#include "EMCALBase/TriggerMappingV2.h"
#include "EMCALBase/Geometry.h"
#include <memory>
#include <vector>
#include <string>

class TCanvas;
class TPaveText;
class TH1;
class TH2;

namespace o2::emcal
{
class CalibDB;
class MappingHandler;
} // namespace o2::emcal

namespace o2::quality_control_modules::emcal
{

/// \class CalibMonitoringTask
/// \brief Quality Control task for the calibration data of the EMCAL
/// \ingroup EMCALQCCPostprocessing
/// \author Cristina Terrevoli
/// \author Markus Fasel <markus.fasel@cern.ch>, Oak Rige National Laboratory
class CalibMonitoringTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  CalibMonitoringTask() = default;
  /// \brief Destructor
  ~CalibMonitoringTask() = default;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param config   ConfigurationInterface with prefix set to ""
  void configure(const boost::property_tree::ptree& config) override;
  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

  void reset();

 private:
  int GetTRUIndexFromSTUIndex(Int_t id, Int_t detector);
  int GetChannelForMaskRun2(int mask, int bitnumber, bool onethirdsm);
  std::vector<int> GetAbsFastORIndexFromMask();

  std::vector<std::string> mCalibObjects;             ///< list of vectors of parm objects to be processed
  TH1* mTimeCalibParamHisto = nullptr;                ///< Monitor Time Calib Param
  TH2* mTimeCalibParamPosition = nullptr;             ///< Monitor time calib param as function of the position in EMCAL
  TH2* mBadChannelMapHisto = nullptr;                 ///< Monitor bad channel map
  TH1* mMaskStatsEMCALHisto = nullptr;                ///< Monitor number of good, bad, dead cells in emcal only
  TH1* mMaskStatsDCALHisto = nullptr;                 ///< Monitor number of good, bad, dead cells in dcal only
  TH1* mMaskStatsAllHisto = nullptr;                  ///< Monitor number of good, bad, dead cells in emcal + dcal only
  TH2* mMaskStatsSupermoduleHisto = nullptr;          ///< Monitor number of good, bad, and dead cells per supermodule
  TH2* mNumberOfBadChannelsFEC = nullptr;             ///< Number of bad channels per FEC
  TH2* mNumberOfDeadChannelsFEC = nullptr;            ///< Number of dead channels per FEC
  TH2* mNumberOfNonGoodChannelsFEC = nullptr;         ///< Number of dead+bad channels per FEC
  TH1* mSRUFirmwareVersion = nullptr;                 ///< The SRU Firmware version as function of supermodule ID
  TH1* mActiveDDLs = nullptr;                         ///< Monitor which DDLs are active
  TH1* mTRUThresholds = nullptr;                      ///< The L0 threshold vs TRU ID PHYS
  TH1* mL0Algorithm = nullptr;                        ///< The L0 algorithm vs TRU ID
  TH1* mRollbackSTU = nullptr;                        ///< The Rollback buffer vs TRU ID
  TH2* mTRUMaskPositionHisto = nullptr;               ///< The FastOR Mask Position in Eta, Phi Histogram
  std::unique_ptr<o2::emcal::CalibDB> mCalibDB;       ///< EMCAL calibration DB handler
  std::unique_ptr<o2::emcal::MappingHandler> mMapper; ///< EMCAL mapper
  o2::emcal::BadChannelMap* mBadChannelMap;           ///< EMCAL channel map
  o2::emcal::TimeCalibrationParams* mTimeCalib;       ///< EMCAL time calib
  o2::emcal::FeeDCS* mFeeDCS;                         ///< EMCAL FEE DCS

  o2::emcal::Geometry* mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);                                  ///< Geometry for mapping position between SM and full EMCAL
  std::unique_ptr<o2::emcal::TriggerMappingV2> mTriggerMapping = std::make_unique<o2::emcal::TriggerMappingV2>(mGeometry); ///!<! Trigger mapping

  //  std::vector<std::map<std::string, std::string>> mStoreMaps{};          ///< meta data to be stored with the output in the QCDB
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_CALIBMONITORINGTASK_H
