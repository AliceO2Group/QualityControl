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

ClassImp(o2::quality_control::core::MonitorObject)

namespace o2 {
namespace quality_control {
namespace core {

constexpr char MonitorObject::SYSTEM_OBJECT_PUBLICATION_LIST[];

MonitorObject::MonitorObject()
  : TObject(), mName(""), mQuality(Quality::Null), mObject(nullptr), mTaskName(""), mIsOwner(true)
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
    mQuality(Quality::Null),
    mObject(object),
    mTaskName(taskName),
    mIsOwner(true)
{

}

} // namespace core
} // namespace quality_control
} // namespace o2
