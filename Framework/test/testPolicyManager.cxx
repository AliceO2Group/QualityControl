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
/// \file   testPolicyManager.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/PolicyManager.h"
#include <DataSampling/DataSampling.h>
using namespace o2::utilities;
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
  PolicyManager policyManager;

  // init
  policyManager.addPolicy("actor1", "OnAny", { "object1" }, false, false);

  // this is like 1 iteration of the run() :
  // get new data
  policyManager.updateObjectRevision("object1");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  // update
  policyManager.updateActorRevision("actor1");
  // policy must be false now because we have already processed this actor
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);

  policyManager.updateGlobalRevision();
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);

  // get new data
  policyManager.updateObjectRevision("object1");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
}

BOOST_AUTO_TEST_CASE(test_basic_isready2)
{
  PolicyManager policyManager;

  // init
  policyManager.addPolicy("actor1", "OnAny", { "object1", "object2" }, false, false);
  policyManager.addPolicy("actor2", "OnAny", { "object2", "object3" }, false, false);
  policyManager.addPolicy("actor3", "OnAny", {}, false, false); // no objects listed
  policyManager.addPolicy("actor4", "OnAny", {}, true, false);  // allMOs set

  // this is like 1 iteration of the run() :
  // get new data
  policyManager.updateObjectRevision("object1");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor3"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor4"), false);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor3");
  policyManager.updateActorRevision("actor4");
  // policy must be false now because we have already processed this actor
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  policyManager.updateGlobalRevision();

  // this is like 1 iteration of the run() :
  // get new data
  policyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor2");
  // policy must be false now because we have already processed this actor
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // this is like 1 iteration of the run() :
  // get new data
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnAll)
{
  PolicyManager policyManager;

  // init
  policyManager.addPolicy("actor1", "OnAll", { "object1", "object2" }, false, false);
  policyManager.addPolicy("actor2", "OnAll", { "object2", "object3" }, false, false);
  //  policyManager.addPolicy("actor3", "OnAll", { }, false, false);
  //  policyManager.addPolicy("actor4", "OnAll", { }, true, false);

  // iteration 1 of run() : get object1
  // get new data
  policyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  //  BOOST_CHECK_EQUAL(policyManager.isReady("actor3"), true);
  //  BOOST_CHECK_EQUAL(policyManager.isReady("actor4"), true);
  policyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  policyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  //  BOOST_CHECK_EQUAL(policyManager.isReady("actor3"), true);
  //  BOOST_CHECK_EQUAL(policyManager.isReady("actor4"), true);
  // update
  policyManager.updateActorRevision("actor1");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  //  BOOST_CHECK_EQUAL(policyManager.isReady("actor3"), true);
  //  BOOST_CHECK_EQUAL(policyManager.isReady("actor4"), true);
  policyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  policyManager.updateObjectRevision("object1");
  policyManager.updateObjectRevision("object2");
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnAny)
{
  PolicyManager policyManager;

  // init
  policyManager.addPolicy("actor1", "OnAny", { "object1", "object2" }, false, false);
  policyManager.addPolicy("actor2", "OnAny", { "object2", "object3" }, false, false);

  // iteration 1 of run() : get object1
  // get new data
  policyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  policyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  policyManager.updateObjectRevision("object1");
  policyManager.updateObjectRevision("object2");
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnAnyNonZero)
{
  PolicyManager policyManager;

  // init
  policyManager.addPolicy("actor1", "OnAnyNonZero", { "object1", "object2" }, false, false);
  policyManager.addPolicy("actor2", "OnAnyNonZero", { "object2", "object3" }, false, false);

  // iteration 1 of run() : get object1
  // get new data
  policyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  policyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  // update
  policyManager.updateActorRevision("actor1");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  policyManager.updateObjectRevision("object1");
  policyManager.updateObjectRevision("object2");
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_check_policy_OnEachSeparately)
{
  PolicyManager policyManager;

  // init
  policyManager.addPolicy("actor1", "OnEachSeparately", { "object1", "object2" }, false, false);
  policyManager.addPolicy("actor2", "OnEachSeparately", { "object2", "object3" }, false, false);

  // iteration 1 of run() : get object1
  // get new data
  policyManager.updateObjectRevision("object1");
  // check whether data is ready
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 2 of run() : get object2
  // get new data
  policyManager.updateObjectRevision("object2");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 3 of run() : get object3
  // get new data
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();

  // iteration 4 of run() : get object1, 2, 3
  // get new data
  policyManager.updateObjectRevision("object1");
  policyManager.updateObjectRevision("object2");
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), true);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  policyManager.updateActorRevision("actor1");
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor1"), false);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();
}

BOOST_AUTO_TEST_CASE(test_errors)
{
  PolicyManager policyManager;

  // init
  BOOST_CHECK_THROW(
    policyManager.addPolicy("actor1", "OnTheMoon", { "object1", "object2" }, false, false), FatalException);
  policyManager.addPolicy("actor2", "OnEachSeparately", { "object2", "object3" }, false, false);

  // get new data
  policyManager.updateObjectRevision("object3");
  // check the policy
  BOOST_CHECK_THROW(policyManager.isReady("actor1"), ObjectNotFoundError);
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), true);
  // update
  BOOST_CHECK_THROW(policyManager.updateActorRevision("actor1"), ObjectNotFoundError);
  policyManager.updateActorRevision("actor2");
  BOOST_CHECK_EQUAL(policyManager.isReady("actor2"), false);
  policyManager.updateGlobalRevision();
}
