// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TRFCollectionTask.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_SKELETONPOSTPROCESSING_H
#define QUALITYCONTROL_SKELETONPOSTPROCESSING_H

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
  //  o2::analysis::TimeRangeFlagCollection mTRFCollection;
};

} // namespace o2::quality_control_modules::common

#endif //QUALITYCONTROL_SKELETONPOSTPROCESSING_H