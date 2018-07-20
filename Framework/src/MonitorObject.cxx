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
#include "Common/Exceptions.h"

ClassImp(o2::quality_control::core::MonitorObject)

namespace o2 {
namespace quality_control {
namespace core {

constexpr char MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST[];

MonitorObject::MonitorObject()
  : TObject(), mName(""), mObject(nullptr), mTaskName(""), mIsOwner(true)
{
}

MonitorObject::~MonitorObject()
{
  if (mIsOwner && mObject != nullptr) {
    delete mObject;
  }
}

MonitorObject::MonitorObject(const std::string &name, TObject *object, const std::string &taskName)
  : TObject(),
    mName(name),
    mObject(object),
    mTaskName(taskName),
    mIsOwner(true)
{
}

void 	MonitorObject::Draw(Option_t *option)
{
  mObject->Draw(option);
}

TObject* MonitorObject::DrawClone(Option_t *option) const
{
  auto* clone = new MonitorObject();
  clone->setName(this->getName());
  clone->setTaskName(this->getTaskName());
  clone->setObject(mObject->DrawClone(option));
  return clone;
}

void MonitorObject::setQualityForCheck(std::string checkName, Quality quality) {
  auto check = mChecks.find(checkName);
  if(check != mChecks.end()) {
    check->second.result = quality;
    mChecks[checkName] = check->second;
  } else {
    throw AliceO2::Common::ObjectNotFoundError();
  }
}

CheckDefinition MonitorObject::getCheck(std::string checkName) const
{
  if(mChecks.find(checkName) != mChecks.end()) {
    return mChecks.at(checkName);
  } else {
    throw AliceO2::Common::ObjectNotFoundError();
  }
}

void MonitorObject::addCheck(const std::string name, const std::string checkClassName, const std::string checkLibraryName)
{
  CheckDefinition check;
  check.name = name;
  check.libraryName = checkLibraryName;
  check.className = checkClassName;
  mChecks[name] = check;
}

void MonitorObject::addOrUpdateCheck(std::string checkName, CheckDefinition check)
{
  mChecks[checkName] = check;
}

const Quality MonitorObject::getQuality() const
{
  Quality global = Quality::Null;

  for(const auto &checkDef : getChecks()) {
    if(checkDef.second.result != Quality::Null) {
      if(checkDef.second.result.isWorstThan(global) || global == Quality::Null) {
        global = checkDef.second.result;
      }
    }
  }

  return global;
}

} // namespace core
} // namespace quality_control
} // namespace o2
