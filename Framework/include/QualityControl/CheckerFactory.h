///
/// \file   CheckerFactory.h
/// \author Piotr Konopka
///
#ifndef QC_CHECKERFACTORY_H
#define QC_CHECKERFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2
{
namespace quality_control
{
namespace checker
{

/// \brief Factory in charge of creating DataProcessorSpec of QC Checker
class CheckerFactory
{
 public:
  CheckerFactory() = default;
  virtual ~CheckerFactory() = default;

  framework::DataProcessorSpec create(std::string checkerName, std::string taskName, std::string configurationSource);
};

} // namespace checker
} // namespace quality_control
} // namespace o2

#endif // QC_CHECKERFACTORY_H
