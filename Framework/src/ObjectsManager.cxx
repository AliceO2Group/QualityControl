// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ObjectsManager.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"
#include "Common/Exceptions.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2::quality_control::core;
using namespace AliceO2::Common;
using namespace std;

namespace o2
{
namespace quality_control
{
namespace core
{

ObjectsManager::ObjectsManager(TaskConfig& taskConfig) : mTaskName(taskConfig.taskName)
{
  startPublishing(&mObjectsList, MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST);
  mMonitorObjects.SetOwner(true);
}

ObjectsManager::~ObjectsManager()
{
}

void ObjectsManager::startPublishing(TObject* object, std::string objectName)
{
  std::string nonEmptyName = objectName.empty() ? object->GetName() : objectName;
  auto* newObject = new MonitorObject(nonEmptyName, object, mTaskName);
  newObject->setIsOwner(false);
  mMonitorObjects.Add(newObject);

  // update index
  if (objectName != MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST) {
    UpdateIndex(nonEmptyName);
  }
}

void ObjectsManager::UpdateIndex(const string& nonEmptyName)
{
  string newString = this->mObjectsList.GetString().Data();
  newString += nonEmptyName;
  newString += ",";
  this->mObjectsList.SetString(newString.c_str());
}

Quality ObjectsManager::getQuality(std::string objectName)
{
  if (mMonitorObjects.count(objectName) == 0) {
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(objectName));
  }
  MonitorObject* mo = getMonitorObject(objectName);
  return mo->getQuality();
}
// fixme: keep user informed, that giving the same names for their objects is a bad idea
void ObjectsManager::addCheck(const std::string& objectName, const std::string& checkName,
                              const std::string& checkClassName, const std::string& checkLibraryName)
{
  MonitorObject* mo = getMonitorObject(objectName);
  mo->addCheck(checkName, checkClassName, checkLibraryName);

  QcInfoLogger::GetInstance() << "Added check : " << objectName << " , " << checkName << " , " << checkClassName
                              << " , " << checkLibraryName << infologger::endm;
}

MonitorObject* ObjectsManager::getMonitorObject(std::string objectName)
{
  TObject* mo = mMonitorObjects.FindObject(objectName.c_str());

  if (mo) {
    return dynamic_cast<MonitorObject*>(mo);
  } else {
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(objectName));
  }
}

TObject* ObjectsManager::getObject(std::string objectName)
{
  MonitorObject* mo = getMonitorObject(objectName);
  return mo->getObject();
}

void ObjectsManager::addCheck(const TObject* object, const std::string& checkName, const std::string& checkClassName,
                              const std::string& checkLibraryName)
{
  addCheck(object->GetName(), checkName, checkClassName, checkLibraryName);
}

} // namespace core
} // namespace quality_control
} // namespace o2
