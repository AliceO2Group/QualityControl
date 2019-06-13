///
/// \file   DatabaseFactory.cxx
/// \author Barthelemy von Haller
///

// std
#include <sstream>
// ROOT
#include <QualityControl/CcdbDatabase.h>
#include <TClass.h>
#include <TROOT.h>
#include <TSystem.h>
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskInterface.h"
#ifdef _WITH_MYSQL
#include "QualityControl/MySqlDatabase.h"
#endif

using namespace std;
using namespace AliceO2::Common;
using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control
{
namespace repository
{

std::unique_ptr<DatabaseInterface> DatabaseFactory::create(std::string name)
{
  if (name == "MySql") {
    QcInfoLogger::GetInstance() << "MySQL backend selected" << QcInfoLogger::endm;
#ifdef _WITH_MYSQL
    return std::make_unique<MySqlDatabase>();
#else
    BOOST_THROW_EXCEPTION(FatalException()
                          << errinfo_details("MySQL was not available during the compilation of the QC"));
#endif
  } else if (name == "CCDB") {
    // TODO check if CCDB installed
    QcInfoLogger::GetInstance() << "CCDB backend selected" << QcInfoLogger::endm;
    return std::make_unique<CcdbDatabase>();
  } else {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("No database named " + name));
  }
  return nullptr;
}

} // namespace repository
} // namespace quality_control
} // namespace o2
