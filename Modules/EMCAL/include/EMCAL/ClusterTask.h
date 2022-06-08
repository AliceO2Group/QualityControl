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
/// \file   ClusterTask.h
/// \author Vivek Kumar Singh
///

#ifndef QC_MODULE_EMC_EMCCLUSTERTASK_H
#define QC_MODULE_EMC_EMCCLUSTERTASK_H

#include <array>
#include <unordered_map>
#include <string_view>

#include "QualityControl/TaskInterface.h"
#include <DataFormatsEMCAL/EventHandler.h>
#include <EMCALBase/ClusterFactory.h>
#include "EMCALBase/Geometry.h"
#include <EMCALReconstruction/Clusterizer.h> //svk

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

/// \brief Example Quality Control DPL Task
/// \author Vivek Kumar Singh
class ClusterTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ClusterTask() = default;
  /// Destructor
  ~ClusterTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override { resetHistograms(); }

 protected:
  void resetHistograms();

  void analyseTimeframe(const gsl::span<const o2::emcal::Cell>& cells, const gsl::span<const o2::emcal::TriggerRecord>& cellTriggerRecords, const gsl::span<const o2::emcal::Cluster> clusters, const gsl::span<const o2::emcal::TriggerRecord> clusterTriggerRecords, const gsl::span<const int> clusterIndices, const gsl::span<const o2::emcal::TriggerRecord> cellIndexTriggerRecords); //svk

  void findClustersInternal(const gsl::span<const o2::emcal::Cell>& cells, const gsl::span<const o2::emcal::TriggerRecord>& cellTriggerRecords, std::vector<o2::emcal::Cluster>& clusters, std::vector<o2::emcal::TriggerRecord>& clusterTriggerRecords, std::vector<int>& clusterIndices, std::vector<o2::emcal::TriggerRecord>& clusterIndexTriggerRecords); //svk

 private:
  o2::emcal::Geometry* mGeometry;
  std::unique_ptr<o2::emcal::EventHandler<o2::emcal::Cell>> mEventHandler;
  std::unique_ptr<o2::emcal::ClusterFactory<o2::emcal::Cell>> mClusterFactory;
  std::unique_ptr<o2::emcal::Clusterizer<o2::emcal::Cell>> mClusterizer; //svk

  bool hasConfigValue(const std::string_view key);
  std::string getConfigValue(const std::string_view key);
  std::string getConfigValueLower(const std::string_view key);

  bool mInternalClusterizer = false; //svk

  TH1* mHistNclustPerTF = nullptr;  //svk
  TH1* mHistNclustPerEvt = nullptr; //svk
  TH2* mHistClustEtaPhi = nullptr;  //svk

  TH2* mHistTime_EMCal = nullptr; //svk
  TH1* mHistClustE_EMCal = nullptr;
  TH1* mHistNCells_EMCal = nullptr;
  TH1* mHistM02_EMCal = nullptr;
  TH1* mHistM20_EMCal = nullptr;
  TH2* mHistM02VsClustE__EMCal = nullptr; //svk
  TH2* mHistM20VsClustE__EMCal = nullptr; //svk

  TH2* mHistTime_DCal = nullptr; //svk
  TH1* mHistClustE_DCal = nullptr;
  TH1* mHistNCells_DCal = nullptr;
  TH1* mHistM02_DCal = nullptr;
  TH1* mHistM20_DCal = nullptr;
  TH2* mHistM02VsClustE__DCal = nullptr; //svk
  TH2* mHistM20VsClustE__DCal = nullptr; //svk
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMC_EMCCLUSTERTASK_H
