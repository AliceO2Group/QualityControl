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
/// \file    testAggregatorInterface.cxx
/// \author  Barthelemy von Haller
///

#include "QualityControl/AggregatorInterface.h"

#define BOOST_TEST_MODULE AggregatorInterface test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TObjString.h>
#include <string>

using namespace o2::quality_control;
using namespace std;

namespace o2::quality_control
{

using namespace core;

namespace test
{

class SimpleTestAggregator : public checker::AggregatorInterface
{
 public:
  /// Default constructor
  SimpleTestAggregator() = default;
  /// Destructor
  ~SimpleTestAggregator() override = default;

  // Override interface
  void configure(std::string name) override
  {
    mValidString = name;
  }

  // Returns a quality matching the number of quality objects passed as argument (1 good, 2 medium, 3 bad, otherwise null)
  std::map<std::string, Quality> aggregate(std::map<std::string, std::shared_ptr<const o2::quality_control::core::QualityObject>>& qoMap) override
  {
    std::map<std::string, Quality> result;
    Quality q;
    switch (qoMap.size()) {
      case 1:
        q = Quality::Good;
        break;
      case 2:
        q = Quality::Medium;
        break;
      case 3:
        q = Quality::Bad;
        break;
      default:
        q = Quality::Null;
        break;
    }
    result["asdf"] = q;
    return result;
  }

  string mValidString;
};

} /* namespace test */
} /* namespace o2::quality_control */

BOOST_AUTO_TEST_CASE(test_invoke_all_methods)
{
  test::SimpleTestAggregator testAggregator;

  // prepare data
  std::shared_ptr<QualityObject> qo_null = make_shared<QualityObject>(0, "testCheckNull", "TST");
  std::shared_ptr<QualityObject> qo_good = make_shared<QualityObject>(1, "testCheckGood", "TST");
  std::shared_ptr<QualityObject> qo_medium = make_shared<QualityObject>(2, "testCheckMedium", "TST");
  std::shared_ptr<QualityObject> qo_bad = make_shared<QualityObject>(3, "testCheckBad", "TST");
  QualityObjectsMapType input;

  std::map<std::string, Quality> result1 = testAggregator.aggregate(input);
  BOOST_CHECK_EQUAL(result1.size(), 1);
  BOOST_CHECK_EQUAL(result1["asdf"], Quality::Null); // because empty vector passed

  input[qo_good->getName()] = qo_good;
  std::map<std::string, Quality> result2 = testAggregator.aggregate(input);
  BOOST_CHECK_EQUAL(result2.size(), 1);
  BOOST_CHECK_EQUAL(result2["asdf"], Quality::Good);

  input[qo_medium->getName()] = qo_medium;
  std::map<std::string, Quality> result3 = testAggregator.aggregate(input);
  BOOST_CHECK_EQUAL(result3.size(), 1);
  BOOST_CHECK_EQUAL(result3["asdf"], Quality::Medium);

  input[qo_bad->getName()] = qo_bad;
  std::map<std::string, Quality> result4 = testAggregator.aggregate(input);
  BOOST_CHECK_EQUAL(result4.size(), 1);
  BOOST_CHECK_EQUAL(result4["asdf"], Quality::Bad);
}
