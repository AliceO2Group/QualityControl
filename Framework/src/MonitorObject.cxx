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
/// \file   MonitorObject.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/MonitorObject.h"

#include <iostream>
#include <Common/Exceptions.h>
#include "QualityControl/RepoPathUtils.h"

ClassImp(o2::quality_control::core::MonitorObject)

  using namespace std;

namespace o2::quality_control::core
{

MonitorObject::MonitorObject() : TObject(), mObject(nullptr), mTaskName(""), mDetectorName(""), mRunNumber(0), mIsOwner(true)
{
}

MonitorObject::MonitorObject(TObject* object, const std::string& taskName, const std::string& detectorName, int runNumber)
  : TObject(), mObject(object), mTaskName(taskName), mDetectorName(detectorName), mRunNumber(runNumber), mIsOwner(true)
{
}

MonitorObject::~MonitorObject()
{
  if (mIsOwner && mObject != nullptr) {
    delete mObject;
  }
}

void MonitorObject::Draw(Option_t* option) { mObject->Draw(option); }

TObject* MonitorObject::DrawClone(Option_t* option) const
{
  auto* clone = new MonitorObject();
  clone->setTaskName(this->getTaskName());
  clone->setObject(mObject->DrawClone(option));
  return clone;
}

const std::string MonitorObject::getName() const
{
  return string(GetName());
}

const char* MonitorObject::GetName() const
{
  if (mObject == nullptr) {
    cerr << "MonitorObject::getName() : No object in this MonitorObject, returning empty string";
    return "";
  }
  return mObject->GetName();
}

void MonitorObject::addMetadata(std::string key, std::string value)
{
  mUserMetadata.insert({ key, value });
}

void MonitorObject::addMetadata(std::map<std::string, std::string> pairs)
{
  // we do not use "merge" because it would ignore the items whose key already exist in mUserMetadata.
  mUserMetadata.insert(pairs.begin(), pairs.end());
}

const std::map<std::string, std::string>& MonitorObject::getMetadataMap() const
{
  return mUserMetadata;
}

void MonitorObject::updateMetadata(std::string key, std::string value)
{
  if (mUserMetadata.count(key) > 0) {
    mUserMetadata[key] = value;
  }
}

void MonitorObject::addOrUpdateMetadata(std::string key, std::string value)
{
  if (mUserMetadata.count(key) > 0) {
    mUserMetadata[key] = value;
  } else {
    mUserMetadata.insert({ key, value });
  }
}

std::string MonitorObject::getPath() const
{
  return RepoPathUtils::getMoPath(this);
}

const string& MonitorObject::getDescription() const
{
  return mDescription;
}

void MonitorObject::setDescription(const string& description)
{
  MonitorObject::mDescription = description;
}

int MonitorObject::getRunNumber() const
{
  return mRunNumber;
}

void MonitorObject::setRunNumber(int runNumber)
{
  MonitorObject::mRunNumber = runNumber;
}

} // namespace o2::quality_control::core
