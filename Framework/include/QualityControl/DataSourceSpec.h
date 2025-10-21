// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QUALITYCONTROL_DATASOURCESPEC_H
#define QUALITYCONTROL_DATASOURCESPEC_H

///
/// \file   DataSourceSpec.h
/// \author Piotr Konopka
///

#include <string>
#include <vector>
#include <Framework/InputSpec.h>
#include <type_traits>

namespace o2::quality_control::core
{

enum class DataSourceType {
  DataSamplingPolicy,
  Direct,
  Task,
  TaskMovingWindow,
  Check,
  Aggregator,
  PostProcessingTask,
  LateTask,
  ExternalTask,
  Invalid
};

// this should allow us to represent all data sources which come from DPL (and maybe CCDB).
struct DataSourceSpec {
  explicit DataSourceSpec(DataSourceType type = DataSourceType::Invalid);

  template <class... DataSourceType>
  bool isOneOf(DataSourceType... dataSourceType) const
  {
    return (... || (dataSourceType == type));
  }

  DataSourceType type;
  std::string id;
  std::string name;
  std::vector<framework::InputSpec> inputs;
  std::vector<std::string> subInputs; // can be MO or QO names
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_DATASOURCESPEC_H
