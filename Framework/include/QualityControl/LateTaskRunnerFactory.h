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
/// \file   LateTaskRunnerFactory.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_LATETASKRUNNERFACTORY_H
#define QC_CORE_LATETASKRUNNERFACTORY_H

#include <vector>
#include <optional>

#include "QualityControl/LateTaskRunnerConfig.h"
#include <Framework/DataProcessorSpec.h>

namespace o2::framework
{
class CompletionPolicy;
}

namespace o2::quality_control::core
{

struct LateTaskSpec;
struct LateTaskRunnerConfig;
struct CommonSpec;

/// \brief Factory in charge of creating DataProcessorSpec of LateTaskRunner
class LateTaskRunnerFactory
{
  public:
  LateTaskRunnerFactory() = default;
  virtual ~LateTaskRunnerFactory() = default;

  /// \brief Creates LateTaskRunner
  ///
  /// \param taskConfig
  static o2::framework::DataProcessorSpec create(const LateTaskRunnerConfig&);

  /// \brief Knows how to create LateTaskConfig from Specs
  static LateTaskRunnerConfig extractConfig(const CommonSpec&, const LateTaskSpec&);

  /// \brief Provides necessary customization of the LateTaskRunners.
  ///
  /// Provides necessary customization of the Completion Policies of the LateTaskRunners. This is necessary to make
  /// them work. Put it inside customize() function before including <Framework/runDataProcessing.cxx>.
  /// \param policies - completion policies vector
  static void customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies);
};

} // namespace o2::quality_control::core

#endif //QC_CORE_LATETASKRUNNERFACTORY_H
