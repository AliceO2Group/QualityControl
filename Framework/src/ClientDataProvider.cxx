///
/// \file   ClientDataProvider.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ClientDataProvider.h"
#include "QualityControl/DatabaseFactory.h"

using namespace std;
using namespace o2::quality_control;

namespace o2 {
namespace quality_control {
namespace client {

ClientDataProvider::ClientDataProvider()
{
  // TODO use the configuration system
  database = repository::DatabaseFactory::create("MySql");
  database->connect("localhost", "quality_control", "qc_user", "qc_user");
}

ClientDataProvider::~ClientDataProvider()
{
  database->disconnect();
}

TObject* ClientDataProvider::getObject(std::string taskName, std::string objectName)
{
  core::MonitorObject *mo = database->retrieve(taskName, objectName);
  if(!mo) {
    return nullptr;
  }
  mo->setIsOwner(false);

  TObject* result = mo->getObject();
  delete mo;
  return result;
}

std::vector<std::string> ClientDataProvider::getListOfActiveTasks()
{
  std::vector<std::string> result;

  // TODO use a proper information service
  result = database->getListOfTasksWithPublications();

//  cout << "List of tasks: " << endl;
//  for (auto const &r : result)
//  {
//      std::cout << "  " << r << std::endl;
//  }

  return result;
}

std::string ClientDataProvider::getTaskStatus(std::string taskName)
{
  // TODO
  return "";
}

std::vector<std::string> ClientDataProvider::getPublicationList(std::string taskName)
{
  std::vector<std::string> result;

  // TODO use a proper information service
  result = database->getPublishedObjectNames(taskName);

//  cout << "List of objects for task " << taskName << " : " << endl;
//  for (auto const &r : result)
//  {
//      std::cout << "  " << r << std::endl;
//  }

  return result;
}

} // namespace client
} // namespace quality_control
} // namespace o2
