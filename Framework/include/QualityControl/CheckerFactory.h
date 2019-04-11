///
/// \file   CheckerFactory.h
/// \author Piotr Konopka
///
#ifndef QC_CHECKERFACTORY_H
#define QC_CHECKERFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2::quality_control::checker
{

/// \brief Factory in charge of creating DataProcessorSpec of QC Checker
class CheckerFactory
{
 public:
  CheckerFactory() = default;
  virtual ~CheckerFactory() = default;

  framework::DataProcessorSpec create(std::string checkerName, std::string taskName, std::string configurationSource);
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKERFACTORY_H
