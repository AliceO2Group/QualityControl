///
/// \file   DatabaseFactory.cxx
/// \author Barthelemy von Haller
///

// std
#include <sstream>
// ROOT
#include <TSystem.h>
#include <TClass.h>
#include <TROOT.h>
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#ifdef _WITH_MYSQL
#include "QualityControl/MySqlDatabase.h"
#endif

using namespace std;
using namespace AliceO2::Common;

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

DatabaseInterface* DatabaseFactory::create(std::string name)
{
  if (name == "MySql") {
#ifdef _WITH_MYSQL
    return new MySqlDatabase();
#else
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("MySQL was not available during the compilation of the QC"));
#endif
  } else {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("No database named " + name));
  }
  return nullptr;
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
