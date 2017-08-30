///
/// \file   Quality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Quality.h"

ClassImp(o2::quality_control::core::Quality)

namespace o2 {
namespace quality_control {
namespace core {

const Quality Quality::Null(0, "Null");
const Quality Quality::Good(1, "Good");
const Quality Quality::Medium(2, "Medium");
const Quality Quality::Bad(3, "Bad");

Quality::Quality(unsigned int level, std::string name)
    : mLevel(level), mName(name)
{
}

Quality::~Quality()
{
}

unsigned int Quality::getLevel() const
{
  return mLevel;
}

const std::string& Quality::getName() const
{
  return mName;
}

} // namespace core
} // namespace quality_control
} // namespace o2
