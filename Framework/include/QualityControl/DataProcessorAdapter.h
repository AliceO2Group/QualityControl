//
// Created by pkonopka on 02/12/2025.
//

#ifndef QUALITYCONTROL_DATAPROCESSORADAPTER_H
#define QUALITYCONTROL_DATAPROCESSORADAPTER_H

#include "QualityControl/Actor.h"
#include "QualityControl/ActorTraits.h"
#include "QualityControl/ActorHelpers.h"

#include <Framework/DataProcessorSpec.h>

namespace o2::quality_control::core {

struct DataProcessorAdapter
{
  template<typename ConcreteActor>
  requires ValidActorTraits<ActorTraits<ConcreteActor>>
  static o2::framework::DataProcessorSpec
  adapt(ConcreteActor&& actor, std::string&& dataProcessorName, framework::Inputs&& inputs, framework::Outputs&& outputs, framework::Options&& options)
  {
    using traits = ActorTraits<ConcreteActor>;

    auto actorPtr = std::make_shared<ConcreteActor>(std::move(actor));
    o2::framework::DataProcessorSpec dataProcessor;

    dataProcessor.name = std::move(dataProcessorName);
    dataProcessor.inputs = std::move(inputs);
    dataProcessor.outputs = std::move(outputs);
    dataProcessor.options = std::move(options);

    dataProcessor.labels = { actor_helpers::dataProcessorLabel<ConcreteActor>() };
    switch (traits::sCriticality) {
      case Criticality::Resilient:
        dataProcessor.labels.emplace_back( "resilient" );
        break;
      case Criticality::Critical:
        // that's the default in DPL
        break;
      case Criticality::Expendable:
        dataProcessor.labels.emplace_back( "expendable" );
        break;
      case Criticality::UserDefined:
        if (!actor.isCritical()) {
          dataProcessor.labels.emplace_back( "expendable" );
        }
        break;
    }

    dataProcessor.algorithm = {
      [actorPtr](framework::InitContext& ictx) {
        actorPtr->init(ictx);
        return [actorPtr](framework::ProcessingContext& ctx) {
          actorPtr->process(ctx);
        };
      }
    };
    return dataProcessor;
  }
};

};

#endif //QUALITYCONTROL_DATAPROCESSORADAPTER_H