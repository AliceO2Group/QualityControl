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
/// \file   CheckerFactory.cxx
/// \author Piotr Konopka
///

#include "QualityControl/CheckerFactory.h"

#include <Framework/DataProcessorSpec.h>
#include "QualityControl/Checker.h"

namespace o2::quality_control::checker
{

using namespace o2::framework;
using namespace o2::quality_control::checker;

DataProcessorSpec CheckerFactory::create(std::string checkerName, std::string taskName, std::string configurationSource)
{
  Checker qcChecker{ checkerName, taskName, configurationSource };

  DataProcessorSpec newChecker{ checkerName,
                                Inputs{ qcChecker.getInputSpec() },
                                Outputs{ qcChecker.getOutputSpec() },
                                adaptFromTask<Checker>(std::move(qcChecker)),
                                Options{},
                                std::vector<std::string>{},
                                std::vector<DataProcessorLabel>{} };

  return newChecker;
}

} // namespace o2::quality_control::checker
