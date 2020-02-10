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
/// \file   CheckRunnerFactory.h
/// \author Piotr Konopka
///

#ifndef QC_CHECKERFACTORY_H
#define QC_CHECKERFACTORY_H

#include <string>

#include "Framework/DataProcessorSpec.h"
#include <Framework/CompletionPolicy.h>
#include "QualityControl/Check.h"

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

  framework::DataProcessorSpec create(Check check, std::string configurationSource, std::vector<std::string> storeVector = {});
  framework::DataProcessorSpec create(std::vector<Check> checks, std::string configurationSource, std::vector<std::string> storeVector = {});

  /*
   * \brief Create a CheckRunner sink DPL device.
   * 
   * The purpose of this device is to receive and store the MO from task.
   *
   * @param input InputSpec with the content to store
   * @param configurationSource
   */
  framework::DataProcessorSpec createSinkDevice(o2::framework::InputSpec input, std::string configurationSource);

  static void customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies);
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKERFACTORY_H
