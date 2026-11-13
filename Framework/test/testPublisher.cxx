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
/// \file   testPublisher.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"

#include <TObjString.h>
#include <catch_amalgamated.hpp>

using namespace std;
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

// fixme: unify with testObjectManager?
TEST_CASE("publisher_test")
{
  std::string taskName = "test";
  std::string detectorName = "TST";
  std::string consulUrl = "invalid";
  ObjectsManager objectsManager(taskName, "taskClass", detectorName, 0);
  TObjString s("content");
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);

  TObjString* s2 = (TObjString*)(objectsManager.getMonitorObject("content")->getObject());
  CHECK(s.GetString() == s2->GetString());
  MonitorObject* mo = nullptr;
  CHECK_THROWS_AS(mo = objectsManager.getMonitorObject("test"), AliceO2::Common::ObjectNotFoundError);
  CHECK(mo == nullptr);
  mo = objectsManager.getMonitorObject("content");
}

} // namespace o2::quality_control::core
