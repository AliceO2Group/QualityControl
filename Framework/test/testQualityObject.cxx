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
/// \file   testQualityObject.cxx
/// \author Piotr Konopka
///

#include "QualityControl/QualityObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/testUtils.h"

#include <DataFormatsQualityControl/FlagReasons.h>

#define BOOST_TEST_MODULE QualityObject test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace o2::quality_control::test;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

BOOST_AUTO_TEST_CASE(quality_object_test_constructors)
{
  QualityObject qo(Quality::Medium,
                   "xyzCheck",
                   "TST",
                   "",
                   { "qc/TST/testTask/mo1", "qc/TST/testTask/mo2" },
                   {},
                   { { "probability", "0.45" }, { "threshold_medium", "0.42" } });

  BOOST_CHECK_EQUAL(qo.getName(), "xyzCheck");
  BOOST_CHECK(strcmp(qo.GetName(), "xyzCheck") == 0);
  BOOST_CHECK_EQUAL(qo.getDetectorName(), "TST");
  BOOST_CHECK_EQUAL(qo.getQuality(), Quality::Medium);
  BOOST_REQUIRE_EQUAL(qo.getInputs().size(), 2);
  BOOST_CHECK_EQUAL(qo.getInputs()[0], "qc/TST/testTask/mo1");
  BOOST_CHECK_EQUAL(qo.getInputs()[1], "qc/TST/testTask/mo2");
  BOOST_REQUIRE_EQUAL(qo.getMetadataMap().count("probability"), 1);
  BOOST_CHECK_EQUAL(qo.getMetadataMap().at("probability"), "0.45");
  BOOST_REQUIRE_EQUAL(qo.getMetadataMap().count("threshold_medium"), 1);
  BOOST_CHECK_EQUAL(qo.getMetadataMap().at("threshold_medium"), "0.42");

  auto qo2 = qo;

  BOOST_CHECK_EQUAL(qo2.getName(), "xyzCheck");
  BOOST_CHECK(strcmp(qo2.GetName(), "xyzCheck") == 0);
  BOOST_CHECK_EQUAL(qo2.getDetectorName(), "TST");
  BOOST_CHECK_EQUAL(qo2.getQuality(), Quality::Medium);
  BOOST_REQUIRE_EQUAL(qo2.getInputs().size(), 2);
  BOOST_CHECK_EQUAL(qo2.getInputs()[0], "qc/TST/testTask/mo1");
  BOOST_CHECK_EQUAL(qo2.getInputs()[1], "qc/TST/testTask/mo2");
  BOOST_REQUIRE_EQUAL(qo2.getMetadataMap().count("probability"), 1);
  BOOST_CHECK_EQUAL(qo2.getMetadataMap().at("probability"), "0.45");
  BOOST_REQUIRE_EQUAL(qo2.getMetadataMap().count("threshold_medium"), 1);
  BOOST_CHECK_EQUAL(qo2.getMetadataMap().at("threshold_medium"), "0.42");
}

BOOST_AUTO_TEST_CASE(quality_object_test_setters)
{
  QualityObject qo(Quality::Null, "xyzCheck");
  qo.setDetectorName("INVALID");
  qo.setDetectorName("TST");
  qo.setQuality(Quality::Null);
  qo.setQuality(Quality::Medium);
  qo.setInputs({ "that should be overwritten" });
  qo.setInputs({ "qc/TST/testTask/mo1", "qc/TST/testTask/mo2" });
  qo.addMetadata("probability", "0.45");
  qo.addMetadata("threshold_medium", "0.42");

  BOOST_CHECK_EQUAL(qo.getName(), "xyzCheck");
  BOOST_CHECK(strcmp(qo.GetName(), "xyzCheck") == 0);
  BOOST_CHECK_EQUAL(qo.getDetectorName(), "TST");
  BOOST_CHECK_EQUAL(qo.getQuality(), Quality::Medium);
  BOOST_REQUIRE_EQUAL(qo.getInputs().size(), 2);
  BOOST_CHECK_EQUAL(qo.getInputs()[0], "qc/TST/testTask/mo1");
  BOOST_CHECK_EQUAL(qo.getInputs()[1], "qc/TST/testTask/mo2");
  BOOST_REQUIRE_EQUAL(qo.getMetadataMap().count("probability"), 1);
  BOOST_CHECK_EQUAL(qo.getMetadataMap().at("probability"), "0.45");
  BOOST_REQUIRE_EQUAL(qo.getMetadataMap().count("threshold_medium"), 1);
  BOOST_CHECK_EQUAL(qo.getMetadataMap().at("threshold_medium"), "0.42");
}

BOOST_AUTO_TEST_CASE(qopath)
{
  // no policy
  QualityObject qo(Quality::Null, "xyzCheck", "DET");
  string path = qo.getPath();
  BOOST_CHECK_EQUAL(path, "qc/DET/QO/xyzCheck");

  // a policy which is not OnEachSeparately
  QualityObject qo2(Quality::Null, "xyzCheck", "DET", "OnAnyNonZero");
  string path2 = qo2.getPath();
  BOOST_CHECK_EQUAL(path2, "qc/DET/QO/xyzCheck");

  // policy is OnEachSeparately
  QualityObject qo3(Quality::Null, "xyzCheck", "DET", "OnEachSeparately", {}, { "objectABC" });
  string path3 = qo3.getPath();
  BOOST_CHECK_EQUAL(path3, "qc/DET/QO/xyzCheck/objectABC");

  // policy is OnEachSeparately and the vector is empty
  QualityObject qo4(Quality::Null, "xyzCheck", "DET", "OnEachSeparately", {}, {});
  BOOST_CHECK_EXCEPTION(qo4.getPath(), AliceO2::Common::FatalException, do_nothing);
}

BOOST_AUTO_TEST_CASE(qo_reasons)
{
  QualityObject qo1(Quality::Bad, "xyzCheck", "DET");
  qo1.addReason(FlagReasonFactory::ProcessingError(), "exception in x");
  qo1.addReason(FlagReasonFactory::ProcessingError(), "exception in y");
  qo1.addReason(FlagReasonFactory::LimitedAcceptance(), "sector C off");

  auto reasons1 = qo1.getReasons();
  BOOST_CHECK_EQUAL(reasons1[0].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(reasons1[0].second, "exception in x");
  BOOST_CHECK_EQUAL(reasons1[1].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(reasons1[1].second, "exception in y");
  BOOST_CHECK_EQUAL(reasons1[2].first, FlagReasonFactory::LimitedAcceptance());
  BOOST_CHECK_EQUAL(reasons1[2].second, "sector C off");

  auto qo2 = qo1;
  auto reasons2 = qo2.getReasons();
  BOOST_CHECK_EQUAL(reasons2[0].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(reasons2[0].second, "exception in x");
  BOOST_CHECK_EQUAL(reasons2[1].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(reasons2[1].second, "exception in y");
  BOOST_CHECK_EQUAL(reasons2[2].first, FlagReasonFactory::LimitedAcceptance());
  BOOST_CHECK_EQUAL(reasons2[2].second, "sector C off");

  auto quality = qo1.getQuality();
  auto reasons3 = quality.getReasons();
  BOOST_CHECK_EQUAL(reasons3[0].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(reasons3[0].second, "exception in x");
  BOOST_CHECK_EQUAL(reasons3[1].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(reasons3[1].second, "exception in y");
  BOOST_CHECK_EQUAL(reasons3[2].first, FlagReasonFactory::LimitedAcceptance());
  BOOST_CHECK_EQUAL(reasons3[2].second, "sector C off");
}