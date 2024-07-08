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
/// \file   CheckInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CheckInterface.h"
#include "QualityControl/ReferenceUtils.h"
#include "QualityControl/MonitorObject.h"

#include <TClass.h>

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control::checker
{

std::string CheckInterface::getAcceptedType() { return "TObject"; }

bool CheckInterface::isObjectCheckable(const std::shared_ptr<MonitorObject> mo)
{
  return isObjectCheckable(mo.get());
}

bool CheckInterface::isObjectCheckable(const MonitorObject* mo)
{
  TObject* encapsulated = mo->getObject();

  return encapsulated->IsA()->InheritsFrom(getAcceptedType().c_str());
}

void CheckInterface::configure()
{
  // noop, override it if you want.
}

void CheckInterface::reset()
{
  // noop, override it if you want.
}

void CheckInterface::startOfActivity(const Activity& activity)
{
  // noop, override it if you want.
}

void CheckInterface::endOfActivity(const Activity& activity)
{
  // noop, override it if you want.
}

shared_ptr<MonitorObject> CheckInterface::retrieveReference(std::string path, Activity referenceActivity)
{
  return o2::quality_control::checker::getReferencePlot(mDatabase.get(), path, referenceActivity);
}

} // namespace o2::quality_control::checker
