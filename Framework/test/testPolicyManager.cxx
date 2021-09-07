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
/// \file   testPolicyManager.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/UpdatePolicyManager.h"
#include <DataSampling/DataSampling.h>
#include <Common/Exceptions.h>
#include <TH1F.h>

#define BOOST_TEST_MODULE Check test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::checker;
using namespace std;
using namespace o2::framework;
using namespace o2::header;
using namespace AliceO2::Common;

BOOST_AUTO_TEST_CASE(test_basic_isready)
{
  UpdatePolicyManager updatePolicyManager;

  // init
  updatePolicyManager.addPolicy("actor1", UpdatePolicyType::OnAny, { "object1" }, false, false);

  // this is like 1 iteration of the run() :
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  // policy must be false now because we have already processed this actor
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);

  updatePolicyManager.updateGlobalRevision();
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);

  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
}

BOOST_AUTO_TEST_CASE(test_basic_isready2)
{
  UpdatePolicyManager updatePolicyManager;

  // init
  updatePolicyManager.addPolicy("actor1", UpdatePolicyType::OnAny, { "object1", "object2" }, false, false);
  updatePolicyManager.addPolicy("actor2", UpdatePolicyType::OnAny, { "object2", "object3" }, false, false);
  updatePolicyManager.addPolicy("actor3", UpdatePolicyType::OnAny, {}, false, false); // no objects listed
  updatePolicyManager.addPolicy("actor4", UpdatePolicyType::OnAny, {}, true, false);  // allMOs set

  // this is like 1 iteration of the run() :
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor3"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor4"), false);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor3");
  updatePolicyManager.updateActorRevision("actor4");
  // policy must be false now because we have already processed this actor
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  updatePolicyManager.updateGlobalRevision();

  // this is like 1 iteration of the run() :
  // get new data
  updatePolicyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor2");
  // policy must be false now because we have already processed this actor
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // this is like 1 iteration of the run() :
  // get new data
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnAll)
{
  UpdatePolicyManager updatePolicyManager;

  // init
  updatePolicyManager.addPolicy("actor1", UpdatePolicyType::OnAll, { "object1", "object2" }, false, false);
  updatePolicyManager.addPolicy("actor2", UpdatePolicyType::OnAll, { "object2", "object3" }, false, false);
  //  updatePolicyManager.addPolicy("actor3", UpdatePolicyType::OnAll, { }, false, false);
  //  updatePolicyManager.addPolicy("actor4", UpdatePolicyType::OnAll, { }, true, false);

  // iteration 1 of run() : get object1
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  //  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor3"), true);
  //  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor4"), true);
  updatePolicyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  updatePolicyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  //  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor3"), true);
  //  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor4"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  //  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor3"), true);
  //  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor4"), true);
  updatePolicyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  updatePolicyManager.updateObjectRevision("object2");
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnAny)
{
  UpdatePolicyManager updatePolicyManager;

  // init
  updatePolicyManager.addPolicy("actor1", UpdatePolicyType::OnAny, { "object1", "object2" }, false, false);
  updatePolicyManager.addPolicy("actor2", UpdatePolicyType::OnAny, { "object2", "object3" }, false, false);

  // iteration 1 of run() : get object1
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  updatePolicyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  updatePolicyManager.updateObjectRevision("object2");
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnAnyNonZero)
{
  UpdatePolicyManager updatePolicyManager;

  // init
  updatePolicyManager.addPolicy("actor1", UpdatePolicyType::OnAnyNonZero, { "object1", "object2" }, false, false);
  updatePolicyManager.addPolicy("actor2", UpdatePolicyType::OnAnyNonZero, { "object2", "object3" }, false, false);

  // iteration 1 of run() : get object1
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  updatePolicyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  updatePolicyManager.updateObjectRevision("object2");
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnEachSeparately)
{
  UpdatePolicyManager updatePolicyManager;

  // init
  updatePolicyManager.addPolicy("actor1", UpdatePolicyType::OnEachSeparately, { "object1", "object2" }, false, false);
  updatePolicyManager.addPolicy("actor2", UpdatePolicyType::OnEachSeparately, { "object2", "object3" }, false, false);

  // iteration 1 of run() : get object1
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  updatePolicyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  updatePolicyManager.updateObjectRevision("object1");
  updatePolicyManager.updateObjectRevision("object2");
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  updatePolicyManager.updateActorRevision("actor1");
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_errors)
{
  UpdatePolicyManager updatePolicyManager;

  // init
  updatePolicyManager.addPolicy("actor2", UpdatePolicyType::OnEachSeparately, { "object2", "object3" }, false, false);

  // get new data
  updatePolicyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_THROW(updatePolicyManager.isReady("actor1"), ObjectNotFoundError);
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), true);
  // update
  BOOST_CHECK_THROW(updatePolicyManager.updateActorRevision("actor1"), ObjectNotFoundError);
  updatePolicyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(updatePolicyManager.isReady("actor2"), false);
  updatePolicyManager.updateGlobalRevision();
}
