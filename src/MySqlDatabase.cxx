///
/// \file   MySqlDatabase.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/MySqlDatabase.h"

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

MySqlDatabase::MySqlDatabase()
{
}

MySqlDatabase::~MySqlDatabase()
{
}

void MySqlDatabase::Connect()
{

}

void MySqlDatabase::Store(AliceO2::QualityControl::Core::MonitorObject *mo)
{
  ;
}

AliceO2::QualityControl::Core::MonitorObject* MySqlDatabase::Retrieve(std::string name)
{
  return nullptr;
}

void MySqlDatabase::Disconnect()
{
  ;
}

} // namespace Repository
} // namespace QualityControl
} // namespace AliceO2
