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

#ifndef QUALITYCONTROL_DATAPROCESSORADAPTER_H
#define QUALITYCONTROL_DATAPROCESSORADAPTER_H

///
/// \file   DataProcessorAdapter.h
/// \author Piotr Konopka
///

#include <Framework/DataProcessorSpec.h>

#include "QualityControl/Actor.h"
#include "QualityControl/ActorTraits.h"
#include "QualityControl/ActorHelpers.h"

namespace o2::quality_control::core {

struct DataProcessorAdapter
{
  template<typename ConcreteActor>
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
        } else {
          // that's the default in DPL
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