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
  std::vector<Quality> aggregate(QualityObjectsType* qos) override
  {
    std::vector<Quality> result;
    switch(qos->size())
    {
      case 1:
        result.push_back(Quality::Good);
      break;
      case 2:
        result.push_back(Quality::Medium);
        break;
      case 3:
        result.push_back(Quality::Bad);
      break;
      default:
        result.push_back(Quality::Null);
        break;
    }
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
  std::shared_ptr<QualityObject> qo_null = make_shared<QualityObject>(0, "testCheck1", "TST");
  std::shared_ptr<QualityObject> qo_good = make_shared<QualityObject>(1, "testCheck1", "TST");
  std::shared_ptr<QualityObject> qo_medium = make_shared<QualityObject>(2, "testCheck1", "TST");
  std::shared_ptr<QualityObject> qo_bad = make_shared<QualityObject>(3, "testCheck1", "TST");
  QualityObjectsType input;

  vector<Quality> result1 = testAggregator.aggregate(&input);
  BOOST_CHECK_EQUAL(result1.size(), 1);
  BOOST_CHECK_EQUAL(result1.at(0), Quality::Null); // because empty vector passed

  input.push_back(qo_good);
  vector<Quality> result2 = testAggregator.aggregate(&input);
  BOOST_CHECK_EQUAL(result2.size(), 1);
  BOOST_CHECK_EQUAL(result2.at(0), Quality::Good);

  input.push_back(qo_good);
  vector<Quality> result3 = testAggregator.aggregate(&input);
  BOOST_CHECK_EQUAL(result3.size(), 1);
  BOOST_CHECK_EQUAL(result3.at(0), Quality::Medium);

  input.push_back(qo_good);
  vector<Quality> result4 = testAggregator.aggregate(&input);
  BOOST_CHECK_EQUAL(result4.size(), 1);
  BOOST_CHECK_EQUAL(result4.at(0), Quality::Bad);

}
