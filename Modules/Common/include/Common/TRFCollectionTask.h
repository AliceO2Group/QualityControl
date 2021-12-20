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
/// \file   TRFCollectionTask.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TRFCOLLECTIONTASK_H
#define QUALITYCONTROL_TRFCOLLECTIONTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "Common/TRFCollectionTaskConfig.h"
#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control_modules::common
{

/// \brief Example Quality Control Postprocessing Task
/// \author My Name
class TRFCollectionTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  TRFCollectionTask() = default;
  /// \brief Destructor
  ~TRFCollectionTask() override;

  void configure(std::string name, const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;

 private:
  quality_control::TimeRangeFlagCollection
    transformQualities(quality_control::repository::DatabaseInterface& qcdb, const uint64_t timestampLimitStart, const uint64_t timestampLimitEnd);

 private:
  TRFCollectionTaskConfig mConfig;
  uint64_t mLastTimestampLimitStart = 0;
};

} // namespace o2::quality_control_modules::common

#endif //QUALITYCONTROL_TRFCOLLECTIONTASK_H