///
/// \file   Quality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Quality.h"

ClassImp(o2::quality_control::core::Quality)

  namespace o2::quality_control::core
{

  const unsigned int Quality::NullLevel =
    10; // could be changed if needed but I don't see why we would need more than 10 levels

  const Quality Quality::Good(1, "Good");
  const Quality Quality::Medium(2, "Medium");
  const Quality Quality::Bad(3, "Bad");
  const Quality Quality::Null(NullLevel, "Null"); // we consider it the worst of the worst

  Quality::Quality(unsigned int level, std::string name) : mLevel(level), mName(name) {}

  Quality::~Quality() {}

  unsigned int Quality::getLevel() const { return mLevel; }

  const std::string& Quality::getName() const { return mName; }

} // namespace o2::quality_control::core
