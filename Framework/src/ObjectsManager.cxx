// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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

const std::string ObjectsManager::gDrawOptionsKey = "drawOptions";
const std::string ObjectsManager::gDisplayHintsKey = "displayHints";

ObjectsManager::ObjectsManager(std::string taskName, std::string taskClass, std::string detectorName, std::string consulUrl, int parallelTaskID, bool noDiscovery)
  : mTaskName(taskName), mTaskClass(taskClass), mDetectorName(detectorName), mUpdateServiceDiscovery(false)
{
  mMonitorObjects = std::make_unique<MonitorObjectCollection>();
  mMonitorObjects->SetOwner(true);
  mMonitorObjects->SetName(taskName.c_str());

  // register with the discovery service
  if (!noDiscovery && !consulUrl.empty()) {
    std::string uniqueTaskID = taskName + "_" + std::to_string(parallelTaskID);
    mServiceDiscovery = std::make_unique<ServiceDiscovery>(consulUrl, taskName, uniqueTaskID);
  } else {
    ILOG(Warning, Ops) << "Service Discovery disabled" << ENDM;
    mServiceDiscovery = nullptr;
  }
}

ObjectsManager::~ObjectsManager() = default;

void ObjectsManager::startPublishing(TObject* object)
{
  if (mMonitorObjects->FindObject(object->GetName()) != nullptr) {
    ILOG(Warning, Support) << "Object is already being published (" << object->GetName() << ")" << ENDM;
    BOOST_THROW_EXCEPTION(DuplicateObjectError() << errinfo_object_name(object->GetName()));
  }
  auto* newObject = new MonitorObject(object, mTaskName, mTaskClass, mDetectorName);
  newObject->setIsOwner(false);
  newObject->setActivity(mActivity);
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
    if (mo) {
      objects += mo->getPath() + ",";
    } else {
      ILOG(Error, Devel) << "ObjectsManager::updateServiceDiscovery : dynamic_cast returned nullptr." << ENDM;
    }
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

void ObjectsManager::stopPublishing(const string& objectName)
{
  auto* mo = dynamic_cast<MonitorObject*>(getMonitorObject(objectName));
  mMonitorObjects->Remove(mo);
}

bool ObjectsManager::isBeingPublished(const string& name)
{
  return (mMonitorObjects->FindObject(name.c_str()) != nullptr);
}

MonitorObject* ObjectsManager::getMonitorObject(std::string objectName)
{
  TObject* object = mMonitorObjects->FindObject(objectName.c_str());
  if (object == nullptr) {
    ILOG(Error, Ops) << "ObjectsManager: Unable to find object \"" << objectName << "\"" << ENDM;
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(objectName));
  }
  return dynamic_cast<MonitorObject*>(object);
}

MonitorObject* ObjectsManager::getMonitorObject(size_t index)
{
  TObject* object = mMonitorObjects->At(index);
  if (object == nullptr) {
    ILOG(Error, Ops) << "ObjectsManager: Unable to find object at index \"" << index << "\"" << ENDM;
    string fakeName = "at index " + to_string(index);
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(fakeName));
  }
  return dynamic_cast<MonitorObject*>(object);
}

MonitorObjectCollection* ObjectsManager::getNonOwningArray() const
{
  return new MonitorObjectCollection(*mMonitorObjects);
}

void ObjectsManager::addMetadata(const std::string& objectName, const std::string& key, const std::string& value)
{
  MonitorObject* mo = getMonitorObject(objectName);
  mo->addMetadata(key, value);
  ILOG(Debug, Devel) << "Added metadata on " << objectName << " : " << key << " -> " << value << ENDM;
}

void ObjectsManager::addOrUpdateMetadata(const std::string& objectName, const std::string& key, const std::string& value)
{
  MonitorObject* mo = getMonitorObject(objectName);
  mo->addOrUpdateMetadata(key, value);
  ILOG(Debug, Devel) << "Added/Modified metadata on " << objectName << " : " << key << " -> " << value << ENDM;
}

size_t ObjectsManager::getNumberPublishedObjects()
{
  return mMonitorObjects->GetLast() + 1; // GetLast returns the index
}

void ObjectsManager::setDefaultDrawOptions(const std::string& objectName, const std::string& options)
{
  MonitorObject* mo = getMonitorObject(objectName);
  mo->addOrUpdateMetadata(gDrawOptionsKey, options);
}

void ObjectsManager::setDefaultDrawOptions(TObject* obj, const std::string& options)
{
  MonitorObject* mo = getMonitorObject(obj->GetName());
  mo->addOrUpdateMetadata(gDrawOptionsKey, options);
}

void ObjectsManager::setDisplayHint(const std::string& objectName, const std::string& hints)
{
  MonitorObject* mo = getMonitorObject(objectName);
  mo->addOrUpdateMetadata(gDisplayHintsKey, hints);
}

void ObjectsManager::setDisplayHint(TObject* obj, const std::string& hints)
{
  MonitorObject* mo = getMonitorObject(obj->GetName());
  mo->addOrUpdateMetadata(gDisplayHintsKey, hints);
}

const Activity& ObjectsManager::getActivity() const
{
  return mActivity;
}

void ObjectsManager::setActivity(const Activity& activity)
{
  mActivity = activity;
  // update the activity of all the objects
  for (auto tobj : *mMonitorObjects) {
    MonitorObject* mo = dynamic_cast<MonitorObject*>(tobj);
    mo->setActivity(activity);
  }
}

} // namespace o2::quality_control::core
