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
/// \file    testCheck.cxx
/// \author  Rafal Pacholek
///

#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/MonitorObject.h"
#include "getTestDataDirectory.h"
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

BOOST_AUTO_TEST_CASE(test_check_specs)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("singleCheck", configFilePath);

  BOOST_REQUIRE_EQUAL(check.getInputs().size(), 1);
  BOOST_CHECK_EQUAL(check.getInputs()[0], (InputSpec{ { "mo" }, "QC", "skeletonTask-mo", 0 }));

  BOOST_CHECK_EQUAL(check.getOutputSpec(), (OutputSpec{ "QC", "singleCheck-chk", 0 }));
}

BOOST_AUTO_TEST_CASE(test_check_isready)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("singleCheck", configFilePath);

  std::string monitorObjectFullName = "skeletonTask/example"; /* taskName / monitorObjectName */
  std::map<std::string, unsigned int> moMap = { { monitorObjectFullName, 10 } };
  check.init();

  BOOST_CHECK(check.isReady(moMap));
  check.updateRevision(13);
  BOOST_CHECK(!check.isReady(moMap));
}

BOOST_AUTO_TEST_CASE(test_check_policy_not_set)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("singleCheck", configFilePath);
  std::map<std::string, unsigned int> moMap = { { "any", 10 } };

  BOOST_CHECK_THROW(check.isReady(moMap), FatalException);
}

/*
 * Test policy
 */

BOOST_AUTO_TEST_CASE(test_check_single_mo)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("singleCheck", configFilePath);
  check.init();

  std::string monitorObjectFullName = "skeletonTask/example"; /* taskName / monitorObjectName */
  std::map<std::string, unsigned int> moMap1 = { { monitorObjectFullName, 10 } };

  BOOST_CHECK(check.isReady(moMap1));
  check.updateRevision(10);
  BOOST_CHECK(!check.isReady(moMap1));

  std::map<std::string, unsigned int> moMap2 = { { monitorObjectFullName, 11 } };

  BOOST_CHECK(check.isReady(moMap2));
  check.updateRevision(11);
  BOOST_CHECK(!check.isReady(moMap2));
}

BOOST_AUTO_TEST_CASE(test_check_policy_all)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("checkAll", configFilePath);
  check.init();

  std::string mo1 = "abcTask/test1"; /* taskName / monitorObjectName */
  std::string mo2 = "abcTask/test2"; /* taskName / monitorObjectName */

  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
  // Single MO ready - all required - should fail
  BOOST_CHECK(!check.isReady(moMap1));

  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
  // All ready - should success
  BOOST_CHECK(check.isReady(moMap2));

  // Update revision - mo1 is old
  check.updateRevision(10);
  // mo1 is to old, mo2 is ok - should fail
  BOOST_CHECK(!check.isReady(moMap2));
}

BOOST_AUTO_TEST_CASE(test_check_policy_any)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("checkAny", configFilePath);
  check.init();

  std::string mo1 = "abcTask/test1"; /* taskName / monitorObjectName */
  std::string mo2 = "abcTask/test2"; /* taskName / monitorObjectName */

  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
  // Single MO ready - should success
  BOOST_CHECK(check.isReady(moMap1));

  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
  // All ready - should success
  BOOST_CHECK(check.isReady(moMap2));

  // Update revision - mo1 and mo2 is old
  check.updateRevision(13);
  // mo1 and mo2 is old - should fail
  BOOST_CHECK(!check.isReady(moMap2));
}

BOOST_AUTO_TEST_CASE(test_check_policy_anynonzero)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("checkAnyNonZero", configFilePath);
  check.init();

  std::string mo1 = "abcTask/test1"; /* taskName / monitorObjectName */
  std::string mo2 = "abcTask/test2"; /* taskName / monitorObjectName */

  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
  // Single MO ready - all required - should fail
  BOOST_CHECK(!check.isReady(moMap1));

  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
  // All ready - should success
  BOOST_CHECK(check.isReady(moMap2));

  check.updateRevision(10);
  BOOST_CHECK(check.isReady(moMap2));

  check.updateRevision(13);
  BOOST_CHECK(!check.isReady(moMap2));
}

BOOST_AUTO_TEST_CASE(test_check_policy_globalany)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("checkGlobalAny", configFilePath);
  check.init();

  std::string mo1 = "xyzTask/test1"; /* taskName / monitorObjectName */
  std::string mo2 = "xyzTask/test2"; /* taskName / monitorObjectName */

  std::map<std::string, unsigned int> moMap1 = { { mo1, 10 } };
  BOOST_CHECK(check.isReady(moMap1));

  std::map<std::string, unsigned int> moMap2 = { { mo1, 10 }, { mo2, 13 } };
  BOOST_CHECK(check.isReady(moMap2));
}

BOOST_AUTO_TEST_CASE(test_check_policy_oneachseparately)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("checkOnEachSeparately", configFilePath);
  check.init();

  std::string moName1 = "abcTask/test1"; /* taskName / monitorObjectName */
  std::string moName2 = "abcTask/test2"; /* taskName / monitorObjectName */

  TH1F h1(moName1.data(), moName1.data(), 100, 0, 99);
  auto mo1 = std::make_shared<MonitorObject>(&h1, "abcTask");
  mo1->setIsOwner(false);
  TH1F h2(moName2.data(), moName2.data(), 100, 0, 99);
  auto mo2 = std::make_shared<MonitorObject>(&h2, "abcTask");
  mo2->setIsOwner(false);

  std::map<std::string, unsigned int> revisionMap1 = { { moName1, 10 } };
  BOOST_CHECK(check.isReady(revisionMap1));

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap1 = { { moName1, mo1 } };
  auto qos1 = check.check(moMap1);
  BOOST_REQUIRE_EQUAL(qos1.size(), 1);
  BOOST_CHECK_EQUAL(qos1[0]->getPath(), "qc/DET/QO/checkOnEachSeparately/abcTask/test1");

  std::map<std::string, unsigned int> revisionMap2 = { { moName1, 10 }, { moName2, 13 } };
  BOOST_CHECK(check.isReady(revisionMap2));
  std::map<std::string, std::shared_ptr<MonitorObject>> moMap2 = { { moName1, mo1 }, { moName2, mo2 } };
  auto qos2 = check.check(moMap2);
  BOOST_REQUIRE_EQUAL(qos2.size(), 2);
  BOOST_CHECK_EQUAL(qos2[0]->getPath(), "qc/DET/QO/checkOnEachSeparately/abcTask/test1");
  BOOST_CHECK_EQUAL(qos2[1]->getPath(), "qc/DET/QO/checkOnEachSeparately/abcTask/test2");
}

/*
 * Test check and beautify
 */
class TestCheck : public CheckInterface
{
 public:
  TestCheck() {}
  void configure(std::string name)
  {
    (void)name;
  }
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
  {
    if (moMap->size()) {
      mCheck = true;
    }
    return Quality();
  }
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null)
  {
    (void)checkResult;
    if (mo) {
      mBeautify = true;
    }
  }
  std::string getAcceptedType()
  {
    return "any";
  }

  bool mCheck = false;
  bool mBeautify = false;
};

BOOST_AUTO_TEST_CASE(test_check_invoke_check_beautify)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("singleCheck", configFilePath);
  check.init();

  TestCheck testCheck;
  check.setCheckInterface(dynamic_cast<CheckInterface*>(&testCheck));

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "skeletonTask/example", std::shared_ptr<MonitorObject>(new MonitorObject()) } };

  check.check(moMap);
  // Check should run
  BOOST_CHECK(testCheck.mCheck);
  // Beautify should run - single MO declared
  BOOST_CHECK(testCheck.mBeautify);
}

BOOST_AUTO_TEST_CASE(test_check_dont_invoke_beautify)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("checkAny", configFilePath);
  check.init();

  TestCheck testCheck;
  check.setCheckInterface(dynamic_cast<CheckInterface*>(&testCheck));

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "abcTask/test1", std::shared_ptr<MonitorObject>() } };

  check.check(moMap);
  // Check should run
  BOOST_CHECK(testCheck.mCheck);
  // Beautify should not run - more than one MO declared
  BOOST_CHECK(!testCheck.mBeautify);
}
