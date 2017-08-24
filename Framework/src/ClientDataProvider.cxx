///
/// \file   ClientDataProvider.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ClientDataProvider.h"
#include "QualityControl/DatabaseFactory.h"

using namespace std;
using namespace AliceO2::QualityControl;

namespace AliceO2 {
namespace QualityControl {
namespace Client {

ClientDataProvider::ClientDataProvider()
{
  // TODO use the configuration system
  database = Repository::DatabaseFactory::create("MySql");
  database->connect("qc_user", "qc_user");
}

ClientDataProvider::~ClientDataProvider()
{
  database->disconnect();
  delete database;
}

TObject* ClientDataProvider::getObject(std::string taskName, std::string objectName)
{
  Core::MonitorObject *mo = database->retrieve(taskName, objectName);
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

} // namespace Client
} // namespace QualityControl
} // namespace AliceO2
