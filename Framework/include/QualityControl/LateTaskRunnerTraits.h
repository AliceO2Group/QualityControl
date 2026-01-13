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

#ifndef QUALITYCONTROL_LATETASKRUNNERTRAITS_H
#define QUALITYCONTROL_LATETASKRUNNERTRAITS_H

///
/// \file   LateTaskRunnerTraits.h
/// \author Piotr Konopka
///

#include <string_view>
#include <array>

#include "QualityControl/ActorTraits.h"
#include "QualityControl/DataSourceSpec.h"

namespace o2::quality_control::core {

class LateTaskRunner;

template <>
struct ActorTraits<LateTaskRunner>
{
  constexpr static std::string_view sActorTypeShort{"late"};
  constexpr static std::string_view sActorTypeKebabCase{"qc-late-task"};
  constexpr static std::string_view sActorTypeUpperCamelCase{"LateTaskRunner"};

  constexpr static size_t sDataDescriptionHashLength{4};

  constexpr static std::array<DataSourceType, 5> sConsumedDataSources{
    DataSourceType::Task, DataSourceType::TaskMovingWindow, DataSourceType::Check, DataSourceType::Aggregator, DataSourceType::LateTask
   };
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources{DataSourceType::LateTask};

  constexpr static std::array<Service, 2> sRequiredServices{Service::InfoLogger, Service::Monitoring};
  constexpr static bkp::DplProcessType sDplProcessType{bkp::DplProcessType::QC_POSTPROCESSING}; // todo at some point we should add a new type in BKP

  constexpr static UserCodeInstanceCardinality sUserCodeInstanceCardinality{UserCodeInstanceCardinality::One};
  constexpr static bool sDetectorSpecific{true};
  constexpr static Criticality sCriticality{Criticality::UserDefined};
};

static_assert(ValidActorTraits<ActorTraits<LateTaskRunner>>);

}

#endif //QUALITYCONTROL_LATETASKRUNNERTRAITS_H