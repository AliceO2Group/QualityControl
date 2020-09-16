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
#if __has_include(<Framework/DataSampling.h>)
#include <Framework/DataSampling.h>
#else
#include <DataSampling/DataSampling.h>
using namespace o2::utilities;
#endif
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

//BOOST_AUTO_TEST_CASE(test_check_isready)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("singleCheck", configFilePath);
//
//  std::string monitorObjectFullName = "skeletonTask/example"; /* taskName / monitorObjectName */
//  std::map<std::string, unsigned int> moMap = { { monitorObjectFullName, 10 } };
//  check.init();
//
//  BOOST_CHECK(check.isReady(moMap));
//  check.updateRevision(13);
//  BOOST_CHECK(!check.isReady(moMap));
//}

//BOOST_AUTO_TEST_CASE(test_check_policy_not_set)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("singleCheck", configFilePath);
//  std::map<std::string, unsigned int> moMap = { { "any", 10 } };
//
//  BOOST_CHECK_THROW(check.isReady(moMap), FatalException);
//}

/*
 * Test policy
 */
//
//BOOST_AUTO_TEST_CASE(test_check_single_mo)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("singleCheck", configFilePath);
//  check.init();
//
//  std::string monitorObjectFullName = "skeletonTask/example"; /* taskName / monitorObjectName */
//  std::map<std::string, unsigned int> moMap1 = { { monitorObjectFullName, 10 } };
//
//  BOOST_CHECK(check.isReady(moMap1));
//  check.updateRevision(10);
//  BOOST_CHECK(!check.isReady(moMap1));
//
//  std::map<std::string, unsigned int> moMap2 = { { monitorObjectFullName, 11 } };
//
//  BOOST_CHECK(check.isReady(moMap2));
//  check.updateRevision(11);
//  BOOST_CHECK(!check.isReady(moMap2));
//}
//
//BOOST_AUTO_TEST_CASE(test_check_policy_all)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("checkAll", configFilePath);
//  check.init();
//
//  std::string mo1 = "abcTask/test1"; /* taskName / monitorObjectName */
//  std::string mo2 = "abcTask/test2"; /* taskName / monitorObjectName */
//
//  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
//  // Single MO ready - all required - should fail
//  BOOST_CHECK(!check.isReady(moMap1));
//
//  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
//  // All ready - should success
//  BOOST_CHECK(check.isReady(moMap2));
//
//  // Update revision - mo1 is old
//  check.updateRevision(10);
//  // mo1 is to old, mo2 is ok - should fail
//  BOOST_CHECK(!check.isReady(moMap2));
//}
//
//BOOST_AUTO_TEST_CASE(test_check_policy_any)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("checkAny", configFilePath);
//  check.init();
//
//  std::string mo1 = "abcTask/test1"; /* taskName / monitorObjectName */
//  std::string mo2 = "abcTask/test2"; /* taskName / monitorObjectName */
//
//  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
//  // Single MO ready - should success
//  BOOST_CHECK(check.isReady(moMap1));
//
//  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
//  // All ready - should success
//  BOOST_CHECK(check.isReady(moMap2));
//
//  // Update revision - mo1 and mo2 is old
//  check.updateRevision(13);
//  // mo1 and mo2 is old - should fail
//  BOOST_CHECK(!check.isReady(moMap2));
//}
//
//BOOST_AUTO_TEST_CASE(test_check_policy_anynonzero)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("checkAnyNonZero", configFilePath);
//  check.init();
//
//  std::string mo1 = "abcTask/test1"; /* taskName / monitorObjectName */
//  std::string mo2 = "abcTask/test2"; /* taskName / monitorObjectName */
//
//  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
//  // Single MO ready - all required - should fail
//  BOOST_CHECK(!check.isReady(moMap1));
//
//  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
//  // All ready - should success
//  BOOST_CHECK(check.isReady(moMap2));
//
//  check.updateRevision(10);
//  BOOST_CHECK(check.isReady(moMap2));
//
//  check.updateRevision(13);
//  BOOST_CHECK(!check.isReady(moMap2));
//}
//
//BOOST_AUTO_TEST_CASE(test_check_policy_globalany)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("checkGlobalAny", configFilePath);
//  check.init();
//
//  std::string mo1 = "xyzTask/test1"; /* taskName / monitorObjectName */
//  std::string mo2 = "xyzTask/test2"; /* taskName / monitorObjectName */
//
//  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
//  BOOST_CHECK(check.isReady(moMap1));
//
//  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
//  BOOST_CHECK(check.isReady(moMap2));
//}
//
//BOOST_AUTO_TEST_CASE(test_check_policy_oneachseparately)
//{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
//
//  Check check("checkOnEachSeparately", configFilePath);
//  check.init();
//
//  std::string moName1 = "abcTask/test1"; /* taskName / monitorObjectName */
//  std::string moName2 = "abcTask/test2"; /* taskName / monitorObjectName */
//
//  TH1F h1(moName1.data(), moName1.data(), 100, 0, 99);
//  auto mo1 = std::make_shared<MonitorObject>(&h1, "abcTask");
//  mo1->setIsOwner(false);
//  TH1F h2(moName2.data(), moName2.data(), 100, 0, 99);
//  auto mo2 = std::make_shared<MonitorObject>(&h2, "abcTask");
//  mo2->setIsOwner(false);
//
//  std::map<std::string, unsigned int> revisionMap1 = { { moName1, 10 } };
//  BOOST_CHECK(check.isReady(revisionMap1));
//
//  std::map<std::string, std::shared_ptr<MonitorObject>> moMap1 = { { moName1, mo1 } };
//  auto qos1 = check.check(moMap1);
//  BOOST_REQUIRE_EQUAL(qos1.size(), 1);
//  BOOST_CHECK_EQUAL(qos1[0]->getPath(), "qc/DET/QO/checkOnEachSeparately/abcTask/test1");
//
//  std::map<std::string, unsigned int> revisionMap2 = { { moName1, 10 }, { moName2, 13 } };
//  BOOST_CHECK(check.isReady(revisionMap2));
//  std::map<std::string, std::shared_ptr<MonitorObject>> moMap2 = { { moName1, mo1 }, { moName2, mo2 } };
//  auto qos2 = check.check(moMap2);
//  BOOST_REQUIRE_EQUAL(qos2.size(), 2);
//  BOOST_CHECK_EQUAL(qos2[0]->getPath(), "qc/DET/QO/checkOnEachSeparately/abcTask/test1");
//  BOOST_CHECK_EQUAL(qos2[1]->getPath(), "qc/DET/QO/checkOnEachSeparately/abcTask/test2");
//}
