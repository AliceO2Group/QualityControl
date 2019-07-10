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
/// \file   CheckerFactory.h
/// \author Piotr Konopka
///

#ifndef QC_CHECKERFACTORY_H
#define QC_CHECKERFACTORY_H

#include <string>

#include "Framework/DataProcessorSpec.h"
#include <Framework/CompletionPolicy.h>

namespace o2::framework
{
struct DataProcessorSpec;
}

namespace o2::quality_control::checker
{

/// \brief Factory in charge of creating DataProcessorSpec of QC Checker
class CheckerFactory
{
 public:
  CheckerFactory() = default;
  virtual ~CheckerFactory() = default;

  framework::DataProcessorSpec create(std::string checkerName, std::string configurationSource);
  framework::DataProcessorSpec create(std::vector<std::string> checkerNames, std::string configurationSource);

  static void customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies);
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKERFACTORY_H
