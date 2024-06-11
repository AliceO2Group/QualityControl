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
/// \file    testQualitiesToFlagCollectionConverter.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/QualitiesToFlagCollectionConverter.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include <DataFormatsQualityControl/QualityControlFlagCollection.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#define BOOST_TEST_MODULE QO2QCFCConversion test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

BOOST_AUTO_TEST_CASE(test_NoQOs)
{
  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 1);
  auto& flag = *qcfc->begin();
  BOOST_CHECK_EQUAL(flag.getStart(), 5);
  BOOST_CHECK_EQUAL(flag.getEnd(), 100);
  BOOST_CHECK_EQUAL(flag.getFlag(), FlagTypeFactory::UnknownQuality());
  BOOST_CHECK_EQUAL(flag.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_NoBeginning)
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 10, 50 });
  qos[1].setValidity({ 50, 120 });

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 2);

  auto& flag1 = *qcfc->begin();
  BOOST_CHECK_EQUAL(flag1.getStart(), 5);
  BOOST_CHECK_EQUAL(flag1.getEnd(), 9);
  BOOST_CHECK_EQUAL(flag1.getFlag(), FlagTypeFactory::UnknownQuality());
  BOOST_CHECK_EQUAL(flag1.getSource(), "qc/DET/QO/xyzCheck");

  auto& flag2 = *(++qcfc->begin());
  BOOST_CHECK_EQUAL(flag2.getStart(), 10);
  BOOST_CHECK_EQUAL(flag2.getEnd(), 50);
  BOOST_CHECK_EQUAL(flag2.getFlag(), FlagTypeFactory::Unknown());
  BOOST_CHECK_EQUAL(flag2.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_NoEnd)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 80 });

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 1);
  auto& flag = *qcfc->begin();
  BOOST_CHECK_EQUAL(flag.getStart(), 80);
  BOOST_CHECK_EQUAL(flag.getEnd(), 100);
  BOOST_CHECK_EQUAL(flag.getFlag(), FlagTypeFactory::UnknownQuality());
  BOOST_CHECK_EQUAL(flag.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_WrongOrder)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 70, 90 });
  qos[1].setValidity({ 60, 90 });
  qos[2].setValidity({ 50, 120 });
  qos[3].setValidity({ 40, 120 });
  qos[4].setValidity({ 30, 120 });

  {
    // good to good
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    converter(qos[0]);
    BOOST_CHECK_THROW(converter(qos[1]), std::runtime_error);
  }
  {
    // good to bad
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    converter(qos[1]);
    BOOST_CHECK_THROW(converter(qos[2]), std::runtime_error);
  }
  {
    // bad to bad
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    converter(qos[2]);
    BOOST_CHECK_THROW(converter(qos[3]), std::runtime_error);
  }
  {
    // bad to good
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    converter(qos[3]);
    BOOST_CHECK_THROW(converter(qos[4]), std::runtime_error);
  }
}

BOOST_AUTO_TEST_CASE(test_MismatchingParameters)
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "TPC" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 10, 120 });
  qos[1].setValidity({ 1000, 10000 });
  qos[2].setValidity({ 40, 30 });

  {
    // different detector
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    BOOST_CHECK_THROW(converter(qos[0]), std::runtime_error);
  }
  {
    // QO start after the QCFC end limit
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    BOOST_CHECK_THROW(converter(qos[1]), std::runtime_error);
  }
  {
    // QO validity starts after it finishes
    std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
    QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
    BOOST_CHECK_THROW(converter(qos[2]), std::runtime_error);
  }
}

BOOST_AUTO_TEST_CASE(test_OverlappingQOs)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 50 });
  qos[1].setValidity({ 10, 50 });
  qos[2].setValidity({ 15, 60 });
  qos[3].setValidity({ 55, 120 });
  qos[4].setValidity({ 60, 120 });
  qos[5].setValidity({ 70, 120 });

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 1);

  auto& flag1 = *qcfc->begin();
  BOOST_CHECK_EQUAL(flag1.getStart(), 55);
  BOOST_CHECK_EQUAL(flag1.getEnd(), 100);
  BOOST_CHECK_EQUAL(flag1.getFlag(), FlagTypeFactory::Unknown());
  BOOST_CHECK_EQUAL(flag1.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_AdjacentQOs)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 10 });
  qos[1].setValidity({ 10, 14 });
  qos[2].setValidity({ 15, 49 });
  qos[3].setValidity({ 50, 80 });
  qos[4].setValidity({ 80, 95 });
  qos[5].setValidity({ 96, 120 });

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");

  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 1);

  auto& flag1 = *qcfc->begin();
  BOOST_CHECK_EQUAL(flag1.getStart(), 50);
  BOOST_CHECK_EQUAL(flag1.getEnd(), 100);
  BOOST_CHECK_EQUAL(flag1.getFlag(), FlagTypeFactory::Unknown());
  BOOST_CHECK_EQUAL(flag1.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_UnexplainedMediumIsBad)
{
  std::vector<QualityObject> qos{
    { Quality::Medium, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 150 });
  qos[1].setValidity({ 10, 100 });

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 1);

  auto& flag1 = *qcfc->begin();
  BOOST_CHECK_EQUAL(flag1.getStart(), 5);
  BOOST_CHECK_EQUAL(flag1.getEnd(), 100);
  BOOST_CHECK_EQUAL(flag1.getFlag(), FlagTypeFactory::Unknown());
  BOOST_CHECK_EQUAL(flag1.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_KnownFlags)
{
  std::vector<QualityObject> qos{
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 10 });
  qos[1].setValidity({ 10, 40 });
  qos[2].setValidity({ 30, 80 });
  qos[3].setValidity({ 50, 100 });

  qos[1].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");
  qos[2].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");
  qos[2].addFlag(FlagTypeFactory::BadHadronPID(), "evil CERN scientists changed the proton mass");
  qos[3].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");
  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 2);

  auto& flag1 = *qcfc->begin();
  BOOST_CHECK_EQUAL(flag1.getStart(), 10);
  BOOST_CHECK_EQUAL(flag1.getEnd(), 100);
  BOOST_CHECK_EQUAL(flag1.getFlag(), FlagTypeFactory::BadTracking());
  BOOST_CHECK_EQUAL(flag1.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(flag1.getSource(), "qc/DET/QO/xyzCheck");

  auto& flag2 = *(++qcfc->begin());
  BOOST_CHECK_EQUAL(flag2.getStart(), 30);
  BOOST_CHECK_EQUAL(flag2.getEnd(), 50);
  BOOST_CHECK_EQUAL(flag2.getFlag(), FlagTypeFactory::BadHadronPID());
  BOOST_CHECK_EQUAL(flag2.getComment(), "evil CERN scientists changed the proton mass");
  BOOST_CHECK_EQUAL(flag2.getSource(), "qc/DET/QO/xyzCheck");
}

BOOST_AUTO_TEST_CASE(test_TheSameFlagsButSeparated)
{
  std::vector<QualityObject> qos{
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Good, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" },
    { Quality::Bad, "xyzCheck", "DET" }
  };

  qos[0].setValidity({ 5, 25 });
  qos[1].setValidity({ 10, 40 });
  qos[2].setValidity({ 30, 50 });
  qos[3].setValidity({ 80, 100 });

  qos[0].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");
  qos[2].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");
  qos[3].addFlag(FlagTypeFactory::BadTracking(), "Bug in reco");

  std::unique_ptr<QualityControlFlagCollection> qcfc{ new QualityControlFlagCollection("test1", "DET", { 5, 100 }) };
  QualitiesToFlagCollectionConverter converter(std::move(qcfc), "qc/DET/QO/xyzCheck");

  for (const auto& qo : qos) {
    converter(qo);
  }
  qcfc = converter.getResult();

  BOOST_REQUIRE_EQUAL(qcfc->size(), 3);

  auto it = qcfc->begin();
  auto& flag1 = *it;
  BOOST_CHECK_EQUAL(flag1.getStart(), 5);
  BOOST_CHECK_EQUAL(flag1.getEnd(), 10);
  BOOST_CHECK_EQUAL(flag1.getFlag(), FlagTypeFactory::BadTracking());
  BOOST_CHECK_EQUAL(flag1.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(flag1.getSource(), "qc/DET/QO/xyzCheck");

  auto& flag2 = *(++it);
  BOOST_CHECK_EQUAL(flag2.getStart(), 30);
  BOOST_CHECK_EQUAL(flag2.getEnd(), 50);
  BOOST_CHECK_EQUAL(flag2.getFlag(), FlagTypeFactory::BadTracking());
  BOOST_CHECK_EQUAL(flag2.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(flag2.getSource(), "qc/DET/QO/xyzCheck");

  auto& flag3 = *(++it);
  BOOST_CHECK_EQUAL(flag3.getStart(), 80);
  BOOST_CHECK_EQUAL(flag3.getEnd(), 100);
  BOOST_CHECK_EQUAL(flag3.getFlag(), FlagTypeFactory::BadTracking());
  BOOST_CHECK_EQUAL(flag3.getComment(), "Bug in reco");
  BOOST_CHECK_EQUAL(flag3.getSource(), "qc/DET/QO/xyzCheck");
}
