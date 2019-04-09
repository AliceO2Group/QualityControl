///
/// \file   CheckerFactory.cxx
/// \author Piotr Konopka
///

#include <Framework/DataProcessorSpec.h>

#include "QualityControl/Checker.h"
#include "QualityControl/CheckerFactory.h"

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

  return std::move(newChecker);
}

} // namespace o2::quality_control::checker
