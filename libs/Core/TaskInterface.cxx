///
/// \file   TaskInterface.cxx
/// \author Barthelemy von Haller
///

#include "TaskInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskInterface::TaskInterface(std::string name, Publisher *publisher) :
  mName(name), mPublisher(publisher)
{
}

TaskInterface::~TaskInterface()
{
}

const std::string &TaskInterface::getName() const
{
  return mName;
}

void TaskInterface::setName(const std::string &name)
{
  mName = name;
}

void TaskInterface::setPublisher(Publisher *publisher)
{
  mPublisher = publisher;
}


} /* namespace Core */
} /* namespace QualityControl */
} /* namespace AliceO2 */
