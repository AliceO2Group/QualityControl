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

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <TObjString.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

// fixme: unify with testObjectManager?
BOOST_AUTO_TEST_CASE(publisher_test)
{
  std::string taskName = "test";
  std::string detectorName = "TST";
  std::string consulUrl = "invalid";
  ObjectsManager objectsManager(taskName, detectorName, consulUrl, 0, true);
  TObjString s("content");
  objectsManager.startPublishing(&s);

  TObjString* s2 = (TObjString*)(objectsManager.getMonitorObject("content")->getObject());
  BOOST_CHECK_EQUAL(s.GetString(), s2->GetString());
  MonitorObject* mo = nullptr;
  BOOST_CHECK_THROW(mo = objectsManager.getMonitorObject("test"), AliceO2::Common::ObjectNotFoundError);
  BOOST_CHECK_EQUAL(mo, nullptr);
  mo = objectsManager.getMonitorObject("content");
}

} // namespace o2::quality_control::core
