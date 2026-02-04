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
/// \file   ActorHelpers.cxx
/// \author Piotr Konopka
///

#include <format>

#include "QualityControl/ActorHelpers.h"
#include "QualityControl/CommonSpec.h"
#include "QualityControl/InfrastructureSpecReader.h"


namespace o2::quality_control::core::actor_helpers {

ServicesConfig extractConfig(const CommonSpec& commonSpec)
{
  return ServicesConfig{
    .database = commonSpec.database,
    .activity{
      commonSpec.activityNumber,
      commonSpec.activityType,
      commonSpec.activityPeriodName,
      commonSpec.activityPassName,
      commonSpec.activityProvenance,
      {commonSpec.activityStart, commonSpec.activityEnd},
      commonSpec.activityBeamType,
      commonSpec.activityPartitionName,
      commonSpec.activityFillNumber,
      commonSpec.activityOriginalNumber
      },
    .monitoringUrl = commonSpec.monitoringUrl,
    .conditionDBUrl = commonSpec.conditionDBUrl,
    .infologgerDiscardParameters = commonSpec.infologgerDiscardParameters,
    .bookkeepingUrl = commonSpec.bookkeepingUrl,
    .kafkaBrokersUrl = commonSpec.kafkaBrokersUrl,
    .kafkaTopicAliECSRun = commonSpec.kafkaTopicAliECSRun
  };
}

std::string dataProcessorName(std::string_view actorTypeKebabCase, std::string_view userCodeName, std::string_view detectorName)
{
  // todo perhaps detector name validation should happen earlier, just once and throw in case of configuration errors
  return std::format("{}-{}-{}", actorTypeKebabCase, InfrastructureSpecReader::validateDetectorName(std::string{detectorName}), userCodeName);
}

}