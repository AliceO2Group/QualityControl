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
/// \file   CheckRunnerFactory.h
/// \author Piotr Konopka
///

#ifndef QC_CHECKRUNNERFACTORY_H
#define QC_CHECKRUNNERFACTORY_H

#include <string>

#include "Framework/DataProcessorSpec.h"
#include <Framework/CompletionPolicy.h>
#include "QualityControl/Check.h"
#include "QualityControl/CheckRunnerConfig.h"

namespace o2::framework
{
struct DataProcessorSpec;
}

namespace o2::quality_control::checker
{

/// \brief Factory in charge of creating DataProcessorSpec of QC CheckRunner
class CheckRunnerFactory
{
 public:
  CheckRunnerFactory() = default;
  virtual ~CheckRunnerFactory() = default;

  static framework::DataProcessorSpec create(CheckRunnerConfig checkRunnerConfig, std::vector<CheckConfig> checkConfigs, std::vector<std::string> storeVector = {});

  /*
   * \brief Create a CheckRunner sink DPL device.
   * 
   * The purpose of this device is to receive and store the MO from task.
   *
   * @param input InputSpec with the content to store
   * @param configurationSource
   */
  static framework::DataProcessorSpec createSinkDevice(CheckRunnerConfig checkRunnerConfig, o2::framework::InputSpec input);

  static CheckRunnerConfig extractConfig(const CommonSpec&);

  static void customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies);
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKRUNNERFACTORY_H
