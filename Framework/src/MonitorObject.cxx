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

#include <iostream>
#include <utility>
#include <Common/Exceptions.h>
#include "QualityControl/RepoPathUtils.h"

ClassImp(o2::quality_control::core::MonitorObject)

  using namespace std;

namespace o2::quality_control::core
{

MonitorObject::MonitorObject()
  : TObject(),
    mObject(nullptr),
    mTaskName(""),
    mDetectorName(""),
    mIsOwner(true)
{
  mActivity.mProvenance = "qc";
  mActivity.mId = 0;
  mActivity.mValidity = gInvalidValidityInterval;
}

MonitorObject::MonitorObject(TObject* object, const std::string& taskName, const std::string& taskClass, const std::string& detectorName, int runNumber, const std::string& periodName, const std::string& passName, const std::string& provenance)
  : TObject(),
    mObject(object),
    mTaskName(taskName),
    mTaskClass(taskClass),
    mDetectorName(detectorName),
    mActivity(runNumber, 0, periodName, passName, provenance, gInvalidValidityInterval),
    mIsOwner(true)
{
}

MonitorObject::~MonitorObject()
{
  if (mIsOwner) {
    delete mObject;
    mObject = nullptr;
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
    cerr << "MonitorObject::getName() : No object in this MonitorObject, returning empty string" << endl;
    static char empty[] = "";
    return empty;
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

} // namespace o2::quality_control::core
