///
/// \file   CheckerDataProcessorFactory.h
/// \author Piotr Konopka
///
#ifndef PROJECT_CHECKERDATAPROCESSORFACTORY_H
#define PROJECT_CHECKERDATAPROCESSORFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2 {
namespace quality_control {
namespace checker {

/// \brief Factory in charge of creating DataProcessorSpec of QC Checker
class CheckerDataProcessorFactory {
public:
  CheckerDataProcessorFactory();
  virtual ~CheckerDataProcessorFactory();

  framework::DataProcessorSpec create(std::string checkerName, std::string taskName, std::string configurationSource);
};

} // namespace checker
} // namespace quality_control
} // namespace o2

#endif // PROJECT_CHECKERDATAPROCESSORFACTORY_H
