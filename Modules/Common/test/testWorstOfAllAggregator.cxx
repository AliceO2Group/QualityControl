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
/// \file   testWorstOfAllAggregator.cxx
/// \author Piotr Konopka
///

#include "Common/WorstOfAllAggregator.h"

#define BOOST_TEST_MODULE WorstOfAllAggregator test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <QualityControl/Quality.h>
#include "DataFormatsQualityControl/FlagReasons.h"

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

BOOST_AUTO_TEST_CASE(test_WorstOfAllAggregator)
{
  WorstOfAllAggregator agg1;
  agg1.configure("agg1");

  // prepare data
  std::shared_ptr<QualityObject> qoNull = std::make_shared<QualityObject>(Quality::Null, "testCheckNull", "TST");
  qoNull->addReason(FlagReasonFactory::ProcessingError(), "oh no");
  std::shared_ptr<QualityObject> qoGood = std::make_shared<QualityObject>(Quality::Good, "testCheckGood", "TST");
  std::shared_ptr<QualityObject> qoMedium = std::make_shared<QualityObject>(Quality::Medium, "testCheckMedium", "TST");
  qoMedium->addReason(FlagReasonFactory::LimitedAcceptance(), "booo");
  std::shared_ptr<QualityObject> qoBad = std::make_shared<QualityObject>(Quality::Bad, "testCheckBad", "TST");
  QualityObjectsMapType input;

  std::map<std::string, Quality> result1 = agg1.aggregate(input);
  BOOST_REQUIRE_EQUAL(result1.size(), 1);
  BOOST_CHECK_EQUAL(result1["agg1"], Quality::Null); // because empty vector passed
  BOOST_REQUIRE_EQUAL(result1["agg1"].getReasons().size(), 1);
  BOOST_CHECK_EQUAL(result1["agg1"].getReasons().at(0).first, FlagReasonFactory::MissingQualityObject());

  input[qoGood->getName()] = qoGood;
  std::map<std::string, Quality> result2 = agg1.aggregate(input);
  BOOST_REQUIRE_EQUAL(result2.size(), 1);
  BOOST_CHECK_EQUAL(result2["agg1"], Quality::Good);
  BOOST_CHECK_EQUAL(result2["agg1"].getReasons().size(), 0);

  input[qoMedium->getName()] = qoMedium;
  std::map<std::string, Quality> result3 = agg1.aggregate(input);
  BOOST_REQUIRE_EQUAL(result3.size(), 1);
  BOOST_CHECK_EQUAL(result3["agg1"], Quality::Medium);
  BOOST_REQUIRE_EQUAL(result3["agg1"].getReasons().size(), 1);
  BOOST_CHECK_EQUAL(result3["agg1"].getReasons().at(0).first, FlagReasonFactory::LimitedAcceptance());

  input[qoBad->getName()] = qoBad;
  std::map<std::string, Quality> result4 = agg1.aggregate(input);
  BOOST_REQUIRE_EQUAL(result4.size(), 1);
  BOOST_CHECK_EQUAL(result4["agg1"], Quality::Bad);
  BOOST_REQUIRE_EQUAL(result4["agg1"].getReasons().size(), 1);
  BOOST_CHECK_EQUAL(result4["agg1"].getReasons().at(0).first, FlagReasonFactory::LimitedAcceptance());

  input[qoNull->getName()] = qoNull;
  std::map<std::string, Quality> result5 = agg1.aggregate(input);
  BOOST_REQUIRE_EQUAL(result5.size(), 1);
  BOOST_CHECK_EQUAL(result5["agg1"], Quality::Null);
  BOOST_REQUIRE_EQUAL(result5["agg1"].getReasons().size(), 2);
  BOOST_CHECK_EQUAL(result5["agg1"].getReasons().at(0).first, FlagReasonFactory::LimitedAcceptance());
  BOOST_CHECK_EQUAL(result5["agg1"].getReasons().at(1).first, FlagReasonFactory::ProcessingError());
}

} // namespace o2::quality_control_modules::common
