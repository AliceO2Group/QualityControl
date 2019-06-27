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

namespace o2::quality_control::core
{

ObjectsManager::ObjectsManager(TaskConfig& taskConfig) : mTaskConfig(taskConfig), mUpdateServiceDiscovery(false)
{
  mMonitorObjects.SetOwner(true);

  // register with the discovery service
  mServiceDiscovery = std::make_unique<ServiceDiscovery>(taskConfig.consulUrl, taskConfig.taskName);
}

ObjectsManager::~ObjectsManager()
{
}

void ObjectsManager::startPublishing(TObject* object)
{
  if (mMonitorObjects.FindObject(object->GetName()) != 0) {
    QcInfoLogger::GetInstance() << "Object already being published (" << object->GetName() << ")"
                                << infologger::endm;
    BOOST_THROW_EXCEPTION(DuplicateObjectError() << errinfo_object_name(object->GetName()));
  }
  auto* newObject = new MonitorObject(object, mTaskConfig.taskName);
  newObject->setIsOwner(false);
  mMonitorObjects.Add(newObject);
  mUpdateServiceDiscovery = true;
}

void ObjectsManager::updateServiceDiscovery()
{
  if (!mUpdateServiceDiscovery || mServiceDiscovery == nullptr) {
    return;
  }
  // prepare the string of comma separated objects and publish it
  string objects;
  for (auto mo : mMonitorObjects) {
    objects += mTaskConfig.taskName + "/" + mo->GetName() + ",";
  }
  objects.pop_back();
  mServiceDiscovery->_register(objects);
  mUpdateServiceDiscovery = false;
}

void ObjectsManager::stopPublishing(TObject* object)
{
  stopPublishing(object->GetName());
}

void ObjectsManager::stopPublishing(const string& name)
{
  auto* mo = dynamic_cast<MonitorObject*>(mMonitorObjects.FindObject(name.data()));
  if (mo == nullptr) {
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(name));
  }
  mMonitorObjects.Remove(mo);
}

Quality ObjectsManager::getQuality(std::string objectName)
{
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

  if (mo != nullptr) {
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

void ObjectsManager::addMetadata(const std::string& objectName, const std::string& key, const std::string& value)
{
  MonitorObject* mo = getMonitorObject(objectName);
  mo->addMetadata(key, value);
  QcInfoLogger::GetInstance() << "Added metadata on " << objectName << " : " << key << " -> " << value << infologger::endm;
}

int ObjectsManager::getNumberPublishedObjects()
{
  return mMonitorObjects.GetLast() + 1; // GetLast returns the index
}

} // namespace o2::quality_control::core
