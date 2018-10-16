///
/// \file   CheckerDataProcessorFactory.h
/// \author Piotr Konopka
///
#ifndef QC_CHECKER_DATAPROCESSORFACTORY_H
#define QC_CHECKER_DATAPROCESSORFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2
{
namespace quality_control
{
namespace checker
{

/// \brief Factory in charge of creating DataProcessorSpec of QC Checker
class CheckerDataProcessorFactory
{
 public:
  CheckerDataProcessorFactory() = default;
  virtual ~CheckerDataProcessorFactory() = default;

  framework::DataProcessorSpec create(std::string checkerName, std::string taskName, std::string configurationSource);
};

} // namespace checker
} // namespace quality_control
} // namespace o2

#endif // QC_CHECKER_DATAPROCESSORFACTORY_H
