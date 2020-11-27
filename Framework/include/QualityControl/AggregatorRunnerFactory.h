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
/// \file   AggregatorRunnerFactory.h
/// \author Barthelemy von Haller
///

#ifndef QC_AGGREGATORRUNNERFACTORY_H
#define QC_AGGREGATORRUNNERFACTORY_H

#include <Framework/DataProcessorSpec.h>
#include <Framework/CompletionPolicy.h>

namespace o2::quality_control::checker
{

/// \brief Factory in charge of creating the AggregatorRunners and their corresponding DataProcessorSpec.
class AggregatorRunnerFactory
{
 public:
  AggregatorRunnerFactory() = default;
  virtual ~AggregatorRunnerFactory() = default;

  static framework::DataProcessorSpec create(const vector<framework::OutputSpec>& checkerRunnerOutputs, const std::string& configurationSource);
  static void customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies);
};

} // namespace o2::quality_control::checker

#endif // QC_AGGREGATORRUNNERFACTORY_H
