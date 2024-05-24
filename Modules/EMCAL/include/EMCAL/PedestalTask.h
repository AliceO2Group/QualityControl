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

#ifndef QC_MODULE_EMCAL_EMCALPEDESTALTASK_H
#define QC_MODULE_EMCAL_EMCALPEDESTALTASK_H

#include "QualityControl/TaskInterface.h"

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2::emcal
{
class Geometry;
}

namespace o2::quality_control_modules::emcal
{

/// \class PedestalTask
/// \brief Monitoring of the Pedestals extracted by the pedestal calibration
/// \ingroup EMCALQCTasks
/// \author Markus Fasel <markus.fasel@cern.ch>, Oak Ridge National Laboratory
/// \since March 27th, 2024
class PedestalTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  PedestalTask() = default;
  /// Destructor
  ~PedestalTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  TH1* mPedestalChannelFECHG = nullptr;
  TH1* mPedestalChannelFECLG = nullptr;
  TH1* mPedestalChannelLEDMONHG = nullptr;
  TH1* mPedestalChannelLEDMONLG = nullptr;

  TH2* mPedestalPositionFECHG = nullptr;
  TH2* mPedestalPositionFECLG = nullptr;
  TH2* mPedestalPositionLEDMONHG = nullptr;
  TH2* mPedestalPositionLEDMONLG = nullptr;

  int mNumberObjectsFetched = 0;

  o2::emcal::Geometry* mGeometry = nullptr;
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALPEDESTALTASK_H
