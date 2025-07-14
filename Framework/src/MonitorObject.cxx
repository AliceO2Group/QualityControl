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
/// \file   MonitorObject.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/MonitorObject.h"
#include <TObject.h>
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/QcInfoLogger.h"

#include <iostream>
#include <iterator>
#include <optional>

using namespace std;

namespace o2::quality_control::core
{

MonitorObject::MonitorObject()
  : TObject{},
    mIsOwner{ true }
{
  mActivity.mProvenance = "qc";
  mActivity.mId = 0;
  mActivity.mValidity = gInvalidValidityInterval;
}

MonitorObject::MonitorObject(TObject* object, const std::string& taskName, const std::string& taskClass, const std::string& detectorName, int runNumber, const std::string& periodName, const std::string& passName, const std::string& provenance)
  : TObject{},
    mObject{ object },
    mTaskName{ taskName },
    mTaskClass{ taskClass },
    mDetectorName{ detectorName },
    mActivity{ runNumber, "NONE", periodName, passName, provenance, gInvalidValidityInterval },
    mIsOwner{ true }
{
}

MonitorObject::MonitorObject(const MonitorObject& other)
  : TObject{ other },
    mObject{},
    mTaskName{ other.mTaskName },
    mTaskClass{ other.mTaskClass },
    mDetectorName{ other.mDetectorName },
    mUserMetadata{ other.mUserMetadata },
    mDescription{ other.mDescription },
    mActivity{ other.mActivity },
    mCreateMovingWindow{ other.mCreateMovingWindow }
{
  cloneAndSetObject(other);
}

MonitorObject& MonitorObject::operator=(const MonitorObject& other)
{
  TObject::operator=(other);
  mTaskName = other.mTaskName;
  mTaskClass = other.mTaskClass;
  mDetectorName = other.mDetectorName;
  mUserMetadata = other.mUserMetadata;
  mDescription = other.mDescription;
  mActivity = other.mActivity;
  mCreateMovingWindow = other.mCreateMovingWindow;
  cloneAndSetObject(other);

  return *this;
}

void MonitorObject::Copy(TObject& object) const
{
  static_cast<MonitorObject&>(object) = *this;
}

MonitorObject::~MonitorObject()
{
  releaseObject();
}

void MonitorObject::Draw(Option_t* option)
{
  if (mObject) {
    mObject->Draw(option);
  } else {
    ILOG(Error, Devel) << "MonitorObject::Draw() : You are trying to draw MonitorObject with no internal TObject" << ENDM;
  }
}

TObject* MonitorObject::DrawClone(Option_t* option) const
{
  if (!mObject) {
    ILOG(Error, Devel) << "MonitorObject::DrawClone() : You are trying to draw MonitorObject with no internal TObject" << ENDM;
    return nullptr;
  }

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
  if (!mObject) {
    ILOG(Error, Ops) << "MonitorObject::getName() : No object in this MonitorObject, returning empty string" << ENDM;
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

std::optional<std::string> MonitorObject::getMetadata(const std::string& key)
{
  if (const auto foundIt = mUserMetadata.find(key); foundIt != std::end(mUserMetadata)) {
    return foundIt->second;
  }
  return std::nullopt;
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
  mDescription = description;
}

const Activity& MonitorObject::getActivity() const
{
  return mActivity;
}

Activity& MonitorObject::getActivity()
{
  return mActivity;
}

void MonitorObject::setActivity(const Activity& activity)
{
  MonitorObject::mActivity = activity;
}

void MonitorObject::updateActivity(int runNumber, const std::string& periodName, const std::string& passName, const std::string& provenance)
{
  mActivity.mId = runNumber;
  mActivity.mPeriodName = periodName;
  mActivity.mPassName = passName;
  mActivity.mProvenance = provenance;
}

void MonitorObject::setValidity(ValidityInterval validityInterval)
{
  mActivity.mValidity = validityInterval;
}

void MonitorObject::updateValidity(validity_time_t value)
{
  mActivity.mValidity.update(value);
}

std::string MonitorObject::getFullName() const
{
  return getTaskName() + "/" + getName();
}

TObject* MonitorObject::getObject() const
{
  return mObject.get();
}

void MonitorObject::setObject(TObject* object)
{
  releaseObject();
  mObject.reset(object);
}

bool MonitorObject::isIsOwner() const
{
  return mIsOwner;
}

void MonitorObject::setIsOwner(bool isOwner)
{
  mIsOwner = isOwner;
}

const std::string& MonitorObject::getTaskName() const
{
  return mTaskName;
}

void MonitorObject::setTaskName(const std::string& taskName)
{
  mTaskName = taskName;
}

const std::string& MonitorObject::getDetectorName() const
{
  return mDetectorName;
}

void MonitorObject::setDetectorName(const std::string& detectorName)
{
  mDetectorName = detectorName;
}

ValidityInterval MonitorObject::getValidity() const
{
  return mActivity.mValidity;
}

const string& MonitorObject::getTaskClass() const
{
  return mTaskClass;
}

void MonitorObject::setTaskClass(const string& taskClass)
{
  MonitorObject::mTaskClass = taskClass;
}

void MonitorObject::setCreateMovingWindow(bool flag)
{
  mCreateMovingWindow = flag;
}

bool MonitorObject::getCreateMovingWindow() const
{
  return mCreateMovingWindow;
}

void MonitorObject::releaseObject()
{
  if (!mIsOwner) {
    void(mObject.release());
  }
}

void MonitorObject::cloneAndSetObject(const MonitorObject& other)
{
  releaseObject();

  if (auto* otherObject = other.getObject(); otherObject != nullptr && other.isIsOwner()) {
    mObject.reset(otherObject->Clone());
  } else {
    mObject.reset(otherObject);
  }
  mIsOwner = other.isIsOwner();
}

} // namespace o2::quality_control::core
