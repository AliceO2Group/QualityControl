//
// Created by pkonopka on 04/12/2025.
//


#include "QualityControl/ActorHelpers.h"
#include "QualityControl/CommonSpec.h"


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

}