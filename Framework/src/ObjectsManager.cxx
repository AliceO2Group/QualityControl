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

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/MonitorObjectCollection.h"
#include <Common/Exceptions.h>
#include <TObjArray.h>

using namespace o2::quality_control::core;
using namespace AliceO2::Common;
using namespace std;

namespace o2::quality_control::core
{

ObjectsManager::ObjectsManager(TaskConfig& taskConfig, bool noDiscovery) : mTaskConfig(taskConfig), mUpdateServiceDiscovery(false)
{
  mMonitorObjects = std::make_unique<MonitorObjectCollection>();
  mMonitorObjects->SetOwner(true);

  // register with the discovery service
  if (!noDiscovery) {
    mServiceDiscovery = std::make_unique<ServiceDiscovery>(taskConfig.consulUrl, taskConfig.taskName);
  } else {
    QcInfoLogger::GetInstance() << "Service Discovery disabled" << infologger::endm;
    mServiceDiscovery = nullptr;
  }
}

ObjectsManager::~ObjectsManager() = default;

void ObjectsManager::startPublishing(TObject* object)
{
  if (mMonitorObjects->FindObject(object->GetName()) != 0) {
    QcInfoLogger::GetInstance() << "Object already being published (" << object->GetName() << ")"
                                << infologger::endm;
    BOOST_THROW_EXCEPTION(DuplicateObjectError() << errinfo_object_name(object->GetName()));
  }
  auto* newObject = new MonitorObject(object, mTaskConfig.taskName, mTaskConfig.detectorName);
  newObject->setIsOwner(false);
  mMonitorObjects->Add(newObject);
  mUpdateServiceDiscovery = true;
}

void ObjectsManager::updateServiceDiscovery()
{
  if (!mUpdateServiceDiscovery || mServiceDiscovery == nullptr) {
    return;
  }
  // prepare the string of comma separated objects and publish it
  string objects;
  for (auto tobj : *mMonitorObjects) {
    MonitorObject* mo = dynamic_cast<MonitorObject*>(tobj);
    objects += mo->getPath() + ",";
  }
  objects.pop_back();
  mServiceDiscovery->_register(objects);
  mUpdateServiceDiscovery = false;
}

void ObjectsManager::removeAllFromServiceDiscovery()
{
  if (mServiceDiscovery == nullptr) {
    return;
  }
  mServiceDiscovery->_register("");
  mUpdateServiceDiscovery = true;
}

void ObjectsManager::stopPublishing(TObject* object)
{
  stopPublishing(object->GetName());
}

void ObjectsManager::stopPublishing(const string& name)
{
  auto* mo = dynamic_cast<MonitorObject*>(mMonitorObjects->FindObject(name.data()));
  if (mo == nullptr) {
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(name));
  }
  mMonitorObjects->Remove(mo);
}

bool ObjectsManager::isBeingPublished(const string& name)
{
  return (mMonitorObjects->FindObject(name.c_str()) != nullptr);
}

MonitorObject* ObjectsManager::getMonitorObject(std::string objectName)
{
  TObject* mo = mMonitorObjects->FindObject(objectName.c_str());

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

MonitorObjectCollection* ObjectsManager::getNonOwningArray() const
{
  return new MonitorObjectCollection(*mMonitorObjects);
}

void ObjectsManager::addMetadata(const std::string& objectName, const std::string& key, const std::string& value)
{
  MonitorObject* mo = getMonitorObject(objectName);
  mo->addMetadata(key, value);
  QcInfoLogger::GetInstance() << "Added metadata on " << objectName << " : " << key << " -> " << value << infologger::endm;
}

int ObjectsManager::getNumberPublishedObjects()
{
  return mMonitorObjects->GetLast() + 1; // GetLast returns the index
}

} // namespace o2::quality_control::core
