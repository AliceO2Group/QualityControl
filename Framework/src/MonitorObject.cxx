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

ClassImp(o2::quality_control::core::MonitorObject)

using namespace std;

namespace o2::quality_control::core
{

MonitorObject::MonitorObject() : TObject(), mObject(nullptr), mTaskName(""), mDetectorName(""), mIsOwner(true) {}

MonitorObject::~MonitorObject()
{
  if (mIsOwner && mObject != nullptr) {
    delete mObject;
  }
}

MonitorObject::MonitorObject(TObject* object, const std::string& taskName, const std::string& detectorName)
  : TObject(), mObject(object), mTaskName(taskName), mDetectorName(detectorName), mIsOwner(true)
{
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

void MonitorObject::setQualityForCheck(std::string checkName, Quality quality)
{
  auto check = mChecks.find(checkName);
  if (check != mChecks.end()) {
    check->second.result = quality;
    mChecks[checkName] = check->second;
  } else {
    throw AliceO2::Common::ObjectNotFoundError();
  }
}

CheckDefinition MonitorObject::getCheck(std::string checkName) const
{
  if (mChecks.find(checkName) != mChecks.end()) {
    return mChecks.at(checkName);
  } else {
    throw AliceO2::Common::ObjectNotFoundError();
  }
}

void MonitorObject::addCheck(const std::string name, const std::string checkClassName,
                             const std::string checkLibraryName)
{
  CheckDefinition check;
  check.name = name;
  check.libraryName = checkLibraryName;
  check.className = checkClassName;
  mChecks[name] = check;
}

void MonitorObject::addOrReplaceCheck(std::string checkName, CheckDefinition check) { mChecks[checkName] = check; }

Quality MonitorObject::getQuality() const
{
  Quality global = Quality::Null;

  for (const auto& checkPair : getChecks()) {
    const CheckDefinition& checkDef = checkPair.second;
    if (checkDef.result != Quality::Null) {
      if (checkDef.result.isWorstThan(global) || global == Quality::Null) {
        global = checkDef.result;
      }
    }
  }

  return global;
}

void MonitorObject::addMetadata(std::string key, std::string value)
{
  mUserMetadata[key] = value;
}

std::map<std::string, std::string> MonitorObject::getMetadataMap() const
{
  return mUserMetadata;
}

std::string MonitorObject::getPath() const
{
  string path = "qc/" + getDetectorName() + "/" + getTaskName() + "/" + getName();
  cout << "" << "\n" << "*** " << "PATH: " << path << "\n" << endl;
  return path;
}

} // namespace o2::quality_control::core
