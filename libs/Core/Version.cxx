///
/// \file    Version.cxx
/// \brief   Version Information
/// \author  Barthelemy von Haller
///

#include "Core/Version.h"
#include <sstream>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

int Version::getMajor()
{
  return QUALITYCONTROL_VERSION_MAJOR;
}

int Version::getMinor()
{
  return QUALITYCONTROL_VERSION_MINOR;
}

int Version::getPatch()
{
  return QUALITYCONTROL_VERSION_PATCH;
}

std::string Version::getString()
{
  std::ostringstream version;
  version << QUALITYCONTROL_VERSION_MAJOR << '.' << QUALITYCONTROL_VERSION_MINOR << '.' << QUALITYCONTROL_VERSION_PATCH;
  return version.str();
}

std::string Version::getRevision()
{
  return QUALITYCONTROL_VCS_REVISION;
}

std::string Version::getRevString()
{
  std::ostringstream version;
  version << getString() << '.' << QUALITYCONTROL_VCS_REVISION;
  return version.str();
}
}
}
}
