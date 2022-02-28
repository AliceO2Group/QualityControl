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

#include "QualityControl/TaskInterface.h"
#include <DataFormatsEMCAL/EventHandler.h>
#include <EMCALBase/ClusterFactory.h>
#include "EMCALBase/Geometry.h"

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal::ClusterTask
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

 private:
  o2::emcal::Geometry* mGeometry;
  std::unique_ptr<o2::emcal::EventHandler<o2::emcal::Cell>> mEventHanlder;
  std::unique_ptr<o2::emcal::ClusterFactory<o2::emcal::Cell>> mClusterFactory;

  TH1* mHistClustE_EMCal = nullptr;
  TH1* mHistNCells_EMCal = nullptr;
  TH1* mHistM02_EMCal = nullptr;
  TH1* mHistM20_EMCal = nullptr;

  TH1* mHistClustE_DCal = nullptr;
  TH1* mHistNCells_DCal = nullptr;
  TH1* mHistM02_DCal = nullptr;
  TH1* mHistM20_DCal = nullptr;
};

} // namespace o2::quality_control_modules::emcal::ClusterTask

#endif // QC_MODULE_EMC_EMCCLUSTERTASK_H
