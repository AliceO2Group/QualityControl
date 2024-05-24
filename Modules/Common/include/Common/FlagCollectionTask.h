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
/// \file   FlagCollectionTask.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_QCFCOLLECTIONTASK_H
#define QUALITYCONTROL_QCFCOLLECTIONTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "Common/FlagCollectionTaskConfig.h"
#include <DataFormatsQualityControl/QualityControlFlagCollection.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control_modules::common
{

/// \brief Example Quality Control Postprocessing Task
/// \author My Name
class FlagCollectionTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  FlagCollectionTask() = default;
  /// \brief Destructor
  ~FlagCollectionTask() override;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  std::unique_ptr<quality_control::QualityControlFlagCollection>
    transformQualities(quality_control::repository::DatabaseInterface& qcdb, const uint64_t timestampLimitStart, const uint64_t timestampLimitEnd);

 private:
  FlagCollectionTaskConfig mConfig;
  uint64_t mLastTimestampLimitStart = 0;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_QCFCOLLECTIONTASK_H