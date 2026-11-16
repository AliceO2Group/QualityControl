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
/// \file   testWorstOfAllAggregator.cxx
/// \author Piotr Konopka
///

#include "Common/WorstOfAllAggregator.h"

#include <QualityControl/Quality.h>
#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

TEST_CASE("test_WorstOfAllAggregator")
{
  WorstOfAllAggregator agg1;
  agg1.configure();
  agg1.setName("agg1");

  // prepare data
  std::shared_ptr<QualityObject> qoNull = std::make_shared<QualityObject>(Quality::Null, "testCheckNull", "TST");
  qoNull->addFlag(FlagTypeFactory::BadTracking(), "oh no");
  std::shared_ptr<QualityObject> qoGood = std::make_shared<QualityObject>(Quality::Good, "testCheckGood", "TST");
  std::shared_ptr<QualityObject> qoMedium = std::make_shared<QualityObject>(Quality::Medium, "testCheckMedium", "TST");
  qoMedium->addFlag(FlagTypeFactory::BadPID(), "booo");
  std::shared_ptr<QualityObject> qoBad = std::make_shared<QualityObject>(Quality::Bad, "testCheckBad", "TST");
  QualityObjectsMapType input;

  std::map<std::string, Quality> result1 = agg1.aggregate(input);
  REQUIRE(result1.size() == 1);
  CHECK(result1["agg1"] == Quality::Null); // because empty vector passed
  REQUIRE(result1["agg1"].getFlags().size() == 1);
  CHECK(result1["agg1"].getFlags().at(0).first == FlagTypeFactory::UnknownQuality());

  input[qoGood->getName()] = qoGood;
  std::map<std::string, Quality> result2 = agg1.aggregate(input);
  REQUIRE(result2.size() == 1);
  CHECK(result2["agg1"] == Quality::Good);
  CHECK(result2["agg1"].getFlags().size() == 0);

  input[qoMedium->getName()] = qoMedium;
  std::map<std::string, Quality> result3 = agg1.aggregate(input);
  REQUIRE(result3.size() == 1);
  CHECK(result3["agg1"] == Quality::Medium);
  REQUIRE(result3["agg1"].getFlags().size() == 1);
  CHECK(result3["agg1"].getFlags().at(0).first == FlagTypeFactory::BadPID());

  input[qoBad->getName()] = qoBad;
  std::map<std::string, Quality> result4 = agg1.aggregate(input);
  REQUIRE(result4.size() == 1);
  CHECK(result4["agg1"] == Quality::Bad);
  REQUIRE(result4["agg1"].getFlags().size() == 1);
  CHECK(result4["agg1"].getFlags().at(0).first == FlagTypeFactory::BadPID());

  input[qoNull->getName()] = qoNull;
  std::map<std::string, Quality> result5 = agg1.aggregate(input);
  REQUIRE(result5.size() == 1);
  CHECK(result5["agg1"] == Quality::Null);
  REQUIRE(result5["agg1"].getFlags().size() == 2);
  CHECK(result5["agg1"].getFlags().at(0).first == FlagTypeFactory::BadPID());
  CHECK(result5["agg1"].getFlags().at(1).first == FlagTypeFactory::BadTracking());
}

} // namespace o2::quality_control_modules::common
